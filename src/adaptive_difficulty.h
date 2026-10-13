#pragma once
#include <algorithm>
#include <cmath>

// Frozen encounter parameters. Neutral defaults keep standalone simulations and
// authored reference tests unchanged; only main-story gameplay opts in.
struct CombatDifficulty {
    float health=1, movement=1, projectile=1, recovery=1, spawnInterval=1, cooldown=1;
    static CombatDifficulty FromAbility(float ability) {
        if(!std::isfinite(ability))ability=.30F;
        const float a=std::clamp(ability,0.0F,1.0F);
        return {.90F+.65F*a,.95F+.30F*a,.95F+.30F*a,
                1.12F-.42F*a,1.12F-.42F*a,1.12F-.42F*a};
    }
};

// Session-only learning. No renderer, input, save file or enemy dependency.
// Changes take effect on the next encounter, never in an ongoing attack.
class AdaptiveDifficulty {
public:
    CombatDifficulty Profile() const {return CombatDifficulty::FromAbility(ability_);}
    float Ability() const {return ability_;}
    bool Active() const {return active_;}
    float Seconds() const {return seconds_;}
    int Completed() const {return completed_;}
    const char* Label() const {
        return ability_<.20F?"缓和":ability_<.48F?"标准":ability_<.75F?"进阶":"高压";
    }
    void Cancel() {active_=false;}
    void Begin(float expectedSeconds,int teamHealth) {
        if(active_)return;
        active_=true;seconds_=0;loss_=knockouts_=connections_=0;
        expected_=std::max(12.0F,expectedSeconds);teamHealth_=std::max(1,teamHealth);
    }
    void Tick(float dt,bool threats) {
        if(active_&&threats&&std::isfinite(dt)&&dt>0)seconds_+=std::min(dt,.05F);
    }
    void ConnectedHit() {if(active_)++connections_;}
    void ObserveDamage(int healthLost,int knockouts) {
        if(active_){loss_+=std::max(0,healthLost);knockouts_+=std::max(0,knockouts);}
    }
    bool Finish(bool won) {
        if(!active_)return false;
        active_=false;
        if(!won) {ability_=std::max(0.0F,ability_-.16F);++completed_;return true;}
        // Ignore instant clears/debug kills; revisiting a clear has no Begin.
        if(seconds_<4||connections_<2)return false;
        const float pace=std::clamp(.5F*expected_/std::max(4.0F,seconds_),0.0F,1.0F);
        const float survival=std::clamp(1.0F-float(loss_)/teamHealth_-knockouts_*.20F,0.0F,1.0F);
        const float target=.55F*pace+.45F*survival;
        // A successful but painful clear should also reduce pressure.
        const float change=std::clamp((target-ability_)*.35F,-.10F,.08F);
        ability_=std::clamp(ability_+change,0.0F,1.0F);++completed_;return true;
    }
private:
    float ability_=.30F,seconds_=0,expected_=12;
    int loss_=0,knockouts_=0,connections_=0,teamHealth_=6,completed_=0;
    bool active_=false;
};
