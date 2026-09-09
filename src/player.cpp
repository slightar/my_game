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

}  // namespace

void Player::Reset() {
    position_ = {240.0F, GameConfig::kFloorY - kHitboxHeight / 2.0F};
    velocity_ = {};
    facingDirection_ = 1;
    jumpCount_ = 0;
    jumpHoldTimer_ = 0.0F;
    animationTime_ = 0.0F;
    attackAnimationTime_ = 0.0F;
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
}

void Player::Update(float deltaTime, std::vector<Bullet>& bullets, AudioSystem& audio) {
    hurtInvincibilityTimer_ = std::max(0.0F, hurtInvincibilityTimer_ - deltaTime);
    animationTime_ += deltaTime;
    if (IsDead()) {
        return;
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
        const float spread = static_cast<float>(GetRandomValue(-1000, 1000)) /
                             1000.0F * kBulletSpreadRadians;
        bullets.push_back({
            {position_.x + direction * (kHitboxWidth / 2.0F + 30.0F),
             position_.y - 16.0F},
            {direction * std::cos(spread) * kBulletSpeed,
             std::sin(spread) * kBulletSpeed},
            kBulletLifetime, kBulletRadius, 1});
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
                              ? std::abs(walkWave) * 5.0F
                              : 0.0F;
    const Vector2 feetPosition{position_.x,
                               position_.y + kHitboxHeight / 2.0F - walkBob};

    float rotation = 0.0F;
    float horizontalScale = 1.0F;
    float verticalScale = 1.0F;
    if (dodgeTimer_ > 0.0F) {
        rotation = -static_cast<float>(facingDirection_) * 11.0F;
        horizontalScale = 1.09F;
        verticalScale = 0.91F;
    } else if (airborne) {
        const bool rising = velocity_.y < 0.0F;
        rotation = static_cast<float>(facingDirection_) * (rising ? -7.0F : 6.0F);
        horizontalScale = rising ? 0.94F : 1.06F;
        verticalScale = rising ? 1.08F : 0.95F;
    } else if (moving) {
        rotation = walkWave * 3.3F;
        horizontalScale = 1.0F + std::abs(walkWave) * 0.025F;
        verticalScale = 1.0F - std::abs(walkWave) * 0.035F;
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
            if (art.HasBattleChibi()) {
                art.DrawBattleChibi(trailPosition, facingDirection_, 120.0F,
                                    BattleChibiAnimation::Idle,
                                    animationTime_, Fade(WHITE, 0.11F));
            } else if (art.HasChibi()) {
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

    if (IsInvincible()) {
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
    if (art.HasBattleChibi()) {
        art.DrawBattleChibi(
            feetPosition, facingDirection_, 120.0F,
            firing_ ? BattleChibiAnimation::Attack
                    : BattleChibiAnimation::Idle,
            firing_ ? attackAnimationTime_ : animationTime_,
            blinkOff ? Fade(WHITE, 0.3F) : WHITE,
            rotation, horizontalScale, verticalScale);
    } else if (art.HasChibi()) {
        art.DrawChibi(feetPosition, facingDirection_,
                      106.0F, animation, animationTime_,
                      blinkOff ? Fade(WHITE, 0.3F) : WHITE);
    } else {
        const Vector2 gunStart{position_.x, position_.y - 10.0F};
        const Vector2 gunEnd{
            position_.x + static_cast<float>(facingDirection_) * 48.0F,
            position_.y - 10.0F};
        DrawLineEx(gunStart, gunEnd, 9.0F, DARKGRAY);
        const Rectangle body{position_.x - 16.0F, position_.y - 28.0F,
                             32.0F, 62.0F};
        DrawRectangleRec(body, blinkOff ? Fade(BLACK, 0.3F) : BLACK);
    }
}

void Player::DrawHud(const UiFont& font, const char* operatorName) const {
    const char* operatorText = TextFormat("当前干员：%s", operatorName);
    font.Draw(operatorText, 72.0F, 42.0F, 18.0F, RAYWHITE);
    DrawRectangle(70, 70, 420, 150, Fade(BLACK, 0.72F));
    font.Draw("A/D 移动   W/K/空格 二段跳", 88.0F, 82.0F, 17.0F, RAYWHITE);
    font.Draw("按住 J 射击并锁定朝向   R 换弹", 88.0F, 108.0F, 17.0F, RAYWHITE);
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

    const char* ammoText = TextFormat("冲锋枪  %02i / %02i", ammo_, kMagazineCapacity);
    const float textWidth = font.Measure(ammoText, 30.0F);
    DrawRectangle(GameConfig::kScreenWidth - static_cast<int>(textWidth) - 76,
                  72, static_cast<int>(textWidth) + 38, 52,
                  Fade(BLACK, 0.78F));
    font.Draw(ammoText,
              static_cast<float>(GameConfig::kScreenWidth) - textWidth - 57.0F,
              82.0F, 30.0F, RAYWHITE);

    if (reloading_) {
        const char* reloadText = "换弹中……";
        const float reloadWidth = font.Measure(reloadText, 24.0F);
        font.Draw(reloadText,
                  static_cast<float>(GameConfig::kScreenWidth) / 2.0F -
                      reloadWidth / 2.0F,
                  82.0F, 24.0F, MAROON);
    }
}

bool Player::TakeDamage(Vector2 damageSource) {
    if (IsDead() || IsInvincible()) {
        return false;
    }
    --health_;
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

bool Player::IsDead() const {
    return health_ <= 0;
}

bool Player::IsInvincible() const {
    return dodgeTimer_ > 0.0F || hurtInvincibilityTimer_ > 0.0F;
}
