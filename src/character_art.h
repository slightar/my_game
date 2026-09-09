#pragma once

#include "raylib.h"

enum class ChibiAnimation {
    Idle,
    Run,
    Jump,
    Dodge,
    Wave
};

enum class BattleChibiAnimation {
    Idle,
    Attack
};

class CharacterArt {
public:
    CharacterArt();
    ~CharacterArt();

    CharacterArt(const CharacterArt&) = delete;
    CharacterArt& operator=(const CharacterArt&) = delete;

    [[nodiscard]] bool HasPortrait() const;
    [[nodiscard]] bool HasChibi() const;
    [[nodiscard]] bool HasBattleChibi() const;
    void DrawPortrait(Rectangle destination, Color tint = WHITE) const;
    void DrawChibi(Vector2 feetPosition, int facingDirection, float height,
                   ChibiAnimation animation, float animationTime,
                   Color tint = WHITE) const;
    void DrawBattleChibi(Vector2 feetPosition, int facingDirection, float height,
                         BattleChibiAnimation animation, float animationTime,
                         Color tint = WHITE, float rotation = 0.0F,
                         float horizontalScale = 1.0F,
                         float verticalScale = 1.0F) const;

private:
    Texture2D portrait_{};
    Texture2D chibi_{};
    Texture2D battleChibi_{};
};
