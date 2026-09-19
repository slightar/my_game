#include "player.h"

#include "audio_system.h"
#include "character_art.h"
#include "ui_theme.h"
#include "ui_font.h"
#include "game_settings.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

constexpr float kMoveSpeed = 320.0F;
constexpr float kJumpSpeed = 610.0F;
constexpr float kGravity = 1650.0F;
constexpr int kMaxJumps = 2;
constexpr float kJumpHoldDuration = 0.18F;
constexpr float kHeldJumpGravityMultiplier = 0.34F;

constexpr float kDodgeSpeed = 880.0F;
constexpr float kDodgeDuration = 0.18F;
constexpr float kDodgeRechargeDuration = 1.5F;
constexpr float kHurtInvincibilityDuration = 1.0F;

constexpr float kFireInterval = 0.085F;
constexpr float kReloadDuration = 1.35F;
constexpr float kBulletSpeed = 1050.0F;
constexpr float kBulletLifetime = 1.4F;
constexpr float kBulletSpreadRadians = 0.06F;
constexpr float kBulletRadius = 7.5F;
constexpr float kOverloadFanStepRadians = 0.10F;
constexpr float kBarrageLaneSpacing = 18.0F;

constexpr float kBarrageDuration = 6.0F;
constexpr float kBarrageCooldownDuration = 12.0F;
constexpr float kOverloadDuration = 4.0F;
constexpr float kOverloadCooldownDuration = 16.0F;

constexpr float kTexasAttackInterval = 0.32F;
constexpr float kTexasRainAttackInterval = 0.52F;
constexpr float kTexasMeleeRange = 112.0F;
constexpr float kTexasRainMeleeRange = 142.0F;
constexpr float kSwordWaveSpeed = 780.0F;
constexpr float kSwordWaveRechargeDuration = 2.4F;
constexpr float kSwordRainDuration = 1.35F;
constexpr float kSwordRainCooldownDuration = 14.0F;
constexpr float kSwordRainSpawnInterval = 0.12F;
constexpr float kSwordRainStunDuration = 0.42F;
constexpr float kTexasMeleeDamage = 1.9F;
constexpr float kTexasRainHitDamage = 1.45F;
constexpr float kTexasSwordWaveDamage = 1.45F;
constexpr float kTexasSwordRainDamage = 1.55F;
constexpr float kTexasRainEnterDuration = 0.62F;
constexpr float kTexasRainExitDuration = 0.28F;
constexpr float kTexasRainSlashDuration = 0.52F;
constexpr float kTexasNormalSlashDuration = 0.17F;

}  // namespace

void Player::Reset(OperatorKind operatorKind) {
    character_.reset();
    animator_ = {};
    operatorKind_ = operatorKind;
    position_ = {240.0F, GameConfig::kFloorY - kHitboxHeight / 2.0F};
    velocity_ = {};
    movementLeft_ = GameConfig::kRoom.x + 28.0F + kHitboxWidth / 2.0F;
    movementRight_ = GameConfig::kRoom.x + GameConfig::kRoom.width - 28.0F -
                     kHitboxWidth / 2.0F;
    facingDirection_ = 1;
    jumpCount_ = 0;
    jumpHoldTimer_ = 0.0F;
    animationTime_ = 0.0F;
    attackAnimationTime_ = 0.0F;
    defeatAnimationTime_ = 0.0F;
    firing_ = false;
    health_ = kMaxHealth;
    hurtInvincibilityTimer_ = 0.0F;
    dodgeCharges_ = kMaxDodgeCharges;
    dodgeDirection_ = 1;
    dodgeTimer_ = 0.0F;
    dodgeRechargeTimer_ = 0.0F;
    ammo_ = kMagazineCapacity;
    shotCooldown_ = 0.0F;
    reloadTimer_ = 0.0F;
    reloading_ = false;
    swordWaveCharges_ = kMaxSwordWaveCharges;
    swordWaveRechargeTimer_ = 0.0F;
    texasRainMode_ = false;
    texasRainBurstTimer_ = 0.0F;
    texasAttackEffectTimer_ = 0.0F;
    swordRainTimer_ = 0.0F;
    swordRainCooldown_ = 0.0F;
    swordRainSpawnTimer_ = 0.0F;
    barrageTimer_ = 0.0F;
    barrageCooldown_ = 0.0F;
    overloadTimer_ = 0.0F;
    overloadCooldown_ = 0.0F;
}

void Player::Reset(const Character& character) {
    character.Validate();
    Reset(OperatorKind::Custom);
    character_.emplace(character);
    health_ = character.stats.health;
}

void Player::PlaceAt(Vector2 position, int facing) {
    position_ = position;
    position_.x = std::clamp(position_.x, movementLeft_, movementRight_);
    velocity_ = {};
    facingDirection_ = facing;
    jumpCount_ = position_.y + kHitboxHeight / 2.0F < GameConfig::kFloorY - 1.0F ? 1 : 0;
    jumpHoldTimer_ = 0.0F;
    dodgeTimer_ = 0.0F;
    firing_ = false;
}

