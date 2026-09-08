#pragma once

#include "raylib.h"

#include <vector>

class UiFont;

class Boss {
public:
    void Reset();
    void Update(float deltaTime, Vector2 playerPosition);
    void Draw(const UiFont& font) const;
    void DrawHud(const UiFont& font) const;

    void TakeDamage(int damage);
    [[nodiscard]] Vector2 Position() const;
    [[nodiscard]] float Radius() const;
    [[nodiscard]] bool AttackHits(Vector2 playerPosition, float playerRadius);
    [[nodiscard]] bool IsDefeated() const;

private:
    enum class State {
        Idle,
        MeleeWindup,
        Melee,
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
    void StartRush(Vector2 playerPosition);
    void StartAirSlash(Vector2 playerPosition);
    void StartDartVolley(Vector2 playerPosition);
    void LaunchDarts();
    void UpdateDarts(float deltaTime);
    void DrawDarts() const;

    static constexpr float kRadius = 34.0F;
    static constexpr int kMaxHealth = 60;

    Vector2 position_{};
    Vector2 slashStart_{};
    Vector2 slashTarget_{};
    State state_ = State::Idle;
    float stateTimer_ = 0.0F;
    int facingDirection_ = -1;
    int health_ = kMaxHealth;
    int nextRangedAttack_ = 0;
    std::vector<Dart> darts_;
};
