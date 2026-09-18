#pragma once

#include "raylib.h"

namespace GameConfig {

inline constexpr int kScreenWidth = 1280;
inline constexpr int kScreenHeight = 720;
inline constexpr Rectangle kRoom{48.0F, 48.0F, 2400.0F, 624.0F};
inline constexpr Rectangle kSafeRoom{48.0F, 48.0F, 720.0F, 624.0F};
inline constexpr Rectangle kBossArena{900.0F, 48.0F, 1548.0F, 624.0F};
inline constexpr float kBossGateX = 874.0F;
inline constexpr float kBossGateWidth = 34.0F;
inline constexpr float kBossTriggerX = 1035.0F;
inline constexpr float kFloorY = kRoom.y + kRoom.height - 28.0F;

}  // namespace GameConfig

enum class OperatorKind {
    Exusiai,
    Texas,
    Custom
};

enum class BulletKind {
    Gun,
    SwordWave,
    MeleeSlash,
    FallingSword
};

enum class DamageType {
    Physical,
    Arts
};

struct Bullet {
    Vector2 position{};
    Vector2 velocity{};
    float lifetime = 0.0F;
    float radius = 4.0F;
    float damage = 1.0F;
    BulletKind kind = BulletKind::Gun;
    DamageType damageType = DamageType::Physical;
    bool destroysEnemyProjectile = false;
    float stunDuration = 0.0F;
    float activationDelay = 0.0F;
    int visualVariant = 0;
    int facingDirection = 0;
    bool hasHit = false;
};