void Player::Update(float deltaTime, Vector2 enemyPosition, float enemyRadius,
                    std::vector<Bullet>& bullets, AudioSystem& audio, const GameSettings& settings, bool assistEnemyAim) {
    hurtInvincibilityTimer_ = std::max(0.0F, hurtInvincibilityTimer_ - deltaTime);
    animationTime_ += deltaTime;
    if (IsDead()) {
        return;
    }

    if (operatorKind_ == OperatorKind::Exusiai && barrageTimer_ > 0.0F) {
        barrageTimer_ = std::max(0.0F, barrageTimer_ - deltaTime);
        if (barrageTimer_ <= 0.0F) {
            barrageCooldown_ = kBarrageCooldownDuration;
        }
    } else if (operatorKind_ == OperatorKind::Exusiai) {
        barrageCooldown_ = std::max(0.0F, barrageCooldown_ - deltaTime);
    }
    if (operatorKind_ == OperatorKind::Exusiai && overloadTimer_ > 0.0F) {
        overloadTimer_ = std::max(0.0F, overloadTimer_ - deltaTime);
        if (overloadTimer_ <= 0.0F) {
            overloadCooldown_ = kOverloadCooldownDuration;
        }
    } else if (operatorKind_ == OperatorKind::Exusiai) {
        overloadCooldown_ = std::max(0.0F, overloadCooldown_ - deltaTime);
    }
    if (operatorKind_ == OperatorKind::Exusiai && settings.Pressed(GameAction::SkillTwo) &&
        barrageTimer_ <= 0.0F &&
        barrageCooldown_ <= 0.0F) {
        barrageTimer_ = kBarrageDuration;
    }
    if (operatorKind_ == OperatorKind::Exusiai && settings.Pressed(GameAction::SkillOne) &&
        overloadTimer_ <= 0.0F &&
        overloadCooldown_ <= 0.0F) {
        overloadTimer_ = kOverloadDuration;
    }

    float moveDirection = 0.0F;
    if (settings.Down(GameAction::MoveLeft)) {
        moveDirection -= 1.0F;
    }
    if (settings.Down(GameAction::MoveRight)) {
        moveDirection += 1.0F;
    }

    bool texasUsingSwordWave = false;
    if (!character_ && operatorKind_ == OperatorKind::Texas &&
        !texasRainMode_ && swordWaveCharges_ > 0 && settings.Down(GameAction::Attack)) {
        const float deltaX = enemyPosition.x - position_.x;
        const bool enemyInMeleeRange =
            deltaX * static_cast<float>(facingDirection_) >= -enemyRadius &&
            std::abs(deltaX) <= kTexasMeleeRange + enemyRadius &&
            std::abs(enemyPosition.y - position_.y) <= 100.0F;
        texasUsingSwordWave = !enemyInMeleeRange;
    }
    const bool rangedBasicAttack =
        character_ ? character_->Definition().attack.type != EffectType::Melee
                   : operatorKind_ == OperatorKind::Exusiai ||
                         texasUsingSwordWave;
    const bool lockFacingWhileAttacking =
        rangedBasicAttack && settings.Down(GameAction::Attack);
    if (moveDirection != 0.0F && !lockFacingWhileAttacking) {
        facingDirection_ = moveDirection > 0.0F ? 1 : -1;
    }
    if (dodgeCharges_ < kMaxDodgeCharges) {
        dodgeRechargeTimer_ -= deltaTime;
        if (dodgeRechargeTimer_ <= 0.0F) {
            ++dodgeCharges_;
            dodgeRechargeTimer_ = dodgeCharges_ < kMaxDodgeCharges
                                        ? kDodgeRechargeDuration
                                        : 0.0F;
        }
    }

    const bool dodgePressed = settings.Pressed(GameAction::Dodge);
    if (dodgePressed && dodgeCharges_ > 0 && dodgeTimer_ <= 0.0F) {
        dodgeDirection_ = moveDirection != 0.0F
                              ? (moveDirection > 0.0F ? 1 : -1)
                              : facingDirection_;
        facingDirection_ = dodgeDirection_;
        dodgeTimer_ = kDodgeDuration;
        --dodgeCharges_;
        if (dodgeRechargeTimer_ <= 0.0F) {
            dodgeRechargeTimer_ = kDodgeRechargeDuration;
        }
    }

    if (dodgeTimer_ > 0.0F) {
        velocity_.x = static_cast<float>(dodgeDirection_) * kDodgeSpeed;
        dodgeTimer_ = std::max(0.0F, dodgeTimer_ - deltaTime);
    } else {
        velocity_.x = moveDirection * (character_ ? character_->Definition().stats.moveSpeed : kMoveSpeed);
    }

    const bool jumpPressed = settings.Pressed(GameAction::Jump);
    const bool jumpHeld = settings.Down(GameAction::Jump);
    if (jumpPressed && jumpCount_ < (character_ ? character_->Definition().stats.jumps : kMaxJumps) && dodgeTimer_ <= 0.0F) {
        velocity_.y = -(character_ ? character_->Definition().stats.jumpSpeed : kJumpSpeed);
        ++jumpCount_;
        jumpHoldTimer_ = kJumpHoldDuration;
    }

    float gravityMultiplier = 1.0F;
    if (jumpHeld && velocity_.y < 0.0F && jumpHoldTimer_ > 0.0F) {
        gravityMultiplier = kHeldJumpGravityMultiplier;
        jumpHoldTimer_ -= deltaTime;
    } else {
        jumpHoldTimer_ = 0.0F;
    }

    velocity_.y += kGravity * gravityMultiplier * deltaTime;
    position_.x += velocity_.x * deltaTime;
    position_.y += velocity_.y * deltaTime;

    position_.x = std::clamp(position_.x, movementLeft_, movementRight_);

    const float ceilingLimit = GameConfig::kRoom.y + 28.0F +
                               kHitboxHeight / 2.0F;
    if (position_.y < ceilingLimit) {
        position_.y = ceilingLimit;
        velocity_.y = 0.0F;
    }
    if (position_.y + kHitboxHeight / 2.0F >= GameConfig::kFloorY) {
        position_.y = GameConfig::kFloorY - kHitboxHeight / 2.0F;
        velocity_.y = 0.0F;
        jumpCount_ = 0;
    }

    shotCooldown_ = std::max(0.0F, shotCooldown_ - deltaTime);
    if (character_) {
        firing_ = settings.Down(GameAction::Attack) && dodgeTimer_ <= 0;
        character_->Update(deltaTime, position_, facingDirection_, enemyPosition,
                           firing_, {settings.Pressed(GameAction::SkillTwo) && dodgeTimer_ <= 0,
                                     settings.Pressed(GameAction::SkillOne) && dodgeTimer_ <= 0}, health_, bullets);
        animator_.Update(deltaTime, {velocity_, position_.y + kHitboxHeight / 2 >= GameConfig::kFloorY - 0.5F,
            dodgeTimer_ > 0, character_->PerformedAction(), false, character_->Definition().attack.type == EffectType::Melee});
        attackAnimationTime_ = firing_ ? attackAnimationTime_ + deltaTime : 0;
        return;
    }
    if (operatorKind_ == OperatorKind::Texas) {
        UpdateTexasCombat(deltaTime, enemyPosition, enemyRadius, bullets, settings);
        return;
    }
    if (settings.Pressed(GameAction::Reload) && ammo_ < kMagazineCapacity && !reloading_) {
        reloading_ = true;
        reloadTimer_ = kReloadDuration;
        audio.PlayReload();
    }

    if (reloading_) {
        reloadTimer_ -= deltaTime;
        if (reloadTimer_ <= 0.0F) {
            ammo_ = kMagazineCapacity;
            reloading_ = false;
        }
    }

    if (settings.Down(GameAction::Attack) && !reloading_ && ammo_ > 0 && shotCooldown_ <= 0.0F &&
        dodgeTimer_ <= 0.0F) {
        const float direction = static_cast<float>(facingDirection_);
        const Vector2 muzzle{
            position_.x + direction * (kHitboxWidth / 2.0F + 30.0F),
            position_.y - 16.0F};
        // Trial-only aim assist lets horizontal gunfire reach low-profile enemies.
        const float aimAngle = assistEnemyAim && (enemyPosition.x - muzzle.x) * direction > 0
            ? std::clamp(std::atan2(enemyPosition.y - muzzle.y, std::abs(enemyPosition.x - muzzle.x)), -.65F, .65F)
            : 0.0F;
        const bool barrageActive = barrageTimer_ > 0.0F;
        const bool overloadActive = overloadTimer_ > 0.0F;
        float bulletDamage = 1.0F;
        if (barrageActive) {
            bulletDamage *= 0.6F;
        }
        if (overloadActive) {
            bulletDamage *= 0.5F;
        }
        const int fanCount = overloadActive ? 5 : 1;
        const int laneCount = barrageActive ? 2 : 1;
        const float randomSpread =
            overloadActive ? 0.0F
                           : static_cast<float>(GetRandomValue(-1000, 1000)) /
                                 1000.0F * kBulletSpreadRadians;

        for (int fanIndex = 0; fanIndex < fanCount; ++fanIndex) {
            const float fanOffset =
                static_cast<float>(fanIndex - fanCount / 2) *
                kOverloadFanStepRadians;
            const float angle = aimAngle + fanOffset + randomSpread;
            const Vector2 velocity{direction * std::cos(angle) * kBulletSpeed,
                                   std::sin(angle) * kBulletSpeed};
            const Vector2 perpendicular{-velocity.y / kBulletSpeed,
                                        velocity.x / kBulletSpeed};
            for (int laneIndex = 0; laneIndex < laneCount; ++laneIndex) {
                const float laneOffset =
                    (static_cast<float>(laneIndex) -
                     static_cast<float>(laneCount - 1) / 2.0F) *
                    kBarrageLaneSpacing;
                bullets.push_back({
                    {muzzle.x + perpendicular.x * laneOffset,
                     muzzle.y + perpendicular.y * laneOffset},
                    velocity, kBulletLifetime, kBulletRadius, bulletDamage});
            }
        }
        --ammo_;
        shotCooldown_ = kFireInterval;
        audio.PlayGunshot(ammo_);
    } else if (settings.Pressed(GameAction::Attack) && reloading_) {
        audio.PlayEmptyClick();
    }

    if (ammo_ == 0 && !reloading_) {
        reloading_ = true;
        reloadTimer_ = kReloadDuration;
        audio.PlayReload();
    }

    firing_ = settings.Down(GameAction::Attack) && !reloading_ && ammo_ > 0 &&
              dodgeTimer_ <= 0.0F;
    if (firing_) {
        attackAnimationTime_ += deltaTime;
    } else {
        attackAnimationTime_ = 0.0F;
    }
}

