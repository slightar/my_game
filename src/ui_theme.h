#pragma once

#include "raylib.h"

#include <array>
#include <algorithm>

namespace TacticalUi {

inline constexpr Color kInk{20, 23, 27, 255};
inline constexpr Color kPanel{27, 31, 36, 238};
inline constexpr Color kPanelSoft{38, 43, 49, 218};
inline constexpr Color kPaper{235, 238, 236, 255};
inline constexpr Color kMuted{151, 160, 166, 255};
inline constexpr Color kCyan{83, 205, 220, 255};
inline constexpr Color kOrange{246, 112, 32, 255};
inline constexpr Color kRed{196, 46, 54, 255};

inline std::array<Vector2, 6> CutCorners(Rectangle bounds, float cut) {
    return {{{bounds.x + cut, bounds.y},
             {bounds.x + bounds.width, bounds.y},
             {bounds.x + bounds.width, bounds.y + bounds.height - cut},
             {bounds.x + bounds.width - cut, bounds.y + bounds.height},
             {bounds.x, bounds.y + bounds.height},
             {bounds.x, bounds.y + cut}}};
}

inline void DrawCutPanel(Rectangle bounds, Color fill,
                         Color outline = BLANK, float cut = 14.0F,
                         float thickness = 1.0F) {
    auto points = CutCorners(bounds, cut);
    // raylib requires counter-clockwise winding for filled triangles.
    std::reverse(points.begin(), points.end());
    DrawTriangleFan(points.data(), static_cast<int>(points.size()), fill);
    if (outline.a == 0) return;
    for (std::size_t i = 0; i < points.size(); ++i) {
        DrawLineEx(points[i], points[(i + 1) % points.size()], thickness,
                   outline);
    }
}

inline void DrawSectionLabel(Rectangle bounds, Color accent = kCyan) {
    DrawRectangleRec({bounds.x, bounds.y, 5.0F, bounds.height}, accent);
    DrawRectangleRec({bounds.x + 11.0F, bounds.y, 42.0F, 2.0F}, accent);
}

inline void DrawProgressLine(Vector2 start, float width, float ratio,
                             Color color, float thickness = 6.0F) {
    const float value = ratio < 0.0F ? 0.0F : ratio > 1.0F ? 1.0F : ratio;
    DrawLineEx(start, {start.x + width, start.y}, thickness,
               Color{65, 71, 77, 230});
    DrawLineEx(start, {start.x + width * value, start.y}, thickness, color);
    DrawCircleV(start, thickness * 0.5F, color);
}

inline void DrawCornerMarks(Rectangle bounds, Color color) {
    constexpr float length = 13.0F;
    constexpr float thickness = 2.0F;
    DrawLineEx({bounds.x, bounds.y}, {bounds.x + length, bounds.y}, thickness,
               color);
    DrawLineEx({bounds.x, bounds.y}, {bounds.x, bounds.y + length}, thickness,
               color);
    DrawLineEx({bounds.x + bounds.width, bounds.y + bounds.height},
               {bounds.x + bounds.width - length, bounds.y + bounds.height},
               thickness, color);
    DrawLineEx({bounds.x + bounds.width, bounds.y + bounds.height},
               {bounds.x + bounds.width, bounds.y + bounds.height - length},
               thickness, color);
}

}  // namespace TacticalUi
