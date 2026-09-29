#pragma once
#include <array>
#include <string>

// Every kind below is named after the real Arknights enemy and backed by that enemy's own
// client rig - the mapping was verified against prts.wiki (see
// assets/enemies/mobs/README.md for the model -> bundle table). Do not rename a kind to
// something more evocative without checking that the rig actually depicts it: the first
// pass here shipped enemy_1030_wteeth under the name 猎犬 when 1030 is 拾荒者, and called
// enemy_1352_eslime 高能源石虫 when that is 辐能源石虫.
enum class EnemyKind {
    Slug,             // 源石虫       enemy_1007_slime
    AcidSlug,         // 酸液源石虫   enemy_1004_mslime
    SlugHigh,         // 高能源石虫   enemy_1021_bslime
    IrrSlug,          // 辐能源石虫   enemy_1352_eslime
    Soldier,          // 士兵         enemy_1002_nsabr
    Crossbow,         // 弩手         enemy_1003_ncbow
    Caster,           // 术师         enemy_1011_wizard
    Hound,            // 猎狗         enemy_1000_gopro
    StealthCrossbow,  // 隐形弩手     enemy_1019_jshoot
    Shield,           // 重装防御者   enemy_1006_shield
    Drone,            // 妖怪         enemy_1005_yokai
    Count
};
struct EnemyDefinition {
    EnemyKind kind;
    const char* id;
    const char* name;
    const char* tactic;
    float health, speed, width, height, reach, windup, recovery, spriteSize;
};
const std::array<EnemyDefinition, 11>& EnemyDefinitions();
const EnemyDefinition& EnemyData(EnemyKind kind);
// Builds the font inventory from the same data that the battle renderer displays.
// Keeping this data driven prevents newly added enemy names from turning into '?'.
std::string EnemyGlyphText();
