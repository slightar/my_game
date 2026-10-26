#pragma once
#include "adaptive_difficulty.h"
#include "chapter_encounters.h"
#include <algorithm>
#include <vector>

namespace AdaptiveEncounters {
// Compare authored units by durability, speed, ranged pressure and special
// mechanics. These scores select a roster; they never modify an enemy's data.
inline float Threat(EnemyKind kind) {
    const auto& enemy=EnemyData(kind);
    return enemy.health*(1+.75F*enemy.armor)+enemy.speed*.03F+
        std::max(0.0F,.8F-enemy.windup)*8+
        (enemy.attack==EnemyAttack::Melee?0:8)+
        (enemy.coldBlast||enemy.shatter||enemy.attack==EnemyAttack::ColdBolt?4:0);
}

inline std::vector<EnemyKind> Formation(std::string_view region,int wave,const CombatDifficulty& difficulty) {
    auto result=ChapterFormation(region,wave);
    const int tier=difficulty.FormationTier();
    if(tier==1||result.empty())return result;
    if(tier==0) {
        // Keep authored order and the other units' warnings. Remove the most
        // demanding slot, such as an icebreaker or a mortar, on a rough run.
        if(result.size()>2)result.erase(std::max_element(result.begin(),result.end(),
            [](auto a,auto b){return Threat(a)<Threat(b);}));
        return result;
    }

    // Only use this chapter's existing roster and already loaded sprites.
    // Introduce a missing role before duplicating a strong unit; cap each
    // kind at two and ranged troops at half of the final wave, rounded up.
    std::vector<EnemyKind> pool;
    for(int authoredWave=0;authoredWave<2;++authoredWave)
        for(auto kind:ChapterFormation(region,authoredWave))
            if(std::find(pool.begin(),pool.end(),kind)==pool.end())pool.push_back(kind);
    const auto target=result.size()+static_cast<std::size_t>(tier-1);
    while(result.size()<target) {
        const auto ranged=std::count_if(result.begin(),result.end(),[](auto kind){
            return EnemyData(kind).attack!=EnemyAttack::Melee;
        });
        EnemyKind selected=EnemyKind::Count;
        bool selectedMissing=false;
        for(auto kind:pool) {
            const auto copies=std::count(result.begin(),result.end(),kind);
            if(copies>=2)continue;
            if(EnemyData(kind).attack!=EnemyAttack::Melee&&ranged>=static_cast<int>((target+1)/2))continue;
            const bool missing=copies==0;
            if(selected==EnemyKind::Count||(missing&&!selectedMissing)||
               (missing==selectedMissing&&Threat(kind)>Threat(selected))) {
                selected=kind;selectedMissing=missing;
            }
        }
        if(selected==EnemyKind::Count)break;
        result.push_back(selected);
    }
    return result;
}
} // namespace AdaptiveEncounters
