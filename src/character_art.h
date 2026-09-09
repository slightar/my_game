#pragma once

#include "game_types.h"
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
    [[nodiscard]] bool HasPortrait(OperatorKind operatorKind) const;
    [[nodiscard]] bool HasChibi() const;
    [[nodiscard]] bool HasChibi(OperatorKind operatorKind) const;
    [[nodiscard]] bool HasBattleChibi() const;
    [[nodiscard]] bool HasTexasSkill2Battle() const;
    [[nodiscard]] bool HasTexasSkill2Effects() const;
    [[nodiscard]] bool HasSkillIcon(int skillIndex) const;
    [[nodiscard]] bool HasSkillIcon(OperatorKind operatorKind,
                                    int skillIndex) const;
    void DrawPortrait(Rectangle destination, Color tint = WHITE) const;
    void DrawPortrait(OperatorKind operatorKind, Rectangle destination,
                      Color tint = WHITE) const;
    void DrawChibi(Vector2 feetPosition, int facingDirection, float height,
                   ChibiAnimation animation, float animationTime,
                   Color tint = WHITE) const;
    void DrawChibi(OperatorKind operatorKind, Vector2 feetPosition,
                   int facingDirection, float height,
                   ChibiAnimation animation, float animationTime,
                   Color tint = WHITE) const;
    void DrawBattleChibi(Vector2 feetPosition, int facingDirection, float height,
                         BattleChibiAnimation animation, float animationTime,
                         Color tint = WHITE) const;
    void DrawTexasSkill2Battle(Vector2 feetPosition, int facingDirection,
                               float height, bool attacking, bool ending,
                               float animationTime,
                               Color tint = WHITE) const;
    void DrawTexasSkill2Aura(Vector2 center, int facingDirection,
                             float animationTime, bool rainMode,
                             bool transitioning,
                             float transitionProgress) const;
    void DrawTexasSkill2TransitionOverlay(Vector2 center, int facingDirection,
                                          bool rainMode,
                                          float transitionProgress) const;
    void DrawTexasSkill2Slash(Vector2 center, int facingDirection,
                              bool secondStrike, bool artsDamage,
                              float remainingLife, float radius) const;
    void DrawSkillIcon(int skillIndex, Rectangle destination,
                       Color tint = WHITE) const;
    void DrawSkillIcon(OperatorKind operatorKind, int skillIndex,
                       Rectangle destination, Color tint = WHITE) const;

private:
    Texture2D portrait_{};
    Texture2D chibi_{};
    Texture2D battleChibi_{};
    Texture2D skillIcons_[2]{};
    Texture2D texasPortrait_{};
    Texture2D texasChibi_{};
    Texture2D texasSkill2Battle_{};
    Texture2D texasSkill2Aura_{};
    Texture2D texasSkill2Slashes_[2]{};
    Texture2D texasSkill2Burst_{};
    Texture2D texasSkillIcons_[2]{};
};
