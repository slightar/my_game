#pragma once

#include "game_types.h"

#include <vector>

class AudioSystem;
class CharacterArt;
class UiFont;

class Player {
public:
    void Reset(OperatorKind operatorKind = OperatorKind::Exusiai);
    void Update(float deltaTime, Vector2 enemyPosition, float enemyRadius,
                std::vector<Bullet>& bullets, AudioSystem& audio);
    void UpdateDefeatAnimation(float deltaTime);
    void Draw(const CharacterArt& art) const;
    void DrawHud(const UiFont& font, const CharacterArt& art,
                 const char* operatorName) const;

    bool TakeDamage(Vector2 damageSource);
    [[nodiscard]] Vector2 Position() const;
    [[nodiscard]] Rectangle Hitbox() const;
    [[nodiscard]] Rectangle ProjectileHitbox() const;
    [[nodiscard]] int FacingDirection() const;
    [[nodiscard]] bool IsDead() const;
    [[nodiscard]] bool IsInvincible() const;
    [[nodiscard]] bool DefeatAnimationFinished() const;
    [[nodiscard]] OperatorKind Kind() const;

private:
    static constexpr float kHitboxWidth = 38.0F;
    static constexpr float kHitboxHeight = 78.0F;
    static constexpr int kMaxHealth = 3;
    static constexpr int kMagazineCapacity = 35;
    static constexpr int kMaxDodgeCharges = 1;
    static constexpr int kMaxSwordWaveCharges = 6;

    void UpdateTexasCombat(float deltaTime, Vector2 enemyPosition,
                           float enemyRadius, std::vector<Bullet>& bullets);

    Vector2 position_{};
    Vector2 velocity_{};
    int facingDirection_ = 1;
    int jumpCount_ = 0;
    float jumpHoldTimer_ = 0.0F;
    float animationTime_ = 0.0F;
    float attackAnimationTime_ = 0.0F;
    float defeatAnimationTime_ = 0.0F;
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

    OperatorKind operatorKind_ = OperatorKind::Exusiai;
    int swordWaveCharges_ = kMaxSwordWaveCharges;
    float swordWaveRechargeTimer_ = 0.0F;
    bool texasRainMode_ = false;
    float texasRainBurstTimer_ = 0.0F;
    float texasAttackEffectTimer_ = 0.0F;
    float swordRainTimer_ = 0.0F;
    float swordRainCooldown_ = 0.0F;
    float swordRainSpawnTimer_ = 0.0F;

    float barrageTimer_ = 0.0F;
    float barrageCooldown_ = 0.0F;
    float overloadTimer_ = 0.0F;
    float overloadCooldown_ = 0.0F;
};
