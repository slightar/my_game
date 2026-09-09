#include "ui_font.h"

#include <filesystem>
#include <string>

namespace {

constexpr const char* kUiGlyphs =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    " /+-.:……，："
    "按案败避标镖并部查朝车冲出弹当档岛德定动端段二发返方放飞"
    "锋干高格关换挥回或击继键交角接近进君卡开砍看可空量罗名命"
    "目能前枪确认任容入色闪尚射生失使始弑署术速锁天跳停统突退"
    "卫未务袭戏系匣向新行续选页一移已游员暂择斩战者中终重主住作"
    "扫模式过载冷却就绪效德克萨斯剑气阵雨连绵";

constexpr int kFontAtlasSize = 128;

const char* FindFontPath() {
    static const std::string bundledPath =
        (std::filesystem::path(GetApplicationDirectory()) /
         "assets/fonts/ui.ttf").string();
    if (FileExists(bundledPath.c_str())) {
        return bundledPath.c_str();
    }
#ifdef _WIN32
    if (FileExists("C:/Windows/Fonts/NotoSansSC-VF.ttf")) {
        return "C:/Windows/Fonts/NotoSansSC-VF.ttf";
    }
    if (FileExists("C:/Windows/Fonts/msyh.ttf")) {
        return "C:/Windows/Fonts/msyh.ttf";
    }
    if (FileExists("C:/Windows/Fonts/simhei.ttf")) {
        return "C:/Windows/Fonts/simhei.ttf";
    }
#endif
    return nullptr;
}

}  // namespace

UiFont::UiFont() : font_(GetFontDefault()) {
    const char* fontPath = FindFontPath();
    if (fontPath == nullptr) {
        return;
    }

    int glyphCount = 0;
    int* codepoints = LoadCodepoints(kUiGlyphs, &glyphCount);
    const Font loadedFont =
        LoadFontEx(fontPath, kFontAtlasSize, codepoints, glyphCount);
    UnloadCodepoints(codepoints);

    if (loadedFont.texture.id != 0 &&
        loadedFont.texture.id != GetFontDefault().texture.id) {
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
