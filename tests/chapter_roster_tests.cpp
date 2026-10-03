#include "chapter_encounters.h"
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
    }
    if(introduced.size()!=20||troops!=40)throw std::runtime_error("Chapter roster coverage incomplete");
    std::cout<<"Five authentic Boss stage rosters, 40 wave slots, 20 exact client variants and proportions passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
