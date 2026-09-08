#include "player.h"

#include "audio_system.h"
#include "ui_font.h"

#include <algorithm>

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

}  // namespace

void Player::Reset() {
    position_ = {240.0F, GameConfig::kFloorY - kRadius};
    velocity_ = {};
    facingDirection_ = 1;
    jumpCount_ = 0;
    jumpHoldTimer_ = 0.0F;
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

    const float leftLimit = GameConfig::kRoom.x + 28.0F + kRadius;
    const float rightLimit =
        GameConfig::kRoom.x + GameConfig::kRoom.width - 28.0F - kRadius;
    position_.x = std::clamp(position_.x, leftLimit, rightLimit);

    const float ceilingLimit = GameConfig::kRoom.y + 28.0F + kRadius;
    if (position_.y < ceilingLimit) {
        position_.y = ceilingLimit;
        velocity_.y = 0.0F;
    }
    if (position_.y + kRadius >= GameConfig::kFloorY) {
        position_.y = GameConfig::kFloorY - kRadius;
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
        bullets.push_back({
            {position_.x + direction * (kRadius + 24.0F), position_.y - 2.0F},
            {direction * kBulletSpeed, 0.0F}, kBulletLifetime, 4.0F, 1});
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
}

void Player::Draw() const {
    if (dodgeTimer_ > 0.0F) {
        for (int trail = 1; trail <= 3; ++trail) {
            DrawCircleV(
                {position_.x - static_cast<float>(dodgeDirection_ * trail) * 18.0F,
                 position_.y},
                kRadius - static_cast<float>(trail * 3), Fade(BLACK, 0.16F));
        }
    }

    const Vector2 gunStart{position_.x, position_.y - 2.0F};
    const Vector2 gunEnd{position_.x + static_cast<float>(facingDirection_) * 48.0F,
                         position_.y - 2.0F};
    DrawLineEx(gunStart, gunEnd, 9.0F, DARKGRAY);

    if (IsInvincible()) {
        DrawCircleV(position_, kRadius + 7.0F, dodgeTimer_ > 0.0F ? SKYBLUE : ORANGE);
    }

    const bool blinkOff = hurtInvincibilityTimer_ > 0.0F &&
                          static_cast<int>(hurtInvincibilityTimer_ * 14.0F) % 2 == 0;
    DrawCircleV(position_, kRadius, blinkOff ? Fade(BLACK, 0.3F) : BLACK);
}

void Player::DrawHud(const UiFont& font) const {
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

float Player::Radius() const {
    return kRadius;
}

bool Player::IsDead() const {
    return health_ <= 0;
}

bool Player::IsInvincible() const {
    return dodgeTimer_ > 0.0F || hurtInvincibilityTimer_ > 0.0F;
}
