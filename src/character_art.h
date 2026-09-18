#pragma once

#include "game_types.h"
#include "raylib.h"
#include "character.h"
#include "character_animation.h"
#include "character_image.h"
#include <filesystem>
#include <map>

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
    void DrawFacingRing(Vector2 ground, int facing) const;

    void RegisterCharacter(const Character& character, const std::filesystem::path& assetRoot);
    void ClearCustomCharacters();
    void DrawCustomPortrait(const std::string& id, Rectangle destination, Color tint = WHITE) const;
    void DrawCustomSprite(const Character& character, Vector2 feet, int facing,
                          int row, float time, Color tint = WHITE) const;
    void DrawAnimatedCharacter(const Character& character, Vector2 feet, int facing,
                               const CharacterAnimator& animator, Color tint = WHITE) const;
    void DrawCustomSkill(const std::string& id, int slot, Rectangle destination) const;

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
    void DrawExusiai(Vector2 feetPosition, int facingDirection, ChibiAnimation movement,
                     bool attacking, bool defeated, float movementTime, float attackTime,
                     float defeatTime, Color tint = WHITE) const;
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
    Texture2D facingRing_{};
    Texture2D facingArrow_{};
    struct CustomTextures { Texture2D portrait{}; Texture2D sprite{}; Texture2D icons[2]{};
        std::map<std::string, Texture2D> parts;
        CharacterFrameMap frames;
    };
    std::map<std::string, CustomTextures> custom_;
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
    Texture2D texasSkill2Composite_{};
    Texture2D texasSkill2HitComposite_{};
    Texture2D texasSkill2DarkTrail_{};
    Texture2D texasSkill2Arc_{};
    Texture2D texasSkill2Impact_{};
    Texture2D texasSkillIcons_[2]{};
};
