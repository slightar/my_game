#include "game.h"
#include "rlgl.h"
#include "boss_client_fx.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#ifndef PERFORMANCE_BASELINE
struct UiFontTestAccess {
    static Font Native(const UiFont& font) { return font.font_; }
};
struct BossClientFxTestAccess {
    static std::weak_ptr<void> Owner(const BossClientFx& fx) { return fx.resources_; }
    static const auto& Textures(const BossClientFx& fx) { return fx.textures_; }
};
#endif
struct WorldPreviewTestAccess {
    static UiFont& Font(Game& game) { return game.uiFont_; }
    static void Home(Game& game) { game.mainMenu_.OpenHome(); }
    static void Map(Game& game) { game.mainMenu_.OpenStageSelect(); }
    static void Boss(Game& game, int index) {
        game.EnterDeveloperRoom();
        auto& lab = game.developerRoom_;
        lab.Mark(lab.Width() * .62F);
        lab.Select(int(kEnemyKindCount) + index);
        if (!lab.SpawnSelected(game.player_->Position())) throw std::runtime_error("Boss spawn failed");
        for (int i = 0; i < 100; ++i) game.Update(1.0F / 60);
    }
};
namespace {
using Clock = std::chrono::steady_clock;
double Milliseconds(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}
#ifndef PERFORMANCE_BASELINE
void VerifyFont(const UiFont& font, const std::filesystem::path& output) {
    const Font native = UiFontTestAccess::Native(font);
    const std::vector<const char*> texts{
        "", "HP 3.75/8", "弑君者 · 镜影交刃", "霜星冰晶锁定散射击碎或换位",
        "W · 遥控雷网", "冰坑 HP 4/4", "长枪沿固定方向贯穿",
        "前行\n跳跃避开\n离开红圈", "WIDE LINE\nx", "x\nWIDE LINE", "\n", "\n\n",
        " trailing  ", "A\tB", "未知字形 🐉", "坏字符 \xff\xc0"};
    auto reference = LoadRenderTexture(1280, 720);
    auto actual = LoadRenderTexture(1280, 720);
    int checks = 0;
    for (int spacing : {0, 2, 11}) {
        SetTextLineSpacing(spacing);
        for (float size : {12.0F, 18.0F, 26.5F, 48.0F}) {
            for (const char* text : texts) {
                const float expectedWidth = MeasureTextEx(native, text, size, 1).x;
                if (expectedWidth != font.Measure(text, size))
                    throw std::runtime_error(std::string("Font width mismatch: ") + text);
                int count = 0;
                int* codepoints = LoadCodepoints(text, &count);
                bool supported = true;
                for (int i = 0; i < count; ++i) {
                    const int index = GetGlyphIndex(native, codepoints[i]);
                    if (index < 0 || index >= native.glyphCount || native.glyphs[index].value != codepoints[i])
                        supported = false;
                }
                UnloadCodepoints(codepoints);
                if (supported != font.SupportsText(text)) throw std::runtime_error("Font support mismatch");
                BeginTextureMode(reference); ClearBackground(BLACK);
                DrawTextEx(native, text, {13.25F, 14.5F}, size, 1, Color{165, 213, 241, 179});
                EndTextureMode();
                BeginTextureMode(actual); ClearBackground(BLACK);
                font.Draw(text, 13.25F, 14.5F, size, Color{165, 213, 241, 179});
                EndTextureMode();
                Image before = LoadImageFromTexture(reference.texture);
                Image after = LoadImageFromTexture(actual.texture);
                const int bytes = GetPixelDataSize(before.width, before.height, before.format);
                const bool equal = before.format == after.format && std::memcmp(before.data, after.data, bytes) == 0;
                if (!equal) {
                    ExportImage(before, (output / "font-reference.png").string().c_str());
                    ExportImage(after, (output / "font-actual.png").string().c_str());
                }
                UnloadImage(before); UnloadImage(after);
                if (!equal) throw std::runtime_error(std::string("Font pixels mismatch: ") + text);
                ++checks;
            }
        }
    }
    SetTextLineSpacing(2);
    UnloadRenderTexture(reference); UnloadRenderTexture(actual);
    std::cout << "FONT_REFERENCE_CHECKS=" << checks << " exact widths and pixel buffers passed\n";
}
void VerifyFontReload() {
    UiFont font(true);
    font.SetAdditionalText("AAB");
    const auto first = UiFontTestAccess::Native(font).texture.id;
    font.SetAdditionalText("BA");
    if (UiFontTestAccess::Native(font).texture.id != first)
        throw std::runtime_error("Unchanged glyph set rebuilt the atlas");
    font.SetAdditionalText("BA龘");
    if (!font.SupportsText("龘")) throw std::runtime_error("New custom glyph was not loaded");
    std::cout << "FONT_RELOAD_CHECKS=passed\n";
}
void VerifyResourceLifetime() {
    auto first = std::make_unique<BossClientFx>();
    std::weak_ptr<void> owner = BossClientFxTestAccess::Owner(*first);
    auto survivor = std::make_unique<BossClientFx>();
    if (owner.lock() != BossClientFxTestAccess::Owner(*survivor).lock())
        throw std::runtime_error("Boss renderers loaded duplicate resources");
    std::size_t bytes = 0;
    for (const auto& [name, texture] : BossClientFxTestAccess::Textures(*first))
        bytes += GetPixelDataSize(texture.width, texture.height, texture.format);
    const auto texture = BossClientFxTestAccess::Textures(*first).at("crown_dart");
    Image before = LoadImageFromTexture(texture);
    first.reset();
    if (!survivor->Ready() || !survivor->ReadyFor(StoryBossKind::Crownslayer) || !survivor->ReadyFor(StoryBossKind::W))
        throw std::runtime_error("Releasing one renderer invalidated its survivor");
    Image after = LoadImageFromTexture(texture);
    const bool equal = IsImageValid(before) && IsImageValid(after) && before.format == after.format &&
        std::memcmp(before.data, after.data, GetPixelDataSize(before.width, before.height, before.format)) == 0;
    UnloadImage(before); UnloadImage(after);
    if (!equal) throw std::runtime_error("Shared Boss texture was released too early");
    survivor.reset();
    if (!owner.expired()) throw std::runtime_error("Boss resources leaked after the last renderer");
    { BossClientFx next; if (!next.Ready()) throw std::runtime_error("Boss resources failed to reload"); }
    std::cout << "FX_RESOURCE_LIFETIME=passed SAVED_TEXTURE_BYTES=" << bytes << '\n';
}
#endif
void MeasureScene(Game& game, const char* name, const std::filesystem::path& output) {
    std::vector<double> times;
    for (int i = 0; i < 140; ++i) {
        BeginDrawing(); ClearBackground(BLACK);
        const auto start = Clock::now();
        game.Update(1.0F / 60);
        game.Draw(1, {});
        rlDrawRenderBatchActive();
        const double time = Milliseconds(start);
        EndDrawing();
        if (i >= 20) times.push_back(time);
    }
    std::sort(times.begin(), times.end());
    std::cout << "FRAME_CPU_MS " << name << " median=" << times[times.size()/2]
              << " p95=" << times[times.size()*95/100] << '\n';
    // TakeScreenshot discards directories in raylib 5.5. Export explicitly
    // so audits never leave generated files in the user's project root.
    auto target = LoadRenderTexture(1280, 720);
    BeginTextureMode(target); ClearBackground(BLACK); game.Draw(1, {}); EndTextureMode();
    Image image = LoadImageFromTexture(target.texture); ImageFlipVertical(&image);
    const bool saved = ExportImage(image, (output / (std::string(name) + ".png")).string().c_str());
    UnloadImage(image); UnloadRenderTexture(target);
    if (!saved) throw std::runtime_error("Audit screenshot failed");
}
}
int main(int argc, char** argv) {
    const std::filesystem::path output = argc > 1 ? argv[1] : "output/performance";
    std::filesystem::create_directories(output);
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "Performance audit");
    InitAudioDevice(); SetMasterVolume(0);
    // Match production pacing, leaving the GPU time to finish each frame so
    // queue backpressure does not distort application-side CPU measurements.
    SetTargetFPS(60);
    int result = 0;
    try {
        const auto start = Clock::now();
        Game game;
        std::cout << "GAME_STARTUP_MS=" << Milliseconds(start) << '\n';
#ifndef PERFORMANCE_BASELINE
        if (argc > 2 && std::string_view(argv[2]) == "--verify-font") {
            VerifyFont(WorldPreviewTestAccess::Font(game), output);
            VerifyFontReload();
        }
#endif
        WorldPreviewTestAccess::Home(game); MeasureScene(game, "home", output);
        WorldPreviewTestAccess::Map(game); MeasureScene(game, "map", output);
        for (int i = 0; i < kStoryBossCount; ++i) {
            WorldPreviewTestAccess::Boss(game, i);
            MeasureScene(game, StoryBossId(static_cast<StoryBossKind>(i+1)), output);
        }
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; result = 1; }
#ifndef PERFORMANCE_BASELINE
    if (result == 0 && argc > 2 && std::string_view(argv[2]) == "--verify-font") {
        try { VerifyResourceLifetime(); }
        catch (const std::exception& e) { std::cerr << e.what() << '\n'; result = 1; }
    }
#endif
    CloseAudioDevice(); CloseWindow();
    return result;
}
