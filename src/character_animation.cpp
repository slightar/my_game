#include "character_animation.h"
#include <algorithm>
#include <cmath>

void CharacterAnimator::Update(float dt, const CharacterMotion& m) {
    if (!std::isfinite(dt) || dt < 0) return;
    dt = std::min(dt, 0.1F);
    time_ += dt;
    attack_ = std::max(0.0F, attack_ - dt);
    hurt_ = std::max(0.0F, hurt_ - dt);
    land_ = std::max(0.0F, land_ - dt);
    if (!grounded_ && m.grounded) land_ = 0.16F;
    grounded_ = m.grounded;
    if (m.attackTriggered && !m.dead) attack_ = 0.24F;
    const float speed = std::clamp(std::abs(m.velocity.x) / 320.0F, 0.0F, 2.0F);
    phase_ = std::fmod(phase_ + dt * 14 * speed, 2 * PI);
    const auto next = m.dead ? CharacterAction::Defeated : hurt_ > 0 ? CharacterAction::Hurt :
        m.dodging ? CharacterAction::Dodge : !m.grounded ? (m.velocity.y < 0 ? CharacterAction::Jump : CharacterAction::Fall) :
        land_ > 0 ? CharacterAction::Land : attack_ > 0 ? CharacterAction::Attack :
        speed > 0.01F ? CharacterAction::Run : CharacterAction::Idle;
    if (next != action_ || m.attackTriggered) actionTime_ = 0;
    else actionTime_ += dt;
    action_ = next;
    CharacterPose target;
    const float gait = std::sin(phase_) * std::min(speed, 1.0F);
    if (m.grounded) {
        target.offset.y = -std::abs(gait) * 0.025F;
        target.scale.y = 1 + std::sin(time_ * 2.8F) * 0.008F;
        target.angle = gait * 2;
        target.legFront = gait * 24; target.legBack = -gait * 24;
        target.armFront = -gait * 20; target.armBack = gait * 20;
        target.head = -gait * 2;
    } else {
        target.scale = m.velocity.y < 0 ? Vector2{0.94F, 1.07F} : Vector2{1.03F, 0.98F};
        target.legFront = -18; target.legBack = 24;
        target.armFront = -25; target.armBack = 25;
        target.angle = m.velocity.y < 0 ? -4 : 5;
    }
    if (land_ > 0 && m.grounded) {
        const float weight = land_ / 0.16F;
        target.scale = {1 + 0.10F * weight, 1 - 0.13F * weight};
    }
    // Upper-body attack layers over locomotion; legs continue their gait in rig mode.
    if (attack_ > 0) {
        const float pulse = std::sin((1 - attack_ / 0.24F) * PI);
        target.armFront = m.melee ? -100 + pulse * 165 : -85 - pulse * 12;
        target.armBack = -35 * pulse;
        target.angle += m.melee ? pulse * 10 : -pulse * 5;
        target.offset.x += m.melee ? pulse * 0.035F : -pulse * 0.025F;
    }
    if (m.dodging) { target.scale = {1.16F, 0.82F}; target.angle = 15; target.armFront = 40; target.armBack = 50; }
    if (hurt_ > 0) { target.angle = -12 * hurt_ / 0.18F; target.scale = {1.07F, 0.93F}; }
    if (m.dead) {
        target = {}; target.scale = {1, 1};
        target.angle = std::min(actionTime_ / 0.45F, 1.0F) * 82;
        target.armFront = -25; target.armBack = 20;
    }
    const float blend = 1 - std::exp(-22 * dt);
    const auto mix = [blend](float& from, float to) { from += (to - from) * blend; };
    mix(pose_.offset.x, target.offset.x); mix(pose_.offset.y, target.offset.y);
    mix(pose_.scale.x, target.scale.x); mix(pose_.scale.y, target.scale.y);
    mix(pose_.angle, target.angle); mix(pose_.head, target.head);
    mix(pose_.armFront, target.armFront); mix(pose_.armBack, target.armBack);
    mix(pose_.legFront, target.legFront); mix(pose_.legBack, target.legBack);
}

int CharacterAnimator::Row(const CharacterAssets& a) const {
    switch (action_) {
        case CharacterAction::Run: return a.runRow;
        case CharacterAction::Attack: return a.attackRow;
        case CharacterAction::Jump: return a.jumpRow;
        case CharacterAction::Fall: return a.fallRow >= 0 ? a.fallRow : a.jumpRow;
        case CharacterAction::Dodge: return a.dodgeRow;
        case CharacterAction::Hurt: return a.hurtRow;
        case CharacterAction::Defeated: return a.defeatedRow;
        default: return a.idleRow;
    }
}
