// Trace what a shield soldier actually renders through a real attack, driven by
// EnemySystem::Update so the state machine, facing and sprite are all the live ones.
#include "enemy_data.h"
#include "enemy_renderer.h"
#include "enemy_system.h"
#include "file_path.h"
#include "ui_font.h"
#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

const char* StateName(EnemyState state) {
    switch (state) {
        case EnemyState::Approach: return "Approach";
        case EnemyState::Windup: return "Windup";
        case EnemyState::Strike: return "Strike";
        case EnemyState::Recover: return "Recover";
        case EnemyState::Fuse: return "Fuse";
        case EnemyState::Blast: return "Blast";
        case EnemyState::Dead: return "Dead";
    }
    return "?";
}

// Alpha-weighted horizontal centroid of a unit render, relative to feet.x.
float Bias(EnemyRenderer& renderer, const UiFont& font, const EnemyUnit& unit, float feetX) {
    auto target = LoadRenderTexture(512, 512);
    BeginTextureMode(target);
    ClearBackground(BLANK);
    EnemyUnit copy = unit;
    copy.feet = {feetX, 400.0F};
    renderer.DrawUnit(copy, font);
    EndTextureMode();
    Image image = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&image);
    double total = 0, weighted = 0;
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x) {
            const unsigned char a = GetImageColor(image, x, y).a;
            if (a < 24) continue;
            total += a;
            weighted += static_cast<double>(x) * a;
        }
    UnloadImage(image);
    UnloadRenderTexture(target);
    if (total <= 0) return 0.0F;
    return static_cast<float>(weighted / total) - feetX;
}

struct Sample {
    int step = 0;
    float feetX = 0, playerX = 0;
    EnemyState state = EnemyState::Approach;
    int facing = 0;
    bool turning = false;
    float bias = 0;
};

void Run(const char* title, float shieldX, float playerStart, float playerEnd, int frames) {
    std::printf("\n=== %s ===\n", title);
    std::printf("shield starts at x=%.0f, player moves %.0f -> %.0f\n", shieldX, playerStart,
                playerEnd);
    EnemyRenderer renderer;
    UiFont font;
    EnemySystem system;
    system.Clear();
    const unsigned id = system.Spawn(EnemyKind::Shield, shieldX);
    (void)id;

    std::vector<Sample> samples;
    EnemyState previous = EnemyState::Approach;
    int previousFacing = 0;
    for (int step = 0; step < frames; ++step) {
        const float t = frames > 1 ? static_cast<float>(step) / static_cast<float>(frames - 1)
                                   : 0.0F;
        const Vector2 player{playerStart + (playerEnd - playerStart) * t, GameConfig::kFloorY};
        system.Update(1.0F / 60.0F, player);
        if (system.Units().empty()) continue;
        const EnemyUnit& unit = system.Units().front();
        Sample sample;
        sample.step = step;
        sample.feetX = unit.feet.x;
        sample.playerX = player.x;
        sample.state = unit.state;
        sample.facing = unit.facing;
        sample.turning = unit.turning;
        sample.bias = Bias(renderer, font, unit, 256.0F);
        samples.push_back(sample);
        if (unit.state != previous || unit.facing != previousFacing) {
            std::printf("  step %3d  %-8s facing=%+d turning=%d  feet.x=%7.1f  player.x=%7.1f  "
                        "spriteBias=%+7.2f\n",
                        step, StateName(unit.state), unit.facing, unit.turning ? 1 : 0,
                        unit.feet.x, player.x, sample.bias);
            previous = unit.state;
            previousFacing = unit.facing;
        }
    }

    // Montage of every 4th frame so the whole motion can be eyeballed.
    const int columns = 12;
    const int rows = (static_cast<int>(samples.size()) + 3) / 4 / columns + 1;
    auto sheet = LoadRenderTexture(columns * 200, rows * 200);
    BeginTextureMode(sheet);
    ClearBackground(Color{26, 30, 38, 255});
    EnemySystem replay;
    replay.Clear();
    replay.Spawn(EnemyKind::Shield, shieldX);
    int cell = 0;
    for (int step = 0; step < frames && cell < columns * rows; ++step) {
        const float t = frames > 1 ? static_cast<float>(step) / static_cast<float>(frames - 1)
                                   : 0.0F;
        const Vector2 player{playerStart + (playerEnd - playerStart) * t, GameConfig::kFloorY};
        replay.Update(1.0F / 60.0F, player);
        if (step % 4 != 0) continue;
        if (replay.Units().empty()) continue;
        EnemyUnit copy = replay.Units().front();
        const int col = cell % columns, row = cell / columns;
        copy.feet = {col * 200.0F + 100.0F, row * 200.0F + 160.0F};
        renderer.DrawUnit(copy, font);
        DrawLine(static_cast<int>(copy.feet.x), row * 200, static_cast<int>(copy.feet.x),
                 row * 200 + 200, Color{255, 96, 96, 110});
        ++cell;
    }
    EndTextureMode();
    Image image = LoadImageFromTexture(sheet.texture);
    ImageFlipVertical(&image);
    const std::string name =
        std::string(Utf8Path(GetApplicationDirectory()).string()) + "/bash-trace-" + title + ".png";
    ExportImage(image, name.c_str());
    UnloadImage(image);
    UnloadRenderTexture(sheet);
    std::printf("  montage: %s\n", name.c_str());
}

}  // namespace

int main() {
    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "bash trace");
    if (!IsWindowReady()) return 2;

    Run("player-left", 1960.0F, 1810.0F, 1810.0F, 300);
    Run("player-right", 1960.0F, 2110.0F, 2110.0F, 300);
    Run("player-crosses", 1960.0F, 1860.0F, 2060.0F, 300);

    CloseWindow();
    return 0;
}
