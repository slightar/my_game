#pragma once

#include "game_types.h"

#include <vector>
#include "character.h"
#include "character_animation.h"
#include <optional>

class AudioSystem;
class CharacterArt;
class UiFont;
class GameSettings;
struct PlayerActions {
    int ranged=0,melee=0,skills=0,dodges=0,jumps=0,reloads=0;
};

class Player {
public:
    void Reset(OperatorKind operatorKind = OperatorKind::Exusiai);
    void Reset(const Character& character);
    void PlaceAt(Vector2 position, int facing);
    void Update(float deltaTime, Vector2 enemyPosition, float enemyRadius,
                std::vector<Bullet>& bullets, AudioSystem& audio, const GameSettings& settings,
                bool assistEnemyAim = false, bool traversalOnly = false, const std::vector<Rectangle>* platforms = nullptr,
                const Rectangle* ramp = nullptr);
    void UpdateDefeatAnimation(float deltaTime);
    void SetHorizontalBounds(float left, float right);
    void Draw(const CharacterArt& art) const;
    void DrawHud(const UiFont& font, const CharacterArt& art,
                 const char* operatorName, const GameSettings* settings = nullptr) const;

    bool TakeDamage(Vector2 damageSource, bool corrosive = false, int amount = 1);
    void ApplyCold();
    [[nodiscard]] bool IsCold() const { return coldTimer_>0; }
    [[nodiscard]] bool IsFrozen() const { return freezeTimer_>0; }
    [[nodiscard]] Vector2 Position() const;
    [[nodiscard]] Rectangle Hitbox() const;
    [[nodiscard]] Rectangle ProjectileHitbox() const;
    [[nodiscard]] int FacingDirection() const;
    [[nodiscard]] bool IsDead() const;
    [[nodiscard]] bool IsInvincible() const;
    // True while 酸液源石虫's corrosion is active, i.e. while post-hit invincibility is
    // halved. Surfaced so the HUD can warn about it.
    [[nodiscard]] bool IsCorroded() const;
    [[nodiscard]] bool DefeatAnimationFinished() const;
    [[nodiscard]] OperatorKind Kind() const;
    [[nodiscard]] int Health() const { return health_; }
    [[nodiscard]] const PlayerActions& Actions() const { return actions_; }

private:
    PlayerActions actions_{};
    std::optional<CharacterRuntime> character_;
    CharacterAnimator animator_;
    static constexpr float kHitboxWidth = 38.0F;
    static constexpr float kHitboxHeight = 78.0F;
    static constexpr int kMaxHealth = 3;
    static constexpr int kMagazineCapacity = 35;
    static constexpr int kMaxDodgeCharges = 1;
    static constexpr int kMaxSwordWaveCharges = 6;

    void UpdateTexasCombat(float deltaTime, Vector2 enemyPosition,
                           float enemyRadius, std::vector<Bullet>& bullets,
                           const GameSettings& settings, AudioSystem& audio);

    float supportGroundY_ = GameConfig::kFloorY;
    Vector2 position_{};
    Vector2 velocity_{};
    float movementLeft_ = 0.0F;
    float movementRight_ = 0.0F;
    int facingDirection_ = 1;
    int jumpCount_ = 0;
    float jumpHoldTimer_ = 0.0F;
    float animationTime_ = 0.0F;
    float attackAnimationTime_ = 0.0F;
    float defeatAnimationTime_ = 0.0F;
    bool firing_ = false;

    int health_ = kMaxHealth;
    float hurtInvincibilityTimer_ = 0.0F;
    // Counts down from kCorrosionDuration after an acid bolt connects.
    float corrosionTimer_ = 0.0F;
    float coldTimer_ = 0.0F, freezeTimer_ = 0.0F;

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
    float texasAttackAnimationTimer_ = 0.0F;
    // 剑雨's post-cast window (drives the HUD timer), its cooldown, and the timer that
    // paces the falling swords.
    float swordRainTimer_ = 0.0F;
    float swordRainCooldown_ = 0.0F;
    float swordRainSpawnTimer_ = 0.0F;

    float barrageTimer_ = 0.0F;
    float barrageCooldown_ = 0.0F;
    float overloadTimer_ = 0.0F;
    float overloadCooldown_ = 0.0F;
};
