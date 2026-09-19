#pragma once

#include "game_types.h"
#include "raylib.h"
#include <algorithm>

struct UiPointer {
    Vector2 position{};
    bool pressed = false;
    bool released = false;
    bool down = false;
    bool valid = false;

    bool Hit(Rectangle bounds) const {
        return valid && position.x >= bounds.x && position.x <= bounds.x + bounds.width &&
               position.y >= bounds.y && position.y <= bounds.y + bounds.height;
    }
    // A click is committed on release, after the pointer has remained over the control.
    bool Clicked(Rectangle bounds) const { return released && Hit(bounds); }
};

inline UiPointer MakeUiPointer(Vector2 screenPosition, bool pressed, bool down, bool released,
                               int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) return {};
    const float scale = std::min(static_cast<float>(windowWidth) / GameConfig::kScreenWidth,
                                 static_cast<float>(windowHeight) / GameConfig::kScreenHeight);
    const float offsetX = (windowWidth - GameConfig::kScreenWidth * scale) * 0.5F;
    const float offsetY = (windowHeight - GameConfig::kScreenHeight * scale) * 0.5F;
    const Vector2 logical{(screenPosition.x - offsetX) / scale,
                          (screenPosition.y - offsetY) / scale};
    const bool valid = logical.x >= 0.0F && logical.x < GameConfig::kScreenWidth &&
                       logical.y >= 0.0F && logical.y < GameConfig::kScreenHeight;
    return {logical, pressed && valid, down && valid, released && valid, valid};
}

inline UiPointer ReadUiPointer(bool& touchPreviouslyDown) {
    const bool touchDown = GetTouchPointCount() > 0;
    const bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool active = touchDown || mouseDown;
    const Vector2 screenPosition = touchDown ? GetTouchPosition(0) : GetMousePosition();
    const bool pressed = active && !touchPreviouslyDown;
    const bool released = !active && touchPreviouslyDown;
    touchPreviouslyDown = active;
    return MakeUiPointer(screenPosition, pressed, active, released, GetScreenWidth(), GetScreenHeight());
}
