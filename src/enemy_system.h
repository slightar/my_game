#pragma once
#include "enemy_data.h"
#include "game_types.h"
#include <vector>

enum class EnemyState { Approach, Windup, Strike, Recover, Fuse, Blast, Dead };
struct EnemyUnit {
    unsigned id = 0;
    EnemyKind kind = EnemyKind::Slug;
    Vector2 feet{};
    int facing = -1;
    float health = 0, timer = 0, animationTime = 0, stun = 0, flash = 0;
    bool moving = false, hitConsumed = false, blocked = false;
    bool turning = false;
    EnemyState state = EnemyState::Approach;
    Vector2 aim{};
    Rectangle Hitbox() const;
    Vector2 Center() const;
    bool Targetable() const;
    bool Guarding() const;
};
struct EnemyBolt {
    Vector2 position{}, previous{}, velocity{};
    float lifetime = 3;
};
struct EnemyTarget { Vector2 position; float radius; };

// Simulation owns no GPU/audio resources, so attack rules can be tested headlessly.
class EnemySystem {
public:
    static constexpr float kBlastRadius = 140;
    void Clear();
    void ResetTrial();
    unsigned Spawn(EnemyKind kind, float x);
    void Update(float dt, Vector2 playerPosition);
    bool ResolveBullet(Bullet& bullet, Vector2 previousPosition);
    bool DestroyBolt(Vector2 position, float radius);
    bool AttackHits(Rectangle body, Rectangle projectileBody, Vector2& source);
    EnemyTarget Target(Vector2 player, int facing) const;
    bool Damage(unsigned id, float damage, DamageType type, Vector2 source, float stun = 0);
    bool Cleared() const;
    int Remaining() const;
    const std::vector<EnemyUnit>& Units() const { return units_; }
    const std::vector<EnemyBolt>& Bolts() const { return bolts_; }
private:
    void Tick(float dt, Vector2 player);
    std::vector<EnemyUnit> units_;
    std::vector<EnemyBolt> bolts_;
    unsigned nextId_ = 1;
};
