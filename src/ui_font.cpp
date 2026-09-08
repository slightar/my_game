#include "ui_font.h"

namespace {

constexpr const char* kUiGlyphs =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
    " /+-.:……"
    "移动空格二段跳按住射击并锁定朝向换弹闪避生命冲锋枪中"
    "任务失败弑君者已击败回车键重新开始游戏挥砍突袭空中斩飞镖";

const char* FindFontPath() {
    if (FileExists("assets/fonts/ui.ttf")) {
        return "assets/fonts/ui.ttf";
    }
#ifdef _WIN32
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
    const Font loadedFont = LoadFontEx(fontPath, 64, codepoints, glyphCount);
    UnloadCodepoints(codepoints);

    if (loadedFont.texture.id != 0 &&
        loadedFont.texture.id != GetFontDefault().texture.id) {
        font_ = loadedFont;
        ownsFont_ = true;
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