void Player::UpdateTexasCombat(float deltaTime, Vector2 enemyPosition,
                               float enemyRadius,
                               std::vector<Bullet>& bullets,
                               const GameSettings& settings) {
    texasAttackEffectTimer_ =
        std::max(0.0F, texasAttackEffectTimer_ - deltaTime);
    texasRainBurstTimer_ =
        std::max(0.0F, texasRainBurstTimer_ - deltaTime);

    if (swordWaveCharges_ < kMaxSwordWaveCharges) {
        swordWaveRechargeTimer_ -= deltaTime;
        while (swordWaveRechargeTimer_ <= 0.0F &&
               swordWaveCharges_ < kMaxSwordWaveCharges) {
            ++swordWaveCharges_;
            swordWaveRechargeTimer_ += kSwordWaveRechargeDuration;
        }
        if (swordWaveCharges_ >= kMaxSwordWaveCharges) {
            swordWaveRechargeTimer_ = 0.0F;
        }
    }

    if (settings.Pressed(GameAction::SkillTwo) && texasRainBurstTimer_ <= 0.0F) {
        texasRainMode_ = !texasRainMode_;
        texasRainBurstTimer_ = texasRainMode_ ? kTexasRainEnterDuration
                                              : kTexasRainExitDuration;
    }

    if (swordRainTimer_ > 0.0F) {
        swordRainTimer_ = std::max(0.0F, swordRainTimer_ - deltaTime);
        swordRainSpawnTimer_ -= deltaTime;
        while (swordRainSpawnTimer_ <= 0.0F && swordRainTimer_ > 0.0F) {
            const float randomX = static_cast<float>(GetRandomValue(-130, 130));
            bullets.push_back({
                {std::clamp(enemyPosition.x + randomX,
                            GameConfig::kRoom.x + 25.0F,
                            GameConfig::kRoom.x + GameConfig::kRoom.width - 25.0F),
                 GameConfig::kRoom.y + 18.0F},
                {static_cast<float>(GetRandomValue(-35, 35)), 920.0F},
                0.82F, 15.0F, kTexasSwordRainDamage, BulletKind::FallingSword,
                DamageType::Arts, false, kSwordRainStunDuration});
            swordRainSpawnTimer_ += kSwordRainSpawnInterval;
        }
        if (swordRainTimer_ <= 0.0F) {
            swordRainCooldown_ = kSwordRainCooldownDuration;
        }
    } else {
        swordRainCooldown_ = std::max(0.0F, swordRainCooldown_ - deltaTime);
    }
    if (settings.Pressed(GameAction::SkillOne) && swordRainTimer_ <= 0.0F &&
        swordRainCooldown_ <= 0.0F) {
        swordRainTimer_ = kSwordRainDuration;
        swordRainSpawnTimer_ = 0.0F;
    }

    if (settings.Down(GameAction::Attack) && shotCooldown_ <= 0.0F && dodgeTimer_ <= 0.0F) {
        const float direction = static_cast<float>(facingDirection_);
        const float range = texasRainMode_ ? kTexasRainMeleeRange
                                           : kTexasMeleeRange;
        const float deltaX = enemyPosition.x - position_.x;
        const bool enemyInMeleeRange =
            deltaX * direction >= -enemyRadius &&
            std::abs(deltaX) <= range + enemyRadius &&
            std::abs(enemyPosition.y - position_.y) <= 100.0F;

        if (texasRainMode_ || enemyInMeleeRange) {
            const int strikeCount = texasRainMode_ ? 2 : 1;
            for (int strike = 0; strike < strikeCount; ++strike) {
                bullets.push_back({
                    {position_.x + direction * range * 0.54F,
                     position_.y - 8.0F + static_cast<float>(strike) * 12.0F},
                    {}, texasRainMode_ ? kTexasRainSlashDuration
                                       : kTexasNormalSlashDuration,
                    range * 0.53F,
                    texasRainMode_ ? kTexasRainHitDamage : kTexasMeleeDamage,
                    BulletKind::MeleeSlash,
                    texasRainMode_ ? DamageType::Arts : DamageType::Physical,
                    false, 0.0F,
                    texasRainMode_ ? static_cast<float>(strike) * 0.16F
                                   : 0.0F,
                    strike, facingDirection_});
            }
        } else if (swordWaveCharges_ > 0) {
            bullets.push_back({
                {position_.x + direction * 48.0F, position_.y - 7.0F},
                {direction * kSwordWaveSpeed, 0.0F},
                1.35F, 13.0F, kTexasSwordWaveDamage, BulletKind::SwordWave,
                DamageType::Physical, true, 0.0F});
            --swordWaveCharges_;
            if (swordWaveRechargeTimer_ <= 0.0F) {
                swordWaveRechargeTimer_ = kSwordWaveRechargeDuration;
            }
        }
        shotCooldown_ = texasRainMode_ ? kTexasRainAttackInterval
                                       : kTexasAttackInterval;
        texasAttackEffectTimer_ = texasRainMode_ ? kTexasRainSlashDuration
                                                 : kTexasNormalSlashDuration;
    }

    firing_ = texasAttackEffectTimer_ > 0.0F;
    if (firing_) {
        attackAnimationTime_ += deltaTime;
    } else {
        attackAnimationTime_ = 0.0F;
    }
}

