#include "terminal_ui.h"
#include "ui_font.h"
#include <algorithm>
#include <cmath>

namespace TerminalUi {
namespace {
UiPointer pointerState;
// Single shared veil so every control reacts to a press with the same gray.
constexpr Color kPressedVeil{118, 126, 134, 170};
}
void SetPointerState(const UiPointer& pointer) { pointerState=pointer; }
bool Pressed(Rectangle rect) {
    return pointerState.Held(rect);
}
Rectangle PressedRect(Rectangle rect) {
    return Pressed(rect) ? Rectangle{rect.x, rect.y + 5.0F, rect.width, rect.height} : rect;
}
void PressedVeil(Rectangle rect) {
    if (Pressed(rect)) DrawRectangleRec({rect.x, rect.y + 5.0F, rect.width, rect.height}, kPressedVeil);
}
void Surface(Rectangle r, Color fill, Color stripe) {
    DrawRectangleRec({r.x + 4, r.y + 5, r.width, r.height}, Fade(BLACK, .17F));
    DrawRectangleRec(r, fill);
    if (stripe.a) DrawRectangleRec({r.x, r.y, 4, r.height}, stripe);
}
void Fit(const UiFont& font, const std::string& text, Rectangle r, float size, Color color) {
    const float width = font.Measure(text.c_str(), size);
    if (width > r.width) size *= r.width / width;
    font.Draw(text.c_str(), r.x, r.y + (r.height - size) / 2, size, color);
}
void Wrap(const UiFont& font, const std::string& text, Rectangle r, float size, Color color) {
    std::string line;
    float y = r.y;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char lead = text[i];
        const auto length = std::min<std::size_t>(lead < 128 ? 1 : lead < 224 ? 2 : lead < 240 ? 3 : 4, text.size() - i);
        const std::string glyph = text.substr(i, length);
        if (glyph == "\n" || (!line.empty() && font.Measure((line + glyph).c_str(), size) > r.width)) {
            if (y + size > r.y + r.height) return;
            font.Draw(line.c_str(), r.x, y, size, color); y += size * 1.65F; line.clear();
        }
        if (glyph != "\n") line += glyph;
        i += length;
    }
    if (y + size <= r.y + r.height) font.Draw(line.c_str(), r.x, y, size, color);
}
void Button(const UiFont& font, Rectangle r, const char* title, bool selected, Color fill, Color text) {
    const Rectangle drawRect=PressedRect(r);
    Surface(drawRect, fill);
    if (selected) {
        DrawRectangleRec({drawRect.x, drawRect.y + drawRect.height - 4, drawRect.width, 4}, Blue);
        DrawRectangleLinesEx({drawRect.x - 3, drawRect.y - 3, drawRect.width + 6, drawRect.height + 6}, 1, Fade(Paper, .85F));
    }
    const float padding=std::min(22.0F,drawRect.width*.18F);
    Fit(font, title, {drawRect.x + padding, drawRect.y, drawRect.width - padding*2, drawRect.height}, 26, text);
    // Pressed state stays purely visual; the action is committed by the caller on release.
    PressedVeil(r);
}
void Background(const UiFont& font, const char* title, const char* subtitle, bool back) {
    ClearBackground({103, 113, 120, 255});
    DrawRectangleGradientV(0, 0, 1280, 720, {149, 158, 161, 255}, {67, 77, 86, 255});
    // Low-contrast architectural planes retain readability without a new background asset.
    DrawTriangle({0, 0}, {0, 720}, {830, 0}, {177, 184, 184, 55});
    DrawTriangle({1280, 720}, {1280, 80}, {500, 720}, {30, 38, 45, 100});
    for (int x = -600; x < 1600; x += 290) DrawLineEx({float(x), 720}, {float(x + 570), 0}, 16, Fade(Paper, .035F));
    font.Skin().Draw("dots", {740, 180, 540, 540}, Fade(WHITE, .065F));
    DrawRectangle(0, 0, 1280, 89, Fade(Ink, .96F));
    if (back) {
        if (!font.Skin().Draw("back", {28, 18, 98, 59})) {
            Surface(Back, Paper);
            font.Draw("<", 62, 26, 32, Ink);
        }
        PressedVeil(Back);
    }
    font.Draw(title, back ? 153 : 42, 20, 31, Paper);
    font.Draw(subtitle, back ? 155 : 44, 58, 11, Muted);
    font.Skin().Draw("rhodes", {1152, 10, 80, 69}, Paper);
    DrawRectangle(0, 89, 1280, 2, Fade(Paper, .25F));
}
void Footer(const UiFont& font, const char* text) {
    DrawRectangle(0, 653, 1280, 67, Fade(Ink, .97F));
    DrawRectangle(32, 666, 4, 22, Blue);
    Fit(font, text, {49, 666, 1173, 24}, 17, Paper);
    font.Draw("RHODES ISLAND  /  DEVELOPMENT BUILD", 49, 698, 10, Muted);
}
void Emblem(Vector2 c, float radius, int kind, Color color) {
    DrawPoly(c, 4, radius, 0, color);
    DrawPoly(c, 4, radius * .69F, 0, Ink);
    if (kind == 0) { // Archive / document.
        for (int i = -1; i <= 1; ++i) DrawLineEx({c.x - radius * .28F, c.y + i * radius * .19F}, {c.x + radius * .28F, c.y + i * radius * .19F}, 3, color);
    } else if (kind == 1) {
        DrawPolyLinesEx(c, 6, radius * .35F, 30, 3, color);
    } else {
        DrawLineEx({c.x - radius * .25F, c.y}, {c.x + radius * .25F, c.y}, 4, color);
        DrawLineEx({c.x, c.y - radius * .25F}, {c.x, c.y + radius * .25F}, 4, color);
    }
}
}
