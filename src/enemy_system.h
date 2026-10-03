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
    int attacks = 0;
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
    bool cold = false, shatter = false;
    float gravity = 0, blastRadius = 0;
    Vector2 landing{};
};
struct EnemyImpact { bool cold = false, shatter = false; int frozenDamage = 2; };
struct EnemyTarget { Vector2 position; float radius; };

// Simulation owns no GPU/audio resources, so attack rules can be tested headlessly.
class EnemySystem {
public:
    static constexpr float kBlastRadius = 140;
    static constexpr float kEntryX = 2260.0F;
    static constexpr float kFirstSpawnDelay = 1.2F;
    static constexpr float kSpawnInterval = 2.4F;
    // A cloaked unit drops its cloak once the operator is within this distance. Exposed
    // so tests and the renderer agree on the number instead of duplicating it.
    static constexpr float kCloakRevealRange = 220.0F;
    // How long 辐能源石虫's parting gift lasts, and how much faster it makes friends.
    static constexpr float kEnergizeDuration = 3.5F;
    static constexpr float kEnergizeSpeedScale = 1.5F;
    void Clear();
    void ResetTrial();
    void ResetEncounter(std::vector<EnemyKind> wave,float entryX,float left,float right);
    float EntryX() const {return entryX_;}
    unsigned Spawn(EnemyKind kind, float x);
    void Update(float dt, Vector2 playerPosition);
    bool ResolveBullet(Bullet& bullet, Vector2 previousPosition);
    bool DestroyBolt(Vector2 position, float radius);
    // `corrosive` is set when the hit that landed came from an acid bolt, so the caller can
    // apply the corrosion debuff to the operator.
    bool AttackHits(Rectangle body, Rectangle projectileBody, Vector2& source,
                    bool* corrosive = nullptr, EnemyImpact* impact = nullptr);
    EnemyTarget Target(Vector2 player, int facing) const;
    bool Damage(unsigned id, float damage, DamageType type, Vector2 source, float stun = 0);
    bool Cleared() const;
    int Remaining() const;
    int Pending() const { return static_cast<int>(spawnQueue_.size() - nextSpawn_); }
    bool HasEntry() const { return !spawnQueue_.empty(); }
    float EntryPulse() const { return entryPulse_; }
    float NextSpawnIn() const { return Pending() ? spawnTimer_ : 0.0F; }
    const std::vector<EnemyUnit>& Units() const { return units_; }
    const std::vector<EnemyBolt>& Bolts() const { return bolts_; }
private:
    void Tick(float dt, Vector2 player);
    float entryX_=kEntryX,leftBound_=GameConfig::kBossGateX+55,rightBound_=GameConfig::kRoom.x+GameConfig::kRoom.width-65;
    std::vector<EnemyUnit> units_;
    std::vector<EnemyBolt> bolts_;
    unsigned nextId_ = 1;
    std::vector<EnemyKind> spawnQueue_;
    std::size_t nextSpawn_ = 0;
    float spawnTimer_ = 0, entryPulse_ = 0;
};