void Player::UpdateDefeatAnimation(float deltaTime) {
    defeatAnimationTime_ += deltaTime;
    if (character_) animator_.Update(deltaTime, {{}, true, false, false, true});
}

void Player::SetHorizontalBounds(float left, float right) {
    movementLeft_ = std::min(left, right);
    movementRight_ = std::max(left, right);
    position_.x = std::clamp(position_.x, movementLeft_, movementRight_);
}

void Player::Draw(const CharacterArt& art) const {
    if (!IsDead()) {
        const Vector2 ringCenter{position_.x, GameConfig::kFloorY + 2.0F};
        art.DrawFacingRing(ringCenter, facingDirection_);
    }

    if (character_) {
        const auto& c = character_->Definition();
        const Color tint = IsDead() ? Fade(WHITE, 0.3F) : IsInvincible() ? Fade(SKYBLUE, 0.65F) : WHITE;
        art.DrawAnimatedCharacter(c, {position_.x, position_.y + kHitboxHeight / 2}, facingDirection_, animator_, tint);
        return;
    }
    const bool airborne = position_.y + kHitboxHeight / 2.0F <
                          GameConfig::kFloorY - 0.5F;
    ChibiAnimation animation = ChibiAnimation::Idle;
    if (dodgeTimer_ > 0.0F) {
        animation = ChibiAnimation::Dodge;
    } else if (airborne) {
        animation = ChibiAnimation::Jump;
    } else if (std::abs(velocity_.x) > 1.0F) {
        animation = ChibiAnimation::Run;
    }
    const bool moving = !airborne && std::abs(velocity_.x) > 1.0F;
    const float walkWave = std::sin(animationTime_ * 10.0F);
    const float walkBob = moving
                              ? std::abs(walkWave) * 1.5F
                              : 0.0F;
    const Vector2 feetPosition{position_.x,
                               position_.y + kHitboxHeight / 2.0F - walkBob};

    if (operatorKind_ == OperatorKind::Texas &&
        (texasRainMode_ || texasRainBurstTimer_ > 0.0F)) {
        const bool transitioning = texasRainBurstTimer_ > 0.0F;
        const float transitionDuration = texasRainMode_
                                             ? kTexasRainEnterDuration
                                             : kTexasRainExitDuration;
        const float transitionProgress = transitioning
            ? 1.0F - texasRainBurstTimer_ / transitionDuration
            : 1.0F;
        for (int streak = 0; streak < 5; ++streak) {
            const float phase = std::fmod(
                animationTime_ * 1.35F + static_cast<float>(streak) * 0.213F,
                1.0F);
            const float streakX = position_.x - 58.0F +
                                  static_cast<float>(streak) * 29.0F;
            const float streakY = GameConfig::kFloorY - phase * 105.0F;
            const float alpha = std::sin(phase * PI) *
                                (texasRainMode_ ? 0.34F : 0.14F);
            DrawLineEx({streakX - 5.0F, streakY - 13.0F},
                       {streakX + 5.0F, streakY + 9.0F},
                       4.0F, Fade(BLACK, alpha));
            DrawLineEx({streakX - 4.0F, streakY - 12.0F},
                       {streakX + 4.0F, streakY + 8.0F},
                       1.5F, Fade(Color{236, 30, 44, 255}, alpha));
        }

        art.DrawTexasSkill2Aura(position_, facingDirection_, animationTime_,
                                texasRainMode_, transitioning,
                                std::clamp(transitionProgress, 0.0F, 1.0F));
    }

    if (airborne) {
        const float heightAboveFloor = GameConfig::kFloorY -
                                       (position_.y + kHitboxHeight / 2.0F);
        const float shadowScale = std::clamp(1.0F - heightAboveFloor / 430.0F,
                                             0.38F, 1.0F);
        DrawEllipse(static_cast<int>(position_.x),
                    static_cast<int>(GameConfig::kFloorY + 3.0F),
                    27.0F * shadowScale, 7.0F * shadowScale,
                    Fade(BLACK, 0.24F));
    }

    if (dodgeTimer_ > 0.0F) {
        for (int trail = 1; trail <= 3; ++trail) {
            const Vector2 trailPosition{
                position_.x - static_cast<float>(dodgeDirection_ * trail) * 18.0F,
                position_.y + kHitboxHeight / 2.0F};
            if (operatorKind_ == OperatorKind::Texas &&
                art.HasChibi(OperatorKind::Texas)) {
                art.DrawChibi(OperatorKind::Texas, trailPosition,
                              facingDirection_, 106.0F,
                              ChibiAnimation::Dodge, animationTime_,
                              Fade(WHITE, 0.11F));
            } else if (operatorKind_ != OperatorKind::Texas &&
                       (art.HasBattleChibi() || art.HasChibi())) {
                art.DrawExusiai(trailPosition, facingDirection_, ChibiAnimation::Dodge,
                                false, false, animationTime_, 0, 0, Fade(WHITE, 0.11F));
            } else {
                const Rectangle trailBody{trailPosition.x - 15.0F,
                                          trailPosition.y - 68.0F,
                                          30.0F, 68.0F};
                DrawRectangleRec(trailBody, Fade(BLACK, 0.16F));
            }
        }
    }

    if (IsInvincible() && !IsDead()) {
        Rectangle aura = Hitbox();
        aura.x -= 7.0F;
        aura.y -= 7.0F;
        aura.width += 14.0F;
        aura.height += 14.0F;
        DrawRectangleLinesEx(aura, 3.0F,
                             dodgeTimer_ > 0.0F ? SKYBLUE : ORANGE);
    }

    const bool blinkOff = hurtInvincibilityTimer_ > 0.0F &&
                          static_cast<int>(hurtInvincibilityTimer_ * 14.0F) % 2 == 0;
    if (operatorKind_ == OperatorKind::Texas &&
        (texasRainMode_ || texasRainBurstTimer_ > 0.0F) &&
        art.HasTexasSkill2Battle()) {
        const bool ending = !texasRainMode_;
        const float skillAnimationTime =
            ending ? 0.18F - texasRainBurstTimer_
                   : (firing_ ? attackAnimationTime_ : animationTime_);
        art.DrawTexasSkill2Battle(
            feetPosition, facingDirection_, 120.0F,
            texasRainMode_ && firing_, ending, skillAnimationTime,
            IsDead() ? Fade(WHITE, 0.28F)
                     : (blinkOff ? Fade(WHITE, 0.3F) : WHITE));
    } else if (operatorKind_ == OperatorKind::Texas &&
        art.HasChibi(OperatorKind::Texas)) {
        const ChibiAnimation texasAnimation =
            firing_ ? ChibiAnimation::Wave : animation;
        art.DrawChibi(OperatorKind::Texas, feetPosition, facingDirection_,
                      112.0F, texasAnimation,
                      firing_ ? attackAnimationTime_ : animationTime_,
                      IsDead() ? Fade(WHITE, 0.28F)
                               : (blinkOff ? Fade(WHITE, 0.3F) : WHITE));
    } else if (operatorKind_ != OperatorKind::Texas && (art.HasBattleChibi() || art.HasChibi())) {
        art.DrawExusiai(feetPosition, facingDirection_, animation, firing_, IsDead(),
                        animationTime_, attackAnimationTime_, defeatAnimationTime_,
                        !IsDead() && blinkOff ? Fade(WHITE, 0.3F) : WHITE);
    } else {
        const Vector2 weaponStart{position_.x, position_.y - 10.0F};
        const Vector2 weaponEnd{
            position_.x + static_cast<float>(facingDirection_) * 48.0F,
            position_.y - 10.0F};
        DrawLineEx(weaponStart, weaponEnd,
                   operatorKind_ == OperatorKind::Texas ? 5.0F : 9.0F,
                   operatorKind_ == OperatorKind::Texas ? RAYWHITE : DARKGRAY);
        const Rectangle body{position_.x - 16.0F, position_.y - 28.0F,
                             32.0F, 62.0F};
        DrawRectangleRec(body, blinkOff ? Fade(BLACK, 0.3F) : BLACK);
    }

    if (operatorKind_ == OperatorKind::Texas && texasRainBurstTimer_ > 0.0F) {
        const float transitionDuration = texasRainMode_
                                             ? kTexasRainEnterDuration
                                             : kTexasRainExitDuration;
        const float transitionProgress =
            1.0F - texasRainBurstTimer_ / transitionDuration;
        art.DrawTexasSkill2TransitionOverlay(
            position_, facingDirection_, texasRainMode_,
            std::clamp(transitionProgress, 0.0F, 1.0F));
    }
}

