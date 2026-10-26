#pragma once
#include "game_types.h"
#include "character_art.h"
#include <algorithm>
#include <cmath>
#include <vector>

// Presentation only: impact feedback never consumes simulation time.
class CombatFeedback {
public:
    void Reset() { impacts_.clear();hitShake_=killShake_=clock_=0; }
    float Advance(float dt) {
        const float elapsed=std::max(0.0F,dt);clock_+=elapsed;
        hitShake_=std::max(0.0F,hitShake_-elapsed);
        killShake_=std::max(0.0F,killShake_-elapsed);
        for(auto& impact:impacts_)impact.age+=elapsed;
        std::erase_if(impacts_,[](const auto& impact){return impact.age>=.28F;});
        return dt;
    }
    void Hit(const Bullet& bullet) {
        if(bullet.impactEvents.empty()) Add(bullet.hasImpact?bullet.impactPoint:bullet.position,false,bullet.facingDirection);
        else for(const auto& impact:bullet.impactEvents) Add(impact.position,impact.killed,bullet.facingDirection);
    }
    float ShakeStrength() const {
        return std::max(hitShake_/.065F,4.0F*killShake_/.18F);
    }
    Vector2 Offset() const {
        const float strength=ShakeStrength();
        return {std::sin(clock_*153)*strength,std::cos(clock_*117)*strength*.5F};
    }
    void Draw(const CharacterArt& art) const {
        for(const auto& hit:impacts_)
            art.DrawCombatImpact(hit.position,hit.age/.28F,hit.killed?84.0F:54.0F,hit.facing);
    }
    std::size_t Count() const { return impacts_.size(); }
private:
    void Add(Vector2 point,bool killed,int facing) {
        // Kill feedback must survive merging with an earlier ordinary hit.
        if(killed)killShake_=.18F;
        for(auto& hit:impacts_)if(hit.age<.05F&&std::hypot(hit.position.x-point.x,hit.position.y-point.y)<26) {
            if(killed&&!hit.killed){hit.killed=true;hit.age=0;}
            return;
        }
        if(!killed)hitShake_=.065F;
        if(impacts_.size()>=32)impacts_.erase(impacts_.begin());
        impacts_.push_back({point,0,killed,facing});
    }
    struct Impact {Vector2 position;float age;bool killed;int facing;};
    std::vector<Impact> impacts_;
    float hitShake_=0,killShake_=0,clock_=0;
};
