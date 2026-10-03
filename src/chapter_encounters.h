#pragma once
#include "enemy_data.h"
#include <string_view>
#include <vector>

// Only the five main-story approach rooms have chapter-specific troops.
// References list the original game's level code, not this project's room index.
inline std::string_view ChapterStage(std::string_view id) {
    if(id=="approach_bridge") return "1-8";
    if(id=="approach_wtower") return "1-12";
    if(id=="approach_ice") return "6-16";
    if(id=="approach_industry") return "7-18";
    if(id=="approach_core") return "JT8-3";
    return {};
}
inline std::vector<EnemyKind> ChapterFormation(std::string_view id,int wave) {
    using enum EnemyKind;
    if(id=="approach_bridge") return wave==0?std::vector{Slug,SlugAlpha,Soldier,HoundPro}:std::vector{MobileShield,Caster,Drone,SlugAlpha};
    if(id=="approach_wtower") return wave==0?std::vector{Scavenger,HoundPro,DualSwordsman,Molotov}:std::vector{MobileShield,DualSwordsman,Molotov,Scavenger};
    if(id=="approach_ice") return wave==0?std::vector{SnowSoldier,SnowSniper,IceSlug,SnowSoldier}:std::vector{SnowCasterLeader,Icebreaker,SnowSniper,IceSlug};
    if(id=="approach_industry") return wave==0?std::vector{GuerrillaHound,GuerrillaFighter,GuerrillaSniper,GuerrillaFighter}:std::vector{GuerrillaRaider,GuerrillaMortar,GuerrillaSarkaz,GuerrillaSniper};
    if(id=="approach_core") return wave==0?std::vector{UrsusBeast,UrsusAssault,UrsusCaster,UrsusBeast}:std::vector{UrsusAssault,UrsusCaster,UrsusCaster,UrsusBeast};
    return wave==0?std::vector{Drone,StealthCrossbow,Caster}:std::vector{IrrSlug,Shield,Drone,Caster};
}
