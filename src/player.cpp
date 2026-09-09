#include "player.h"

#include "audio_system.h"
#include "character_art.h"
#include "ui_font.h"

#include <algorithm>
#include <cmath>

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

}  // namespace

void Player::Reset(OperatorKind operatorKind) {
    operatorKind_ = operatorKind;
    position_ = {240.0F, GameConfig::kFloorY - kHitboxHeight / 2.0F};
    velocity_ = {};
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

void Player::Update(float deltaTime, Vector2 enemyPosition, float enemyRadius,
                    std::vector<Bullet>& bullets, AudioSystem& audio) {
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
    if (operatorKind_ == OperatorKind::Exusiai && IsKeyPressed(KEY_E) &&
        barrageTimer_ <= 0.0F &&
        barrageCooldown_ <= 0.0F) {
        barrageTimer_ = kBarrageDuration;
    }
    if (operatorKind_ == OperatorKind::Exusiai && IsKeyPressed(KEY_Q) &&
        overloadTimer_ <= 0.0F &&
        overloadCooldown_ <= 0.0F) {
        overloadTimer_ = kOverloadDuration;
    }

    float moveDirection = 0.0F;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        moveDirection -= 1.0F;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        moveDirection += 1.0F;
    }

    if (moveDirection != 0.0F && !IsKeyDown(KEY_J)) {
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

    const bool dodgePressed = IsKeyPressed(KEY_S) || IsKeyPressed(KEY_L) ||
                              IsKeyPressed(KEY_LEFT_SHIFT) ||
                              IsKeyPressed(KEY_RIGHT_SHIFT);
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
        velocity_.x = moveDirection * kMoveSpeed;
    }

    const bool jumpPressed = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_K) ||
                             IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP);
    const bool jumpHeld = IsKeyDown(KEY_W) || IsKeyDown(KEY_K) ||
                          IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP);
    if (jumpPressed && jumpCount_ < kMaxJumps && dodgeTimer_ <= 0.0F) {
        velocity_.y = -kJumpSpeed;
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

    const float leftLimit = GameConfig::kRoom.x + 28.0F + kHitboxWidth / 2.0F;
    const float rightLimit =
        GameConfig::kRoom.x + GameConfig::kRoom.width - 28.0F -
        kHitboxWidth / 2.0F;
    position_.x = std::clamp(position_.x, leftLimit, rightLimit);

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
    if (operatorKind_ == OperatorKind::Texas) {
        UpdateTexasCombat(deltaTime, enemyPosition, enemyRadius, bullets);
        return;
    }
    if (IsKeyPressed(KEY_R) && ammo_ < kMagazineCapacity && !reloading_) {
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

    if (IsKeyDown(KEY_J) && !reloading_ && ammo_ > 0 && shotCooldown_ <= 0.0F &&
        dodgeTimer_ <= 0.0F) {
        const float direction = static_cast<float>(facingDirection_);
        const Vector2 muzzle{
            position_.x + direction * (kHitboxWidth / 2.0F + 30.0F),
            position_.y - 16.0F};
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
            const float angle = fanOffset + randomSpread;
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
    } else if (IsKeyPressed(KEY_J) && reloading_) {
        audio.PlayEmptyClick();
    }

    if (ammo_ == 0 && !reloading_) {
        reloading_ = true;
        reloadTimer_ = kReloadDuration;
        audio.PlayReload();
    }

    firing_ = IsKeyDown(KEY_J) && !reloading_ && ammo_ > 0 &&
              dodgeTimer_ <= 0.0F;
    if (firing_) {
        attackAnimationTime_ += deltaTime;
    } else {
        attackAnimationTime_ = 0.0F;
    }
}

void Player::UpdateTexasCombat(float deltaTime, Vector2 enemyPosition,
                               float enemyRadius,
                               std::vector<Bullet>& bullets) {
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

    if (IsKeyPressed(KEY_E) && texasRainBurstTimer_ <= 0.0F) {
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
    if (IsKeyPressed(KEY_Q) && swordRainTimer_ <= 0.0F &&
        swordRainCooldown_ <= 0.0F) {
        swordRainTimer_ = kSwordRainDuration;
        swordRainSpawnTimer_ = 0.0F;
    }

    if (IsKeyDown(KEY_J) && shotCooldown_ <= 0.0F && dodgeTimer_ <= 0.0F) {
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
                    {}, 0.12F + static_cast<float>(strike) * 0.035F,
                    range * 0.53F,
                    texasRainMode_ ? kTexasRainHitDamage : kTexasMeleeDamage,
                    BulletKind::MeleeSlash,
                    texasRainMode_ ? DamageType::Arts : DamageType::Physical,
                    false, 0.0F,
                    texasRainMode_ ? static_cast<float>(strike) * 0.10F
                                   : 0.0F,
                    strike});
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
        shotCooldown_ = kTexasAttackInterval;
        texasAttackEffectTimer_ = 0.19F;
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
}

void Player::Draw(const CharacterArt& art) const {
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
    const float walkWave = std::sin(animationTime_ * 14.0F);
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
        const float pulse = 0.5F + 0.5F * std::sin(animationTime_ * 7.5F);
        DrawEllipse(static_cast<int>(position_.x),
                    static_cast<int>(GameConfig::kFloorY + 1.0F),
                    84.0F + pulse * 5.0F, 14.0F + pulse * 2.0F,
                    Fade(Color{132, 7, 18, 255},
                         texasRainMode_ ? 0.17F : 0.08F));
        DrawEllipse(static_cast<int>(position_.x),
                    static_cast<int>(GameConfig::kFloorY),
                    64.0F, 10.0F, Fade(BLACK, 0.28F));

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
                       art.HasBattleChibi()) {
                art.DrawBattleChibi(trailPosition, facingDirection_, 120.0F,
                                    BattleChibiAnimation::Idle,
                                    animationTime_, Fade(WHITE, 0.11F));
            } else if (operatorKind_ != OperatorKind::Texas && art.HasChibi()) {
                art.DrawChibi(trailPosition, facingDirection_, 106.0F,
                              animation, animationTime_, Fade(WHITE, 0.11F));
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
    } else if (operatorKind_ != OperatorKind::Texas && art.HasBattleChibi()) {
        const BattleChibiAnimation battleAnimation =
            IsDead() ? BattleChibiAnimation::Defeated
                     : (firing_ ? BattleChibiAnimation::Attack
                                : BattleChibiAnimation::Idle);
        art.DrawBattleChibi(
            feetPosition, facingDirection_, 120.0F,
            battleAnimation,
            IsDead() ? defeatAnimationTime_
                     : (firing_ ? attackAnimationTime_ : animationTime_),
            !IsDead() && blinkOff ? Fade(WHITE, 0.3F) : WHITE);
    } else if (operatorKind_ != OperatorKind::Texas && art.HasChibi()) {
        art.DrawChibi(feetPosition, facingDirection_,
                      106.0F, animation, animationTime_,
                      blinkOff ? Fade(WHITE, 0.3F) : WHITE);
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
                     const char* operatorName) const {
    const char* operatorText = TextFormat("当前干员：%s", operatorName);
    font.Draw(operatorText, 72.0F, 42.0F, 18.0F, RAYWHITE);
    DrawRectangle(70, 70, 420, 150, Fade(BLACK, 0.72F));
    font.Draw("A/D 移动   W/K/空格 二段跳", 88.0F, 82.0F, 17.0F, RAYWHITE);
    font.Draw(operatorKind_ == OperatorKind::Texas
                  ? "按住 J 近战/剑气并锁定朝向"
                  : "按住 J 射击并锁定朝向   R 换弹",
              88.0F, 108.0F, 17.0F, RAYWHITE);
    font.Draw("S/L/Shift 闪避", 88.0F, 134.0F, 17.0F, RAYWHITE);

    font.Draw("生命", 88.0F, 170.0F, 18.0F, RAYWHITE);
    for (int heart = 0; heart < kMaxHealth; ++heart) {
        const Rectangle healthBlock{125.0F + static_cast<float>(heart) * 32.0F,
                                    169.0F, 24.0F, 20.0F};
        DrawRectangleRec(healthBlock, heart < health_ ? RED : Color{70, 76, 86, 255});
        DrawRectangleLinesEx(healthBlock, 2.0F, RAYWHITE);
    }

    font.Draw("闪避", 238.0F, 170.0F, 16.0F, RAYWHITE);
    for (int charge = 0; charge < kMaxDodgeCharges; ++charge) {
        const Rectangle segment{306.0F + static_cast<float>(charge) * 40.0F,
                                169.0F, 32.0F, 20.0F};
        DrawRectangleRec(segment,
                         charge < dodgeCharges_ ? SKYBLUE : Color{70, 76, 86, 255});
        DrawRectangleLinesEx(segment, 2.0F, RAYWHITE);
    }

    constexpr float ammoPanelX = 70.0F;
    constexpr float ammoPanelY = 234.0F;
    constexpr float ammoPanelWidth = 108.0F;
    constexpr float ammoPanelHeight = 302.0F;
    DrawRectangleRec({ammoPanelX, ammoPanelY, ammoPanelWidth, ammoPanelHeight},
                     Fade(BLACK, 0.76F));
    if (operatorKind_ == OperatorKind::Texas) {
        font.Draw("剑气", ammoPanelX + 37.0F, ammoPanelY + 12.0F,
                  17.0F, RAYWHITE);
        for (int charge = 0; charge < kMaxSwordWaveCharges; ++charge) {
            const float y = ammoPanelY + 238.0F -
                            static_cast<float>(charge) * 34.0F;
            const Rectangle segment{ammoPanelX + 31.0F, y, 46.0F, 25.0F};
            DrawRectangleRec(segment,
                             charge < swordWaveCharges_
                                 ? Color{221, 235, 244, 255}
                                 : Color{55, 61, 69, 255});
            DrawRectangleLinesEx(segment, 2.0F, RAYWHITE);
        }
        if (swordWaveCharges_ < kMaxSwordWaveCharges) {
            const float restoreRatio = std::clamp(
                1.0F - swordWaveRechargeTimer_ / kSwordWaveRechargeDuration,
                0.0F, 1.0F);
            DrawRectangle(static_cast<int>(ammoPanelX + 18.0F),
                          static_cast<int>(ammoPanelY + 274.0F), 72, 6,
                          Color{55, 61, 69, 255});
            DrawRectangle(static_cast<int>(ammoPanelX + 18.0F),
                          static_cast<int>(ammoPanelY + 274.0F),
                          static_cast<int>(72.0F * restoreRatio), 6, RAYWHITE);
        }
    } else {
        font.Draw("冲锋枪", ammoPanelX + 21.0F, ammoPanelY + 10.0F,
                  16.0F, RAYWHITE);
        font.Draw("弹匣", ammoPanelX + 37.0F, ammoPanelY + 34.0F,
                  16.0F, Fade(RAYWHITE, 0.72F));
        const Rectangle ammoBar{ammoPanelX + 38.0F, ammoPanelY + 66.0F,
                                32.0F, 188.0F};
        DrawRectangleRec(ammoBar, Color{61, 66, 74, 255});
        const float ammoRatio = static_cast<float>(ammo_) /
                                static_cast<float>(kMagazineCapacity);
        const float ammoFillHeight = ammoBar.height * ammoRatio;
        DrawRectangleRec({ammoBar.x,
                          ammoBar.y + ammoBar.height - ammoFillHeight,
                          ammoBar.width, ammoFillHeight},
                         reloading_ ? ORANGE : SKYBLUE);
        DrawRectangleLinesEx(ammoBar, 2.0F, RAYWHITE);
        const char* ammoText = TextFormat("%02i/%02i", ammo_, kMagazineCapacity);
        const float ammoTextWidth = font.Measure(ammoText, 17.0F);
        font.Draw(ammoText,
                  ammoPanelX + (ammoPanelWidth - ammoTextWidth) / 2.0F,
                  ammoPanelY + 268.0F, 17.0F, RAYWHITE);
    }

    const auto drawSkillIcon = [&font, &art, this](float x, const char* key,
                                       const char* name, float activeTimer,
                                       float cooldownTimer,
                                       float cooldownDuration, Color color,
                                       int skillIndex,
                                       bool showActiveTimer = true) {
        constexpr float size = 86.0F;
        const Rectangle icon{x, 552.0F, size, size};
        const bool active = activeTimer > 0.0F;
        const bool coolingDown = !active && cooldownTimer > 0.0F;
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
        DrawRectangleLinesEx(icon, active ? 4.0F : 2.0F,
                             active ? color : RAYWHITE);

        DrawRectangleRec({icon.x + 5.0F, icon.y + 5.0F, 22.0F, 22.0F},
                         Fade(BLACK, 0.72F));
        font.Draw(key, icon.x + 10.0F, icon.y + 6.0F, 17.0F, RAYWHITE);
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
                         Fade(BLACK, 0.62F));
        font.Draw(name, icon.x + 8.0F, icon.y + 65.0F, 14.0F, RAYWHITE);
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
        drawSkillIcon(1064.0F, "E", texasRainMode_ ? "阵雨连绵" : "初始",
                      texasRainMode_ ? 1.0F : 0.0F, 0.0F, 1.0F,
                      Color{207, 45, 48, 255}, 0, false);
        drawSkillIcon(1156.0F, "Q", "剑雨", swordRainTimer_,
                      swordRainCooldown_, kSwordRainCooldownDuration,
                      Color{230, 235, 242, 255}, 1);
    } else {
        drawSkillIcon(1064.0F, "E", "扫射", barrageTimer_, barrageCooldown_,
                      kBarrageCooldownDuration, SKYBLUE, 0);
        drawSkillIcon(1156.0F, "Q", "过载", overloadTimer_, overloadCooldown_,
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
