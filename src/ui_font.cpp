#include "ui_font.h"

#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>

namespace {

constexpr const char* kUiGlyphs =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    " /+-.:……，："
    "按案败避标镖并部查朝车冲出弹当档岛德定动端段二发返方放飞"
    "锋干高格关换挥回或击继键交角接近进君卡开砍看可空量罗名命"
    "目能前枪确认任容入色闪尚射生失使始弑署术速锁天跳停统突退"
    "卫未务袭戏系匣向新行续选页一移已游员暂择斩战者中终重主住作"
    "扫模式过载冷却就绪效德克萨斯剑气阵雨连绵跳跃攻击技拖入角色导入刷新成功本地保存失败配置文件";

constexpr int kFontAtlasSize = 128;

const char* FindFontPath() {
    static const std::string bundledPath =
        (std::filesystem::path(GetApplicationDirectory()) /
         "assets/fonts/ui.ttf").string();
    if (FileExists(bundledPath.c_str())) {
        return bundledPath.c_str();
    }
#ifdef _WIN32
    if (FileExists("C:/Windows/Fonts/msyh.ttf")) {
        return "C:/Windows/Fonts/msyh.ttf";
    }
    if (FileExists("C:/Windows/Fonts/simhei.ttf")) {
        return "C:/Windows/Fonts/simhei.ttf";
    }
    // stb_truetype uses a variable font's default axis, which can be too thin at HUD sizes.
    if (FileExists("C:/Windows/Fonts/NotoSansSC-VF.ttf")) {
        return "C:/Windows/Fonts/NotoSansSC-VF.ttf";
    }
#endif
    return nullptr;
}

}  // namespace

UiFont::UiFont() : font_(GetFontDefault()) {
    SetAdditionalText("");
}

void UiFont::SetAdditionalText(const std::string& text) {
    const char* fontPath = FindFontPath();
    if (fontPath == nullptr) {
        return;
    }

    int glyphCount = 0;
    const std::string glyphs = std::string(kUiGlyphs) + text;
    int* codepoints = LoadCodepoints(glyphs.c_str(), &glyphCount);
    std::vector<int> unique(codepoints, codepoints + glyphCount);
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    const Font loadedFont =
        LoadFontEx(fontPath, kFontAtlasSize, unique.data(), static_cast<int>(unique.size()));
    UnloadCodepoints(codepoints);

    if (loadedFont.texture.id != 0 &&
        loadedFont.texture.id != GetFontDefault().texture.id) {
        if (ownsFont_) UnloadFont(font_);
        font_ = loadedFont;
        ownsFont_ = true;
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
    }
}

UiFont::~UiFont() {
    if (ownsFont_) {
        UnloadFont(font_);
    }
}

void UiFont::Draw(const char* text, float x, float y, float size, Color color) const {
    DrawTextEx(font_, text, {x, y}, size, 1.0F, color);
}

float UiFont::Measure(const char* text, float size) const {
    return MeasureTextEx(font_, text, size, 1.0F).x;
}
