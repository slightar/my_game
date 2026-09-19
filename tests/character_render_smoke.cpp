#include "character_art.h"
#include "character_repository.h"
#include "file_path.h"
#include "main_menu.h"
#include "player.h"
#include "ui_font.h"
#include "menu_render_checks.h"
#include "enemy_render_checks.h"
#include <iostream>
#include <fstream>

// Manual GPU smoke test; excluded from CTest so unit tests need no display server.
int main() {
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1280, 720, "Character render smoke test");
    if (!IsWindowReady()) return 1;
    int result = 0;
    try {
        const auto app = Utf8Path(GetApplicationDirectory());
        LocalCharacterRepository repository(app / "assets" / "characters");
        const auto list = repository.LoadAll();
        if (!list.errors.empty() || list.characters.empty()) throw std::runtime_error("Missing/invalid bundled character");
        CharacterArt art;
        UiFont font;
        std::string text;
        for (const auto& c : list.characters) {
            art.RegisterCharacter(c, repository.Root()); text += c.name + c.description;
            for (const auto& s : c.skills) text += s.name + s.description;
        }
        font.SetAdditionalText(text);
        MainMenu menu; menu.OpenHome();
        Player player; player.Reset(list.characters.front());
        const auto output = app / "character-qa"; std::filesystem::create_directories(output);
        const auto render = [&](const char* filename, auto draw) {
            auto target = LoadRenderTexture(1280, 720);
            if (!target.id) throw std::runtime_error("Render target allocation failed");
            BeginTextureMode(target); ClearBackground(Color{28, 33, 41, 255}); draw(); EndTextureMode();
            Image image = LoadImageFromTexture(target.texture); ImageFlipVertical(&image);
            const bool saved = ExportImage(image, (output / filename).string().c_str());
            UnloadImage(image); UnloadRenderTexture(target);
            if (!saved) throw std::runtime_error("Cannot export render");
        };
        render("menu.png", [&] { menu.Draw(font, art); });
        CheckEnemyRendering(font, render);
        menu.OpenHome();
        render("home.png", [&] { menu.Draw(font, art); });
        menu.OpenStageSelect();
        render("stage.png", [&] { menu.Draw(font, art); });
        menu.OpenArchive();
        render("archive.png", [&] { menu.Draw(font, art); });
        menu.OpenEquipment();
        render("equipment.png", [&] { menu.Draw(font, art); });
        menu.OpenSettings();
        render("settings.png", [&] { menu.Draw(font, art); });
        CheckMenuInteraction(output, [&](const char* name, const MainMenu& subject) {
            render(name, [&] { subject.Draw(font, art); });
        });
        Player builtin;
        builtin.Reset();
        render("exusiai-hud.png", [&] {
            builtin.Draw(art); builtin.DrawHud(font, art, "能天使");
            art.DrawFacingRing({500, 644}, -1);
        });
        render("battle.png", [&] {
            DrawRectangle(48, 644, 1184, 50, DARKGRAY);
            player.Draw(art); player.DrawHud(font, art, "");
        });
        auto guard = list.characters.front();
        // Simulate a previously imported definition with no frame metadata.
        guard.assets.rowFrameCounts.clear();
        Image guardImage = LoadCharacterImage(repository.Root() / Utf8Path(guard.assets.sprite));
        auto guardFrames = FindCharacterFrames(guardImage, guard.assets); UnloadImage(guardImage);
        if (guardFrames.at(0).size() != 7 || guardFrames.at(3).size() != 4)
            throw std::runtime_error("Regression: demo blank cells included in animation");
        art.RegisterCharacter(guard, repository.Root());
        render("idle-loop-regression.png", [&] {
            font.Draw("DEMO IDLE / TWO COMPLETE LOOPS", 30, 30, 24, SKYBLUE);
            for (int i = 0; i < 16; ++i) {
                const Vector2 feet{90 + (i % 8) * 154.0F, 260 + (i / 8) * 260.0F};
                const float time = i / guard.assets.fps;
                art.DrawCustomSprite(guard, feet, 1, 0, time);
                font.Draw(TextFormat("t=%.3f", time), feet.x - 40, feet.y + 20, 16, LIGHTGRAY);
            }
        });
        // Check actual GPU output for every time slot on both menu and in-battle paths.
        for (bool menuPath : {false, true}) {
            CharacterAnimator idle;
            for (int i = 0; i < 120; ++i) {
                idle.Update(1.0F / 60, {});
                auto frame = LoadRenderTexture(192, 192);
                BeginTextureMode(frame); ClearBackground(BLANK);
                if (menuPath) art.DrawCustomSprite(guard, {96, 180}, 1, 0, i / 60.0F);
                else art.DrawAnimatedCharacter(guard, {96, 180}, 1, idle);
                EndTextureMode();
                Image pixels = LoadImageFromTexture(frame.texture);
                const Rectangle bounds = GetImageAlphaBorder(pixels, 0.1F);
                UnloadImage(pixels); UnloadRenderTexture(frame);
                if (bounds.width < 20 || bounds.height < 50) throw std::runtime_error("Regression: character disappears in idle loop");
            }
        }
        render("exusiai-walk-regression.png", [&] {
            font.Draw("EXUSIAI / WALK / MOVING FIRE / JUMP + DODGE", 30, 24, 22, SKYBLUE);
            for (int i = 0; i < 8; ++i) {
                const float x = 90 + i * 154.0F;
                art.DrawExusiai({x, 205}, 1, ChibiAnimation::Run, false, false, i / 12.0F, 0, 0);
                art.DrawExusiai({x, 395}, -1, ChibiAnimation::Run, true, false, i / 12.0F, i / 12.0F, 0);
                art.DrawExusiai({x, 585}, 1, i < 4 ? ChibiAnimation::Jump : ChibiAnimation::Dodge, false, false, i / 12.0F, 0, 0);
            }
        });
        // Diagnostic modular puppet built from code-native shapes, not segmented character art.
        const auto partsDir = output / "rig_parts"; std::filesystem::create_directories(partsDir);
        const auto partImage = [&](const char* name, int w, int h, Color color) {
            Image image = GenImageColor(w, h, BLANK);
            ImageDrawRectangle(&image, 2, 2, w - 4, h - 4, color);
            ImageDrawRectangle(&image, 4, 4, std::max(1, w / 5), h - 8, Fade(WHITE, 0.3F));
            if (std::string(name) == "head") {
                ImageDrawCircle(&image, w / 3, h / 2, 3, DARKBLUE);
                ImageDrawCircle(&image, w * 2 / 3, h / 2, 3, DARKBLUE);
            }
            const bool saved = ExportImage(image, (partsDir / (std::string(name) + ".png")).string().c_str());
            UnloadImage(image); if (!saved) throw std::runtime_error("Cannot write rig fixture");
        };
        partImage("head", 64, 56, Color{240, 195, 140, 255});
        partImage("torso", 52, 64, SKYBLUE);
        partImage("armFront", 18, 58, SKYBLUE); partImage("armBack", 18, 58, BLUE);
        partImage("legFront", 22, 60, Color{96, 114, 142, 255}); partImage("legBack", 22, 60, DARKBLUE);
        partImage("weapon", 14, 72, LIGHTGRAY);
        Character rig; rig.id = "rig_demo"; rig.name = "Modular demo"; rig.assets.height = 138;
        rig.attack.type = EffectType::Melee;
        for (const char* slot : {"torso", "head", "armFront", "armBack", "legFront", "legBack", "weapon"}) {
            auto p = DefaultCharacterPart(slot); p.image = std::string("rig_parts/") + slot + ".png";
            rig.assets.parts[slot] = p;
        }
        rig.Validate(); art.RegisterCharacter(rig, output);
        { std::ofstream out(output / "rig_character.json"); out << rig.ToJson(); }
        // Compose a single neutral sprite; the same defaults then animate that flat image.
        auto flatTarget = LoadRenderTexture(180, 160);
        BeginTextureMode(flatTarget); ClearBackground(BLANK);
        CharacterAnimator neutral; art.DrawAnimatedCharacter(rig, {90, 150}, 1, neutral);
        EndTextureMode(); Image flatImage = LoadImageFromTexture(flatTarget.texture); ImageFlipVertical(&flatImage);
        ImageAlphaCrop(&flatImage, 0.05F);
        const bool flatSaved = ExportImage(flatImage, (output / "single_sprite.png").string().c_str());
        UnloadImage(flatImage); UnloadRenderTexture(flatTarget);
        if (!flatSaved) throw std::runtime_error("Cannot write single-sprite fixture");
        Character single; single.id = "single_demo"; single.name = "Single sprite demo";
        single.assets.sprite = "single_sprite.png"; single.assets.height = 138; single.attack.type = EffectType::Melee;
        art.RegisterCharacter(single, output);
        { std::ofstream out(output / "single_character.json"); out << single.ToJson(); }
        const auto animated = [](int state) {
            CharacterAnimator a;
            CharacterMotion m;
            if (state == 1) m.velocity.x = 320;
            if (state == 2 || state == 3) { m.grounded = false; m.velocity.y = state == 2 ? -400.0F : 300.0F; }
            if (state == 4) { m.velocity.x = 220; m.melee = true; }
            if (state == 5) m.dodging = true;
            if (state == 6) a.TriggerHurt();
            if (state == 7) m.dead = true;
            const int steps = state == 7 ? 60 : 8;
            for (int j = 0; j < steps; ++j) { m.attackTriggered = state == 4 && j == 0; a.Update(1.0F / 60, m); }
            return a;
        };
        render("default-actions.png", [&] {
            const char* names[] = {"IDLE", "WALK", "JUMP", "FALL", "ATTACK", "DODGE", "HURT", "DEFEAT"};
            font.Draw("DEFAULT ACTIONS / SINGLE IMAGE", 30, 30, 26, SKYBLUE);
            font.Draw("MODULAR PARTS / SAME CONTROLLER", 30, 356, 26, SKYBLUE);
            for (int state = 0; state < 8; ++state) {
                const float x = 76 + state * 153.0F - (state == 7 ? 30 : 0);
                auto animator = animated(state);
                for (float y : {244.0F, 567.0F}) DrawLine(static_cast<int>(x - 50), static_cast<int>(y), static_cast<int>(x + 65), static_cast<int>(y), GRAY);
                art.DrawAnimatedCharacter(single, {x, 244}, 1, animator);
                art.DrawAnimatedCharacter(rig, {x, 567}, 1, animator);
                font.Draw(names[state], x - 36, 292, 17, LIGHTGRAY);
                font.Draw(names[state], x - 36, 615, 17, LIGHTGRAY);
            }
        });
        render("mirrored-actions.png", [&] {
            font.Draw("LEFT FACING / MODULAR JOINTS", 40, 40, 26, SKYBLUE);
            for (int state = 0; state < 7; ++state) {
                auto animator = animated(state);
                art.DrawAnimatedCharacter(rig, {110 + state * 170.0F, 350}, -1, animator);
            }
        });
        std::cout << "Character render smoke test passed: " << output << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; result = 1; }
    CloseWindow(); return result;
}
