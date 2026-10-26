#include "chapter_encounters.h"
#include "adaptive_encounters.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>

int main(int argc,char** argv) {try {
    if(argc!=2)throw std::runtime_error("Expected enemy asset directory");
    const std::filesystem::path root=argv[1];
    const auto read=[&](const char* name){nlohmann::json j;std::ifstream in(root/name);in>>j;return j;};
    const auto roster=read("chapter_rosters.json"),sources=read("chapter_sources.json"),manifest=read("manifest.json");
    std::set<EnemyKind> introduced;
    int troops=0;
    for(const char* boss:{"bridge","wtower","ice","industry","core"}) {
        const std::string approach=std::string("approach_")+boss;
        const auto& chapter=roster.at(boss);
        if(chapter.at("code").get<std::string>()!=ChapterStage(approach))throw std::runtime_error("Chapter stage mismatch");
        if(chapter.at("source_sha256").get<std::string>().size()!=64)throw std::runtime_error("Missing original level identity");
        for(int wave:{0,1}) {
            const auto formation=ChapterFormation(approach,wave);
            if(formation.size()!=4)throw std::runtime_error("Chapter wave lost troops");
            for(auto kind:formation) {
                ++troops;const auto& d=EnemyData(kind);const auto& rig=manifest.at(d.id);
                const std::string model=rig.at("model");bool found=false;
                for(const auto& original:chapter.at("originalEnemies")) {
                    if(original.at("id")==model) {
                        if(original.at("name").get<std::string>()!=d.name)throw std::runtime_error("Enemy name does not match original chapter");
                        found=true;
                    }
                }
                if(!found)throw std::runtime_error(std::string(d.id)+" absent from original Boss stage");
                if(static_cast<unsigned>(kind)>=static_cast<unsigned>(EnemyKind::SlugAlpha)) {
                    introduced.insert(kind);
                    if(sources.at(d.id).at("model")!=model || rig.at("source")!=sources.at(d.id))throw std::runtime_error("Variant model provenance changed");
                    if(d.height<50 || d.height>130)throw std::runtime_error("Enemy size out of operator scale");
                }
                if(!std::filesystem::exists(root/(std::string(d.id)+".png")))throw std::runtime_error("Missing chapter sprite");
            }
        }
        std::set<EnemyKind> allowed;
        for(int wave:{0,1})for(auto kind:ChapterFormation(approach,wave))allowed.insert(kind);
        for(int wave:{0,1}) {
            const auto authored=ChapterFormation(approach,wave);
            const std::set<EnemyKind> authoredKinds(authored.begin(),authored.end());
            std::size_t previousSize=0;
            float previousHealth=0;
            for(float ability:{0.0F,.30F,.60F,1.0F}) {
                const auto profile=CombatDifficulty::FromAbility(ability);
                const auto formation=AdaptiveEncounters::Formation(approach,wave,profile);
                if(formation.size()!=static_cast<std::size_t>(3+profile.FormationTier()))
                    throw std::runtime_error("Adaptive chapter wave count incorrect");
                if(profile.FormationTier()==1&&formation!=ChapterFormation(approach,wave))
                    throw std::runtime_error("Standard difficulty changed the authored roster");
                float totalHealth=0;
                int ranged=0;
                std::set<EnemyKind> present;
                for(auto kind:formation) {
                    if(!allowed.contains(kind))throw std::runtime_error("Adaptive wave crossed chapter rosters");
                    if(std::count(formation.begin(),formation.end(),kind)>2)
                        throw std::runtime_error("Adaptive wave stacked the same unit too many times");
                    totalHealth+=EnemyData(kind).health*profile.health;
                    ranged+=EnemyData(kind).attack!=EnemyAttack::Melee;
                    present.insert(kind);
                }
                if(profile.FormationTier()>1&&ranged>static_cast<int>((formation.size()+1)/2))
                    throw std::runtime_error("Adaptive wave overwhelmed the player with ranged units");
                if(profile.FormationTier()>1&&present.size()<std::min(allowed.size(),
                    authoredKinds.size()+static_cast<std::size_t>(profile.FormationTier()-1)))
                    throw std::runtime_error("Adaptive wave failed to introduce an available role");
                if(formation.size()<=previousSize||totalHealth<=previousHealth)
                    throw std::runtime_error("Increasing ability did not increase encounter workload");
                previousSize=formation.size();previousHealth=totalHealth;
            }
        }
    }
    if(introduced.size()!=20||troops!=40)throw std::runtime_error("Chapter roster coverage incomplete");
    const auto gentleIce=AdaptiveEncounters::Formation("approach_ice",1,CombatDifficulty::FromAbility(0));
    if(std::find(gentleIce.begin(),gentleIce.end(),EnemyKind::Icebreaker)!=gentleIce.end())
        throw std::runtime_error("Gentle winter wave retained the icebreaker");
    std::cout<<"Five authentic stage rosters, adaptive 3/4/5/6-unit waves, chapter-safe role variety and pressure caps passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
