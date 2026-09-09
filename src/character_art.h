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
    Attack,
    Defeated
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
    [[nodiscard]] bool HasSkillIcon(int skillIndex) const;
    void DrawPortrait(Rectangle destination, Color tint = WHITE) const;
    void DrawChibi(Vector2 feetPosition, int facingDirection, float height,
                   ChibiAnimation animation, float animationTime,
                   Color tint = WHITE) const;
    void DrawBattleChibi(Vector2 feetPosition, int facingDirection, float height,
                         BattleChibiAnimation animation, float animationTime,
                         Color tint = WHITE) const;
    void DrawSkillIcon(int skillIndex, Rectangle destination,
                       Color tint = WHITE) const;

private:
    Texture2D portrait_{};
    Texture2D chibi_{};
    Texture2D battleChibi_{};
    Texture2D skillIcons_[2]{};
};
