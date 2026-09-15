#pragma once

#include "raylib.h"
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

private:
    Font font_{};
    bool ownsFont_ = false;
};
