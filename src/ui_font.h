#pragma once

#include "raylib.h"
#include "ui_assets.h"
#include <string>
#include <unordered_map>
#include <vector>

class UiFont {
public:
    explicit UiFont(bool deferFontLoad = false);
    ~UiFont();

    UiFont(const UiFont&) = delete;
    UiFont& operator=(const UiFont&) = delete;
    void SetAdditionalText(const std::string& text);

    void Draw(const char* text, float x, float y, float size, Color color) const;
    [[nodiscard]] float Measure(const char* text, float size) const;
    [[nodiscard]] bool SupportsText(const std::string& text) const;
    [[nodiscard]] const UiAssets& Skin() const { return skin_; }

private:
    friend struct UiFontTestAccess;
    UiAssets skin_;
    Font font_{};
    bool ownsFont_ = false;
    std::vector<int> loadedCodepoints_;
    std::unordered_map<int, int> glyphIndices_;
    int fallbackGlyph_ = 0;
    void IndexGlyphs();
    [[nodiscard]] int GlyphIndex(int codepoint) const;
};
