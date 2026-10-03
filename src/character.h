#pragma once

#include "game_types.h"
#include <array>
#include <string>
#include <vector>
#include <map>

struct CharacterPart {
    std::string image;
    float x = 0, y = 0; // Joint position, in character-height units from feet.
    float height = 0.3F;
    float pivotX = 0.5F, pivotY = 0.1F;
};
CharacterPart DefaultCharacterPart(const std::string& slot);

// Persistent, renderer-independent character data. No textures or live timers in JSON.
struct CharacterAssets {
    std::string portrait;
    std::string sprite;
    int columns = 1;
    int rows = 1;
    std::vector<int> rowFrameCounts; // Optional per-row limits; absent rows use columns.
    int idleRow = 0;
    int runRow = -1;
    int attackRow = -1;
    int defeatedRow = -1;
    float fps = 8.0F;
    float height = 112.0F;
    std::string animationMode = "auto"; // auto, procedural, frames, rig
    float motionStrength = 1.0F;
    int jumpRow = -1, fallRow = -1, dodgeRow = -1, hurtRow = -1;
    std::map<std::string, CharacterPart> parts;
};

struct CharacterStats {
    int health = 3;
    float moveSpeed = 320.0F;
    float jumpSpeed = 610.0F;
    int jumps = 2;
    float attackInterval = 0.25F;
};

enum class EffectType { Projectile, Melee, Rain, Heal, Buff };

struct SkillEffect {
    EffectType type = EffectType::Projectile;
    DamageType damageType = DamageType::Physical;
    float amount = 1.0F;
    float speed = 900.0F;
    float radius = 8.0F;
    float lifetime = 1.4F;
    float range = 110.0F;
    float stun = 0.0F;
    int count = 1;
    float spread = 0.12F;
    bool destroysProjectiles = false;
    float damageMultiplier = 1.0F;
    float attackSpeedMultiplier = 1.0F;
};

struct CharacterSkill {
    std::string name;
    std::string description;
    std::string icon;
    float cooldown = 8.0F;
    float duration = 0.0F;
    // Zero: apply once on activation. Positive: repeat throughout duration.
    float interval = 0.0F;
    std::vector<SkillEffect> effects;
};

class Character {
public:
    std::string id;
    std::string name;
    std::string description;
    CharacterAssets assets;
    CharacterStats stats;
    SkillEffect attack;
    std::vector<CharacterSkill> skills; // E, Q; missing slots are disabled.

    [[nodiscard]] static Character FromJson(const std::string& json);
    [[nodiscard]] std::string ToJson() const;
    void Validate() const;
};

struct SkillState {
    float cooldown = 0;
    float remaining = 0;
    float nextPulse = 0;
};

// One runtime per spawned player. A Character can be shared without sharing state.
class CharacterRuntime {
public:
    explicit CharacterRuntime(Character character);
    [[nodiscard]] const Character& Definition() const { return character_; }
    [[nodiscard]] const std::array<SkillState, 2>& Skills() const { return skills_; }
    [[nodiscard]] bool PerformedAction() const { return performedAction_; }
    [[nodiscard]] bool PerformedAttack() const { return performedAttack_; }
    void Update(float dt, Vector2 position, int facing, Vector2 target,
                bool attacking, std::array<bool, 2> activated,
                int& health, std::vector<Bullet>& bullets);
private:
    void Apply(const SkillEffect& effect, Vector2 position, int facing,
               Vector2 target, float damageMultiplier, int& health,
               std::vector<Bullet>& bullets);
    Character character_;
    std::array<SkillState, 2> skills_{};
    float attackCooldown_ = 0;
    bool performedAction_ = false;
    bool performedAttack_ = false;
};
