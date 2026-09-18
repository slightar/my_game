#pragma once

#include "raylib.h"
#include "ui_assets.h"
#include <string>

class UiFont {
public:
    UiFont();
    ~UiFont();

    UiFont(const UiFont&) = delete;
    UiFont& operator=(const UiFont&) = delete;
    void SetAdditionalText(const std::string& text);

    void Draw(const char* text, float x, float y, float size, Color color) const;
    [[nodiscard]] float Measure(const char* text, float size) const;
    [[nodiscard]] const UiAssets& Skin() const { return skin_; }

private:
    UiAssets skin_;
    Font font_{};
    bool ownsFont_ = false;
};
