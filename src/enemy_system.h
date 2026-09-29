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
    // Spike-proofing for the "walk around it" reward: a guard cannot be back-stabbed
    // during the first moments of an encounter, before it has had a chance to face
    // anything. Counts up while the unit is in Approach.
    float facingTime = 0;
    bool moving = false, hitConsumed = false, blocked = false;
    // Stealth units are cloaked by default. While cloaked they are still fully
    // targetable and meleeable - only ranged projectiles pass through, which is the
    // whole point: the answer is to walk up, not to out-shoot them. Recomputed every
    // tick from the player's distance, and forced off during Windup/Strike so the shot
    // itself is always telegraphed.
    bool cloaked = false;
    // Set by 辐能源石虫's death. While it runs, the unit moves faster - the action-game
    // reading of the original's "使场上敌人获得1点能量".
    float haste = 0;
    EnemyState state = EnemyState::Approach;
    Vector2 aim{};
    Rectangle Hitbox() const;
    Vector2 Center() const;
    bool Targetable() const;
    bool Guarding() const;
    // Ranged projectiles ignore a cloaked unit; melee never does.
    bool InterceptsProjectiles() const;
};
struct EnemyBolt {
    Vector2 position{}, previous{}, velocity{};
    float lifetime = 3;
    // Caster bolts are Arts, so they punch through the shield's physical reduction.
    bool arts = false;
    // 酸液源石虫's spit. On hitting the operator it applies corrosion, which shortens
    // their post-hit invincibility - the game's stand-in for a defence-down debuff.
    bool corrosive = false;
};
struct EnemyTarget { Vector2 position; float radius; };

// Simulation owns no GPU/audio resources, so attack rules can be tested headlessly.
class EnemySystem {
public:
    static constexpr float kBlastRadius = 140;
    // A cloaked unit drops its cloak once the operator is within this distance. Exposed
    // so tests and the renderer agree on the number instead of duplicating it.
    static constexpr float kCloakRevealRange = 220.0F;
    // How long 辐能源石虫's parting gift lasts, and how much faster it makes friends.
    static constexpr float kEnergizeDuration = 3.5F;
    static constexpr float kEnergizeSpeedScale = 1.5F;
    void Clear();
    void ResetTrial();
    unsigned Spawn(EnemyKind kind, float x);
    void Update(float dt, Vector2 playerPosition);
    bool ResolveBullet(Bullet& bullet, Vector2 previousPosition);
    bool DestroyBolt(Vector2 position, float radius);
    // `corrosive` is set when the hit that landed came from an acid bolt, so the caller can
    // apply the corrosion debuff to the operator.
    bool AttackHits(Rectangle body, Rectangle projectileBody, Vector2& source,
                    bool* corrosive = nullptr);
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
