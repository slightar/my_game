#pragma once

#include "raylib.h"

#include <vector>

class UiFont;

class Boss {
public:
    Boss();
    ~Boss();

    Boss(const Boss&) = delete;
    Boss& operator=(const Boss&) = delete;

    void Reset();
    void Update(float deltaTime, Vector2 playerPosition,
                int playerFacingDirection);
    void Draw(const UiFont& font) const;
    void DrawHud(const UiFont& font) const;

    void TakeDamage(float damage);
    void Stun(float duration);
    bool DestroyProjectileAt(Vector2 position, float radius);
    [[nodiscard]] Vector2 Position() const;
    [[nodiscard]] float Radius() const;
    [[nodiscard]] bool AttackHits(Rectangle playerHitbox,
                                  Rectangle projectileHitbox);
    [[nodiscard]] bool CanDealContactDamage() const;
    [[nodiscard]] bool IsDefeated() const;

private:
    enum class State {
        Idle,
        MeleeWindup,
        Melee,
        RushTeleportOut,
        RushTeleportIn,
        RushWindup,
        Rush,
        AirSlashWindup,
        AirSlash,
        DartWindup,
        Recover,
        Defeated
    };

    struct Dart {
        Vector2 position{};
        Vector2 velocity{};
        float rotation = 0.0F;
    };

    void StartMelee(Vector2 playerPosition);
    void StartRush(Vector2 playerPosition, int playerFacingDirection);
    void StartAirSlash(Vector2 playerPosition);
    void StartDartVolley(Vector2 playerPosition);
    void LaunchDarts();
    void UpdateDarts(float deltaTime);
    void DrawDarts() const;
    void DrawBattleSprite() const;
    void DrawFallbackBody() const;

    static constexpr float kRadius = 34.0F;
    static constexpr float kMaxHealth = 60.0F;

    Vector2 position_{};
    Vector2 slashStart_{};
    Vector2 slashTarget_{};
    State state_ = State::Idle;
    float stateTimer_ = 0.0F;
    int facingDirection_ = -1;
    float health_ = kMaxHealth;
    int nextRangedAttack_ = 0;
    float animationTime_ = 0.0F;
    float rushTeleportTargetX_ = 0.0F;
    float stunTimer_ = 0.0F;
    Texture2D battleSprite_{};
    std::vector<Dart> darts_;
};