void Player::DrawHud(const UiFont& font, const CharacterArt& art,
                     const char* operatorName, const GameSettings* settings) const {
    const int maxHealth = character_ ? character_->Definition().stats.health
                                     : kMaxHealth;
    const char* displayName = character_ ? character_->Definition().name.c_str()
                                         : operatorName;
    const char* role = character_
                           ? (character_->Definition().attack.type ==
                                      EffectType::Melee
                                  ? "GUARD // 近卫"
                                  : "RANGED // 远程")
                           : operatorKind_ == OperatorKind::Texas
                                 ? "GUARD // 近卫"
                                 : "SNIPER // 狙击";
    const Rectangle statusPanel{32.0F, 28.0F, 405.0F, 116.0F};
    TacticalUi::DrawCutPanel(statusPanel, TacticalUi::kPanel,
                             Fade(RAYWHITE, 0.22F), 18.0F, 1.0F);
    font.Skin().Draw("panel", statusPanel);
    font.Draw(role, 53.0F, 39.0F, 13.0F, TacticalUi::kMuted);
    font.Draw(displayName, 53.0F, 58.0F, 27.0F, TacticalUi::kPaper);
    font.Draw(TextFormat("HP  %i / %i", health_, maxHealth),
              53.0F, 97.0F, 15.0F, TacticalUi::kPaper);
    TacticalUi::DrawProgressLine(
        {143.0F, 108.0F}, 210.0F,
        static_cast<float>(health_) / static_cast<float>(maxHealth),
        health_ <= 1 ? TacticalUi::kRed : TacticalUi::kCyan, 7.0F);
    font.Draw("DODGE", 358.0F, 91.0F, 11.0F, TacticalUi::kMuted);
    DrawRectangleRec({378.0F, 110.0F, 27.0F, 5.0F},
                     dodgeCharges_ > 0 ? TacticalUi::kPaper
                                       : Color{66, 72, 78, 255});

    const Rectangle helpPanel{32.0F, 669.0F, 570.0F, 27.0F};
    TacticalUi::DrawCutPanel(helpPanel, Fade(TacticalUi::kInk, 0.82F),
                             BLANK, 7.0F);
    const std::string controls = settings
        ? "MOVE " + GameSettings::KeyName(settings->Key(GameAction::MoveLeft)) + "/" +
          GameSettings::KeyName(settings->Key(GameAction::MoveRight)) +
          "  JUMP " + GameSettings::KeyName(settings->Key(GameAction::Jump)) +
          "  DODGE " + GameSettings::KeyName(settings->Key(GameAction::Dodge)) +
          "  ATTACK " + GameSettings::KeyName(settings->Key(GameAction::Attack)) +
          "  SKILL " + GameSettings::KeyName(settings->Key(GameAction::SkillOne)) + "/" +
          GameSettings::KeyName(settings->Key(GameAction::SkillTwo))
        : "A/D MOVE   W/K JUMP   J ATTACK   S/L DODGE   E/Q SKILL";
    font.Draw(controls.c_str(), 45.0F, 675.0F, 13.0F, Fade(RAYWHITE, 0.76F));

    if (character_) {
        const auto& c = character_->Definition();
        for (std::size_t i = 0; i < c.skills.size(); ++i) {
            const auto& skill = c.skills[i]; const auto& state = character_->Skills()[i];
            const float x = 978.0F + static_cast<float>(i) * 145.0F;
            constexpr float y = 604.0F;
            const Rectangle skillPanel{x, y, 137.0F, 92.0F};
            font.Skin().Draw("skill_shadow", {x - 8, y - 8, 150, 110});
            TacticalUi::DrawCutPanel(skillPanel, TacticalUi::kPanel,
                                     state.cooldown <= 0.0F
                                         ? Color{185, 221, 66, 255}
                                         : Fade(RAYWHITE, 0.30F),
                                     12.0F, state.cooldown <= 0.0F ? 2.0F : 1.0F);
            art.DrawCustomSkill(c.id, static_cast<int>(i),
                                {x + 6.0F, y + 6.0F, 48.0F, 48.0F});
            font.Draw(i == 0 ? "E" : "Q", x + 61.0F, y + 7.0F, 20.0F,
                      TacticalUi::kCyan);
            const float size = std::min(16.0F, 124.0F / std::max(1.0F, font.Measure(skill.name.c_str(), 16)) * 16);
            font.Draw(skill.name.c_str(), x + 8.0F, y + 59.0F, size, RAYWHITE);
            font.Draw(state.remaining > 0 ? TextFormat("生效 %.1f", state.remaining) :
                      state.cooldown > 0 ? TextFormat("冷却 %.1f", state.cooldown) : "READY",
                      x + 78.0F, y + 31.0F, 13.0F,
                      state.cooldown <= 0.0F ? Color{185, 221, 66, 255}
                                            : TacticalUi::kMuted);
        }
        return;
    }
    constexpr float ammoPanelX = 32.0F;
    constexpr float ammoPanelY = 158.0F;
    constexpr float ammoPanelWidth = 190.0F;
    constexpr float ammoPanelHeight = 82.0F;
    TacticalUi::DrawCutPanel(
        {ammoPanelX, ammoPanelY, ammoPanelWidth, ammoPanelHeight},
        TacticalUi::kPanelSoft, Fade(RAYWHITE, 0.18F), 13.0F);
    if (operatorKind_ == OperatorKind::Texas) {
        font.Draw("SWORD WAVE // 剑气", ammoPanelX + 15.0F,
                  ammoPanelY + 10.0F, 13.0F, TacticalUi::kMuted);
        for (int charge = 0; charge < kMaxSwordWaveCharges; ++charge) {
            const Rectangle segment{
                ammoPanelX + 15.0F + static_cast<float>(charge) * 23.0F,
                ammoPanelY + 43.0F, 17.0F, 8.0F};
            DrawRectangleRec(segment,
                             charge < swordWaveCharges_
                                 ? TacticalUi::kPaper
                                 : Color{55, 61, 69, 255});
        }
        if (swordWaveCharges_ < kMaxSwordWaveCharges) {
            const float restoreRatio = std::clamp(
                1.0F - swordWaveRechargeTimer_ / kSwordWaveRechargeDuration,
                0.0F, 1.0F);
            TacticalUi::DrawProgressLine({ammoPanelX + 15.0F,
                                           ammoPanelY + 65.0F},
                                          132.0F, restoreRatio,
                                          TacticalUi::kCyan, 4.0F);
        }
    } else {
        font.Draw("SMG // 弹匣", ammoPanelX + 15.0F, ammoPanelY + 10.0F,
                  13.0F, TacticalUi::kMuted);
        const float ammoRatio = static_cast<float>(ammo_) /
                                static_cast<float>(kMagazineCapacity);
        TacticalUi::DrawProgressLine({ammoPanelX + 15.0F,
                                       ammoPanelY + 51.0F},
                                      105.0F, ammoRatio,
                                      reloading_ ? TacticalUi::kOrange
                                                 : TacticalUi::kCyan,
                                      7.0F);
        const char* ammoText = TextFormat("%02i/%02i", ammo_, kMagazineCapacity);
        font.Draw(ammoText, ammoPanelX + 124.0F, ammoPanelY + 40.0F,
                  15.0F, TacticalUi::kPaper);
    }

    const auto drawSkillIcon = [&font, &art, this](float x, const char* key,
                                       const char* name, float activeTimer,
                                       float cooldownTimer,
                                       float cooldownDuration, Color color,
                                       int skillIndex,
                                       bool showActiveTimer = true) {
        constexpr float size = 86.0F;
        const Rectangle icon{x, 604.0F, size, size};
        const bool active = activeTimer > 0.0F;
        const bool coolingDown = !active && cooldownTimer > 0.0F;
        font.Skin().Draw("skill_shadow", {icon.x - 13, icon.y - 12, 115, 112});
        TacticalUi::DrawCutPanel(
            {icon.x - 7.0F, icon.y - 8.0F,
             icon.width + 14.0F, icon.height + 14.0F},
            TacticalUi::kPanel,
            active ? color : coolingDown ? Fade(RAYWHITE, 0.20F)
                                         : Color{185, 221, 66, 255},
            12.0F, active || !coolingDown ? 2.0F : 1.0F);
        DrawRectangleRec(icon, active ? Fade(color, 0.82F)
                                     : (coolingDown
                                            ? Color{25, 29, 35, 248}
                                            : Color{39, 44, 52, 242}));
        const Rectangle artwork{icon.x + 3.0F, icon.y + 3.0F,
                                icon.width - 6.0F, icon.height - 6.0F};
        if (art.HasSkillIcon(operatorKind_, skillIndex)) {
            art.DrawSkillIcon(operatorKind_, skillIndex, artwork,
                              coolingDown ? Color{150, 150, 150, 255}
                                          : WHITE);
        } else {
            font.Draw(key, icon.x + 31.0F, icon.y + 24.0F,
                      34.0F, RAYWHITE);
        }
        DrawRectangleLinesEx(icon, active ? 3.0F : 1.0F,
                             active ? color : Fade(RAYWHITE, 0.68F));

        DrawRectangleRec({icon.x - 4.0F, icon.y - 4.0F, 27.0F, 25.0F},
                         coolingDown ? Fade(BLACK, 0.82F)
                                     : Color{85, 110, 35, 255});
        font.Draw(key, icon.x + 4.0F, icon.y - 3.0F, 18.0F, RAYWHITE);
        if (coolingDown) {
            const float progress = std::clamp(
                1.0F - cooldownTimer / cooldownDuration, 0.0F, 1.0F);
            const Vector2 center{icon.x + size / 2.0F,
                                 icon.y + size / 2.0F};
            DrawRing(center, 34.0F, 40.0F, -90.0F, 270.0F, 64,
                     Color{108, 112, 120, 245});
            DrawRing(center, 34.0F, 40.0F, -90.0F,
                     -90.0F + progress * 360.0F, 64, RAYWHITE);
            const float handAngle = (-90.0F + progress * 360.0F) * DEG2RAD;
            const Vector2 handEnd{center.x + std::cos(handAngle) * 32.0F,
                                  center.y + std::sin(handAngle) * 32.0F};
            DrawLineEx(center, handEnd, 4.0F,
                       Color{132, 136, 144, 255});
            DrawCircleV(center, 4.0F, Color{132, 136, 144, 255});
        }
        DrawRectangleRec({icon.x + 2.0F, icon.y + 62.0F,
                          icon.width - 4.0F, 22.0F},
                         Fade(BLACK, 0.76F));
        font.Draw(name, icon.x + 8.0F, icon.y + 65.0F, 14.0F, RAYWHITE);
        if (!active && !coolingDown) {
            DrawRectangleRec({icon.x + 2.0F, icon.y + icon.height - 3.0F,
                              icon.width - 4.0F, 4.0F},
                             TacticalUi::kOrange);
        }
        if ((active && showActiveTimer) || coolingDown) {
            const float timer = active ? activeTimer : cooldownTimer;
            const char* timerText = TextFormat("%.1f", timer);
            const float timerWidth = font.Measure(timerText, 17.0F);
            DrawRectangleRec({icon.x + icon.width - timerWidth - 11.0F,
                              icon.y + 5.0F, timerWidth + 7.0F, 22.0F},
                             Fade(BLACK, 0.72F));
            font.Draw(timerText, icon.x + icon.width - timerWidth - 8.0F,
                      icon.y + 6.0F, 17.0F, RAYWHITE);
        }
    };
    if (operatorKind_ == OperatorKind::Texas) {
        drawSkillIcon(1082.0F, "E", texasRainMode_ ? "阵雨连绵" : "初始",
                      texasRainMode_ ? 1.0F : 0.0F, 0.0F, 1.0F,
                      Color{207, 45, 48, 255}, 0, false);
        drawSkillIcon(1176.0F, "Q", "剑雨", swordRainTimer_,
                      swordRainCooldown_, kSwordRainCooldownDuration,
                      Color{230, 235, 242, 255}, 1);
    } else {
        drawSkillIcon(1082.0F, "E", "扫射", barrageTimer_, barrageCooldown_,
                      kBarrageCooldownDuration, SKYBLUE, 0);
        drawSkillIcon(1176.0F, "Q", "过载", overloadTimer_, overloadCooldown_,
                      kOverloadCooldownDuration, ORANGE, 1);
    }

    if (operatorKind_ == OperatorKind::Exusiai && reloading_) {
        const char* reloadText = "换弹中……";
        const float reloadWidth = font.Measure(reloadText, 24.0F);
        font.Draw(reloadText, ammoPanelX + ammoPanelWidth / 2.0F -
                                  reloadWidth / 2.0F,
                  ammoPanelY + ammoPanelHeight + 7.0F, 20.0F, ORANGE);
    }
}

