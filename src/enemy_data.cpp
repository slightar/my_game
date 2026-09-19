#include "enemy_data.h"
#include <stdexcept>

const std::array<EnemyDefinition, 6>& EnemyDefinitions() {
    // Prototype values in this action game's units, not the original tower-defense stats.
    // TODO: Tune per-chapter variants and rewards when real story encounters are authored.
    static const std::array<EnemyDefinition, 6> data{{
        {EnemyKind::Slug, "slug", "源石虫", "低矮近战：短前摇扑咬，跳跃或闪避后反击", 6, 100, 62, 42, 48, .40F, .85F, 100},
        {EnemyKind::Soldier, "soldier", "整合运动士兵", "近身斩击：看准举刀前摇，绕后反击", 12, 100, 48, 90, 68, .62F, .85F, 174},
        {EnemyKind::Crossbow, "crossbow", "弩手", "远程压制：瞄准线锁定后侧移，剑气可抵消弩箭", 9, 72, 48, 88, 440, .90F, 1.25F, 165},
        {EnemyKind::Exploder, "exploder", "自爆源石虫", "接近或死亡后蓄爆：红圈内危险，及时撤离", 7, 120, 62, 44, 86, 1.05F, .35F, 105},
        {EnemyKind::Shield, "shield", "大盾兵", "正面物理减伤 80%：绕后、法术或盾击后摇破防", 36, 46, 66, 120, 85, .95F, 1.25F, 160},
        {EnemyKind::Drone, "drone", "侦察无人机", "空中巡航：保持高度，瞄准后发射能量弹；优先打断其锁定", 16, 70, 76, 58, 520, 1.10F, 1.40F, 112},
    }};
    return data;
}
const EnemyDefinition& EnemyData(EnemyKind kind) {
    const auto i = static_cast<unsigned>(kind);
    if (i >= EnemyDefinitions().size()) throw std::out_of_range("Unknown enemy kind");
    return EnemyDefinitions()[i];
}
