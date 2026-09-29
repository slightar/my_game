// Focused verification for the two reported bugs.
//   Bug 1 - the shield soldier's bash must face the direction it actually attacks.
//   Bug 2 - a button grays out while held and only fires after the pointer is released.
// Run from a directory that has ./assets next to the executable (raylib app dir).
#include "character_art.h"
#include "character_repository.h"
#include "enemy_data.h"
#include "enemy_renderer.h"
#include "enemy_system.h"
#include "file_path.h"
#include "main_menu.h"
#include "terminal_ui.h"
#include "ui_font.h"
#include "raylib.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int gFailures = 0;

void Check(bool ok, const std::string& what) {
    std::cout << (ok ? "  ok    " : "  FAIL  ") << what << '\n';
    if (!ok) ++gFailures;
}

Image RenderToImage(int width, int height, const std::function<void()>& draw,
                    Color clear = Color{28, 33, 41, 255}) {
    auto target = LoadRenderTexture(width, height);
    if (!target.id) throw std::runtime_error("render target allocation failed");
    BeginTextureMode(target);
    ClearBackground(clear);
    draw();
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    UnloadRenderTexture(target);
    return image;
}

// Mean colour of a rectangle. Transparent pixels are ignored so the probe can sit
// on a partially covered control without diluting the sample.
Color SampleRegion(const Image& image, Rectangle rect) {
    long long r = 0, g = 0, b = 0, n = 0;
    const int x0 = std::max(0, static_cast<int>(rect.x));
    const int y0 = std::max(0, static_cast<int>(rect.y));
    const int x1 = std::min(image.width, static_cast<int>(rect.x + rect.width));
    const int y1 = std::min(image.height, static_cast<int>(rect.y + rect.height));
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x) {
            const Color c = GetImageColor(image, x, y);
            if (c.a < 8) continue;
            r += c.r; g += c.g; b += c.b; ++n;
        }
    if (!n) return BLANK;
    return Color{static_cast<unsigned char>(r / n), static_cast<unsigned char>(g / n),
                 static_cast<unsigned char>(b / n), 255};
}

// How far a colour sits from neutral gray. The shared press veil must move every
// control toward gray, whatever its base colour is.
float DistanceToGray(Color c) {
    const float dr = c.r - 128.0F, dg = c.g - 128.0F, db = c.b - 128.0F;
    return std::sqrt(dr * dr + dg * dg + db * db);
}

// Alpha-weighted horizontal centroid of everything drawn, relative to feet.x.
// A pose drawn facing right puts its mass right of the feet, and vice versa.
float SilhouetteBias(const Image& image, float feetX) {
    double total = 0, weighted = 0;
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x) {
            const unsigned char a = GetImageColor(image, x, y).a;
            if (a < 24) continue;
            total += a;
            weighted += static_cast<double>(x) * a;
        }
    if (total <= 0) return 0.0F;
    return static_cast<float>(weighted / total) - feetX;
}

}  // namespace

