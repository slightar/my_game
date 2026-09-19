#pragma once
#include <array>

enum class EnemyKind { Slug, Soldier, Crossbow, Exploder, Shield, Drone, Count };
struct EnemyDefinition {
    EnemyKind kind;
    const char* id;
    const char* name;
    const char* tactic;
    float health, speed, width, height, reach, windup, recovery, spriteSize;
};
const std::array<EnemyDefinition, 6>& EnemyDefinitions();
const EnemyDefinition& EnemyData(EnemyKind kind);