bool Player::TakeDamage(Vector2 damageSource) {
    if (IsDead() || IsInvincible()) {
        return false;
    }
    --health_;
    if (character_) animator_.TriggerHurt();
    if (IsDead()) {
        defeatAnimationTime_ = 0.0F;
        hurtInvincibilityTimer_ = 0.0F;
        dodgeTimer_ = 0.0F;
        firing_ = false;
        velocity_ = {};
        position_.y = GameConfig::kFloorY - kHitboxHeight / 2.0F;
        return true;
    }
    hurtInvincibilityTimer_ = kHurtInvincibilityDuration;
    velocity_.x = (position_.x >= damageSource.x ? 1.0F : -1.0F) * 430.0F;
    velocity_.y = -260.0F;
    return true;
}

Vector2 Player::Position() const {
    return position_;
}

Rectangle Player::Hitbox() const {
    return {position_.x - kHitboxWidth / 2.0F,
            position_.y - kHitboxHeight / 2.0F,
            kHitboxWidth, kHitboxHeight};
}

Rectangle Player::ProjectileHitbox() const {
    constexpr float width = 24.0F;
    constexpr float height = 36.0F;
    return {position_.x - width / 2.0F,
            position_.y - height / 2.0F,
            width, height};
}

int Player::FacingDirection() const {
    return facingDirection_;
}

bool Player::IsDead() const {
    return health_ <= 0;
}

bool Player::IsInvincible() const {
    return dodgeTimer_ > 0.0F || hurtInvincibilityTimer_ > 0.0F;
}

bool Player::DefeatAnimationFinished() const {
    return defeatAnimationTime_ >= 0.92F;
}

OperatorKind Player::Kind() const {
    return operatorKind_;
}
