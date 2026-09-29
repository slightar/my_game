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
    Vector2 pressOrigin{};
    bool captured = false;

    bool Hit(Rectangle bounds) const {
        return valid && position.x >= bounds.x && position.x <= bounds.x + bounds.width &&
               position.y >= bounds.y && position.y <= bounds.y + bounds.height;
    }
    // A click is committed on release, after the pointer has remained over the control.
    bool Held(Rectangle bounds) const { return down && captured && Hit(bounds) && CheckCollisionPointRec(pressOrigin,bounds); }
    bool Clicked(Rectangle bounds) const { return released && captured && Hit(bounds) && CheckCollisionPointRec(pressOrigin,bounds); }
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
    // Assign by name: UiPointer declares released before down, so a positional brace
    // list silently swaps the two and turns Clicked into a "still held" test.
    UiPointer pointer;
    pointer.position = logical;
    pointer.pressed = pressed && valid;
    pointer.released = released && valid;
    pointer.down = down && valid;
    pointer.valid = valid;
    return pointer;
}

// One state per UI owner, no raylib frame-edge polling or field-order assumptions.
struct UiPointerState {
    bool wasDown=false, captured=false, touchWasDown=false;
    Vector2 origin{}, lastTouch{};
    UiPointer Sample(Vector2 position,bool active,int width,int height) {
        auto p=MakeUiPointer(position,active&&!wasDown,active,!active&&wasDown,width,height);
        if(p.pressed) {origin=p.position;captured=p.valid;}
        p.pressOrigin=origin;p.captured=captured;
        if(!active)captured=false;
        wasDown=active;return p;
    }
};
inline UiPointer ReadUiPointer(UiPointerState& state) {
    const bool touchDown = GetTouchPointCount() > 0;
    const bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    const bool active = touchDown || mouseDown;
    if(touchDown)state.lastTouch=GetTouchPosition(0);
    const Vector2 screenPosition = touchDown || state.touchWasDown ? state.lastTouch : GetMousePosition();
    state.touchWasDown=touchDown;
    return state.Sample(screenPosition,active,GetScreenWidth(),GetScreenHeight());
}