int main() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "bug fix verification");
    if (!IsWindowReady()) { std::cerr << "window not ready\n"; return 2; }

    try {
        const auto app = Utf8Path(GetApplicationDirectory());
        const auto fixture = app / "fix-verify-fixture";
        std::filesystem::create_directories(fixture);
        { ProgressSystem seed(fixture); seed.ResetProgress(); }

        UiFont font;
        CharacterArt art;
        {
            LocalCharacterRepository repository(app / "assets" / "characters");
            const auto list = repository.LoadAll();
            for (const auto& character : list.characters)
                art.RegisterCharacter(character, repository.Root());
        }

        enum : unsigned { MouseUp = 5, MouseDown = 6, MousePosition = 7 };
        const auto event = [](unsigned type, int a, int b = 0) {
            PlayAutomationEvent({0, type, {a, b, 0, 0}});
        };
        const auto down = [&](int x, int y) {
            event(MousePosition, x, y); event(MouseDown, MOUSE_BUTTON_LEFT);
        };
        const auto up = [&] { event(MouseUp, MOUSE_BUTTON_LEFT); };
        const auto moveTo = [&](int x, int y) { event(MousePosition, x, y); };

        MainMenu menu(fixture);
        menu.OpenHome();

        // ---------------------------------------------------------------- Bug 2
        std::cout << "\n[Bug 2] press grays the control, release commits the action\n";

        // (a) A Button()-drawn control (Ink fill) must move toward gray while held.
        const Rectangle trial = TerminalUi::EnemyTrialButton;      // {432,397,218,42}
        const Rectangle trialProbe{438, 403, 14, 20};              // flat fill, left of the label
        const Color trialIdle = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), trialProbe);

        down(520, 418);                                            // press over 小怪试炼
        menu.Update();
        const Color trialHeld = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), trialProbe);

        up(); menu.Update();
        const Color trialReleased = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), trialProbe);

        std::cout << "        idle=" << (int)trialIdle.r << ',' << (int)trialIdle.g << ','
                  << (int)trialIdle.b << "  held=" << (int)trialHeld.r << ',' << (int)trialHeld.g
                  << ',' << (int)trialHeld.b << "  released=" << (int)trialReleased.r << ','
                  << (int)trialReleased.g << ',' << (int)trialReleased.b << '\n';
        Check(DistanceToGray(trialHeld) < DistanceToGray(trialIdle) - 10.0F,
              "held button is grayer than its idle colour");
        Check(std::abs((int)trialHeld.r - (int)trialIdle.r) > 10 ||
                  std::abs((int)trialHeld.g - (int)trialIdle.g) > 10 ||
                  std::abs((int)trialHeld.b - (int)trialIdle.b) > 10,
              "held button visibly changes (the gray veil is drawn)");
        Check(std::abs((int)trialReleased.r - (int)trialIdle.r) <= 6 &&
                  std::abs((int)trialReleased.g - (int)trialIdle.g) <= 6 &&
                  std::abs((int)trialReleased.b - (int)trialIdle.b) <= 6,
              "the gray veil disappears again after release");

        // (b) The same feedback must reach the hand-drawn home tiles (Paper fill).
        menu.OpenHome();
        const Rectangle tile = TerminalUi::HomeButtons[3];          // 设置, {720,522,238,66}
        const Rectangle tileProbe{tile.x + 170, tile.y + 10, 40, 40};
        const Color tileIdle = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), tileProbe);
        down(800, 550); menu.Update();
        const Color tileHeld = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), tileProbe);
        up(); menu.Update();
        std::cout << "        tile idle=" << (int)tileIdle.r << ',' << (int)tileIdle.g << ','
                  << (int)tileIdle.b << "  held=" << (int)tileHeld.r << ',' << (int)tileHeld.g
                  << ',' << (int)tileHeld.b << '\n';
        Check(DistanceToGray(tileHeld) < DistanceToGray(tileIdle) - 20.0F,
              "home tile uses the same unified gray press reaction");

        // (c) Holding must not fire the action; release over the control fires it once.
        menu.OpenHome();
        const Rectangle quit = TerminalUi::HomeButtons[5];          // 退出, {974,522,238,66}
        down(static_cast<int>(quit.x + quit.width / 2),
             static_cast<int>(quit.y + quit.height / 2));
        bool firedWhileHeld = false;
        for (int frame = 0; frame < 60; ++frame)
            if (menu.Update() != MenuAction::None) firedWhileHeld = true;
        Check(!firedWhileHeld, "60 held frames never fire the action");
        up();
        Check(menu.Update() == MenuAction::Quit, "release over the button fires the action once");
        Check(menu.Update() == MenuAction::None, "the action does not repeat on later frames");

        // (d) Pressing the control but releasing elsewhere must not fire.
        menu.OpenHome();
        down(static_cast<int>(quit.x + quit.width / 2),
             static_cast<int>(quit.y + quit.height / 2));
        menu.Update();
        moveTo(200, 600);                                           // drag off the control
        menu.Update();
        up();
        Check(menu.Update() == MenuAction::None, "releasing away from the button does not fire");

        // (e) The settings rows share the veil, and the volume slider still drags.
        menu.OpenSettings();
        const Rectangle volumeRow{84, 166, 1115, 71};
        const Rectangle volumeProbe{230, 176, 60, 40};           // flat fill, clear of the slider
        const Color volumeIdle = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), volumeProbe);
        down(860, 200); menu.Update();
        const int volumeAfterPress = menu.Settings().MasterVolume();
        const Color volumeHeld = SampleRegion(
            RenderToImage(1280, 720, [&] { menu.Draw(font, art); }), volumeProbe);
        up(); menu.Update();
        std::cout << "        volume row idle=" << (int)volumeIdle.r << ',' << (int)volumeIdle.g
                  << ',' << (int)volumeIdle.b << "  held=" << (int)volumeHeld.r << ','
                  << (int)volumeHeld.g << ',' << (int)volumeHeld.b << "  volume="
                  << volumeAfterPress << '\n';
        Check(volumeAfterPress >= 48 && volumeAfterPress <= 50,
              "volume slider still commits the dragged value");
        Check(menu.Settings().MasterVolume() == volumeAfterPress,
              "releasing the slider does not apply the value twice");
        Check(DistanceToGray(volumeHeld) < DistanceToGray(volumeIdle) - 10.0F,
              "settings row uses the same unified gray press reaction");
        (void)volumeRow;

        // ---------------------------------------------------------------- Bug 1
        std::cout << "\n[Bug 1] shield bash faces the way it attacks\n";
        EnemyRenderer renderer;
        Check(renderer.HasSprite(EnemyKind::Shield), "shield sprite loaded");

        constexpr float kFeetX = 256.0F, kFeetY = 400.0F;
        const auto bias = [&](EnemyState state, float time, int facing) {
            EnemyUnit unit;
            unit.kind = EnemyKind::Shield;
            unit.state = state;
            unit.facing = facing;
            unit.feet = {kFeetX, kFeetY};
            unit.animationTime = time;
            unit.health = EnemyData(EnemyKind::Shield).health;
            auto image = RenderToImage(512, 512, [&] { renderer.DrawUnit(unit, font); }, BLANK);
            const float value = SilhouetteBias(image, kFeetX);
            UnloadImage(image);
            return value;
        };
        const float idleLeft = bias(EnemyState::Approach, 0.0F, -1);
        const float idleRight = bias(EnemyState::Approach, 0.0F, 1);
        const float bashLeft = bias(EnemyState::Strike, 0.055F, -1);   // row 2, frame 5
        const float bashRight = bias(EnemyState::Strike, 0.055F, 1);
        std::cout << "        silhouette bias from feet.x (px, negative = faces left)\n"
                  << "          idle facing left  : " << idleLeft << '\n'
                  << "          idle facing right : " << idleRight << '\n'
                  << "          bash facing left  : " << bashLeft << '\n'
                  << "          bash facing right : " << bashRight << '\n';

        Check(idleLeft < 0.0F && idleRight > 0.0F, "idle pose mirrors with facing");
        // The regression: a left-facing guard used to render its bash with the native
        // right-facing pose, so the shield swung away from the target it was hitting.
        Check(bashLeft < -12.0F, "left-facing bash silhouette points left (the fix)");
        Check(bashRight > 12.0F, "right-facing bash silhouette points right");
        Check((idleLeft < 0.0F) == (bashLeft < 0.0F),
              "left-facing guard bashes the same way it idles");
        Check((idleRight > 0.0F) == (bashRight > 0.0F),
              "right-facing guard bashes the same way it idles");

        // Contact sheet for manual review: idle / wind-up / bash / recovery, both facings.
        const auto sheetPath = app / "shield-fix-sheet.png";
        auto sheet = RenderToImage(512 * 4, 512 * 2, [&] {
            const float times[4] = {0.0F, 0.0F, 0.055F, 0.0F};
            const EnemyState states[4] = {EnemyState::Approach, EnemyState::Windup,
                                          EnemyState::Strike, EnemyState::Recover};
            for (int row = 0; row < 2; ++row) {
                const int facing = row == 0 ? -1 : 1;
                for (int col = 0; col < 4; ++col) {
                    EnemyUnit unit;
                    unit.kind = EnemyKind::Shield;
                    unit.state = states[col];
                    unit.facing = facing;
                    unit.feet = {kFeetX + col * 512.0F, kFeetY + row * 512.0F};
                    unit.animationTime = col == 1 ? 0.0F : times[col];
                    unit.health = EnemyData(EnemyKind::Shield).health;
                    if (col == 1) unit.timer = 0.0F;
                    renderer.DrawUnit(unit, font);
                    DrawLine(static_cast<int>(unit.feet.x), 0,
                             static_cast<int>(unit.feet.x), 512, Color{255, 96, 96, 120});
                }
            }
        });
        const bool saved = ExportImage(sheet, sheetPath.string().c_str());
        UnloadImage(sheet);
        Check(saved, "wrote shield-fix-sheet.png for visual review");
        std::cout << "        contact sheet: " << sheetPath.string() << '\n';
    } catch (const std::exception& e) {
        std::cerr << "exception: " << e.what() << '\n';
        gFailures = 1;
    }

    std::cout << "\n" << (gFailures == 0 ? "ALL CHECKS PASSED" : "FAILURES: " + std::to_string(gFailures))
              << '\n';
    CloseWindow();
    return gFailures == 0 ? 0 : 1;
}
