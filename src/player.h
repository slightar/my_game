#pragma once

#include "game_types.h"

#include <vector>

class AudioSystem;
class CharacterArt;
class UiFont;

class Player {
public:
    void Reset();
    void Update(float deltaTime, std::vector<Bullet>& bullets, AudioSystem& audio);
    void Draw(const CharacterArt& art) const;
    void DrawHud(const UiFont& font, const char* operatorName) const;

    bool TakeDamage(Vector2 damageSource);
    [[nodiscard]] Vector2 Position() const;
    [[nodiscard]] float Radius() const;
    [[nodiscard]] bool IsDead() const;
    [[nodiscard]] bool IsInvincible() const;

private:
    static constexpr float kRadius = 24.0F;
    static constexpr int kMaxHealth = 3;
    static constexpr int kMagazineCapacity = 35;
    static constexpr int kMaxDodgeCharges = 1;

    Vector2 position_{};
    Vector2 velocity_{};
    int facingDirection_ = 1;
    int jumpCount_ = 0;
    float jumpHoldTimer_ = 0.0F;
    float animationTime_ = 0.0F;
    float attackAnimationTime_ = 0.0F;
    bool firing_ = false;

    int health_ = kMaxHealth;
    float hurtInvincibilityTimer_ = 0.0F;

    int dodgeCharges_ = kMaxDodgeCharges;
    int dodgeDirection_ = 1;
    float dodgeTimer_ = 0.0F;
    float dodgeRechargeTimer_ = 0.0F;

    int ammo_ = kMagazineCapacity;
    float shotCooldown_ = 0.0F;
    float reloadTimer_ = 0.0F;
    bool reloading_ = false;
};
