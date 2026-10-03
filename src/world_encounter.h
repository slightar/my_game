#pragma once
#include "world_layout.h"
#include "enemy_system.h"
#include "chapter_encounters.h"
#include <set>

// Encounter progress belongs to the current excursion, not stage selection.
// Leaving an unfinished approach resets its patrols; cleared roads stay clear
// until starting a new action. Each wave waits for physical player advancement.
class WorldEncounter {
public:
    void Reset() { cleared_.clear();active_=false;wave_=0; }
    void Enter(const std::string& id,EnemySystem& enemies) {
        id_=id;active_=WorldLayout::Internal(id);wave_=cleared_.contains(id)?2:0;
        enemies.Clear();
    }
    bool Active() const {return active_;}
    // Includes delayed reinforcements and the gap between waves, but not
    // pre-encounter exploration or an already cleared approach.
    bool InCombat(const EnemySystem& enemies) const {
        return active_ && (wave_>0 || enemies.Remaining()>0) && !Cleared(enemies);
    }
    int Wave() const {return wave_;}
    bool Cleared(const EnemySystem& enemies) const {return wave_==2&&(!enemies.HasEntry()||enemies.Cleared());}
    void Update(float x,EnemySystem& enemies) {
        if(!active_)return;
        if(Cleared(enemies)){cleared_.insert(id_);return;}
        const float width=WorldLayout::Find(id_)->width;
        if(wave_<2&&(wave_==0||enemies.Cleared())&&x>=width*(wave_==0?.30F:.63F)) {
            enemies.ResetEncounter(Formation(id_,wave_),width*(wave_==0?.52F:.85F),80,width-80);
            ++wave_;
        }
    }
    static std::vector<EnemyKind> Formation(std::string_view id,int wave) {
        return ChapterFormation(id,wave);
    }
private:
    std::string id_;
    std::set<std::string> cleared_;
    bool active_=false;
    int wave_=0;
};
