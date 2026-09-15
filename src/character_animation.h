#pragma once
#include "character.h"

enum class CharacterAction { Idle, Run, Jump, Fall, Land, Dodge, Attack, Hurt, Defeated };
struct CharacterMotion {
    Vector2 velocity{};
    bool grounded = true;
    bool dodging = false;
    bool attackTriggered = false;
    bool dead = false;
    bool melee = false;
};
struct CharacterPose {
    Vector2 offset{}; // Character-height units, independent of world movement/collisions.
    Vector2 scale{1, 1};
    float angle = 0;
    float armFront = 0, armBack = 0, legFront = 0, legBack = 0, head = 0;
};

class CharacterAnimator {
public:
    void Update(float dt, const CharacterMotion& motion);
    void TriggerHurt() { hurt_ = 0.18F; }
    [[nodiscard]] CharacterPose Pose() const { return pose_; }
    [[nodiscard]] CharacterAction Action() const { return action_; }
    [[nodiscard]] float ActionTime() const { return actionTime_; }
    [[nodiscard]] int Row(const CharacterAssets& assets) const;
private:
    CharacterPose pose_{};
    CharacterAction action_ = CharacterAction::Idle;
    float time_ = 0, phase_ = 0, actionTime_ = 0, attack_ = 0, hurt_ = 0, land_ = 0;
    bool grounded_ = true;
};
