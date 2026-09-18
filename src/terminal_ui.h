#pragma once
#include "raylib.h"
#include <string>
class UiFont;

// Menu-only design tokens. Battle HUD keeps its existing TacticalUi theme.
namespace TerminalUi {
inline constexpr Color Ink{28, 31, 34, 255};
inline constexpr Color Paper{239, 240, 237, 255};
inline constexpr Color Muted{157, 166, 172, 255};
inline constexpr Color Blue{0, 153, 204, 255};
inline constexpr Color Orange{221, 112, 45, 255};
inline constexpr Color Warm{190, 144, 129, 255};
inline constexpr Rectangle Back{28, 22, 98, 48};
inline constexpr Rectangle HomeButtons[] = {
    {720, 157, 492, 147}, {720, 407, 238, 99}, {974, 407, 238, 99},
    {720, 522, 238, 66}, {720, 320, 492, 71}, {974, 522, 238, 66}};
inline constexpr int HomeOrder[]{0, 4, 1, 2, 3, 5};
inline constexpr Rectangle MapArea{85, 184, 1110, 412};
inline constexpr Rectangle MapTask{85, 602, 440, 34};
inline constexpr Rectangle Confirm{790, 545, 385, 52};
inline constexpr Rectangle Equip{946, 554, 247, 43};
inline constexpr Rectangle Unequip{655, 554, 270, 43};
inline constexpr Rectangle ResetConfirm{737, 438, 310, 57};
inline constexpr Rectangle Category(int i) { return {56, 150.0F + i * 74, 243, 62}; }
void Background(const UiFont& font, const char* title, const char* subtitle, bool back = true);
void Surface(Rectangle rect, Color fill = Ink, Color stripe = BLANK);
void Button(const UiFont& font, Rectangle rect, const char* title, bool selected = false,
            Color fill = Paper, Color text = Ink);
void Footer(const UiFont& font, const char* text);
void Fit(const UiFont& font, const std::string& text, Rectangle rect, float size, Color color);
void Wrap(const UiFont& font, const std::string& text, Rectangle rect, float size, Color color);
void Emblem(Vector2 center, float radius, int kind, Color color);
}
