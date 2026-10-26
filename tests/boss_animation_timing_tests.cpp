#include "boss_animation_timing.h"
#include "story_boss.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

void Check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
bool Near(float a,float b){return std::abs(a-b)<.0001F;}
int main(){try {
    using namespace BossAnimationTiming;
    // A 2.4-second seal has the same preparation speed as a 1.25-second prism.
    for(float t:{.1F,.3F,.55F})Check(Near(Windup(t,2.4F,.25F,.4F),Windup(t,1.25F,.25F,.4F)),"Long warning slowed the preparation pose");
    Check(Near(Windup(1.0F,2.4F,.25F,.4F),.25F),"Long warning continued attacking instead of holding ready");
    Check(Near(Windup(2.4F,2.4F,.25F,.4F),.4F)&&Near(Release(0,.4F),.4F),"Preparation and release jumped between poses");
    Check(Near(Release(.56F,.4F),1)&&Near(Release(2.9F,.4F),1),"W projectile lifetime stretched the follow-through");
    Check(Near(Quake(0),.48F)&&Near(Quake(.65F),.48F)&&Near(Quake(1.3F),.48F),"Quake strikes were not synchronized with all three damage pulses");
    Check(Near(Quake(1.79F),1),"Final quake did not finish its recovery before leaving the state");
    for(float frame:{1.0F/120,1.0F/60,1.0F/30}) {
        float previous=Windup(0,1.3F,.3F,.48F);
        for(float t=frame;t<1.3F;t+=frame) {
            const float pose=Windup(t,1.3F,.3F,.48F);
            Check(pose>=previous&&pose-previous<.10F,"Windup skipped a large portion of the animation");previous=pose;
        }
    }
    StoryBoss b;b.StartSandbox(StoryBossKind::FrostNova,2900,1900);
    bool single=false,triple=false;
    for(int i=0;i<18000&&!(single&&triple);++i) {
        b.Update(1.0F/120,{b.Feet().x-360,605});
        if(b.State()==StoryBossState::FrostCast) {
            const float duration=b.StateDuration();single|=Near(duration,.8F);triple|=Near(duration,.98F);
            Check(Near(Windup(duration,duration,.3F,.5F),.5F),"Different frost cast durations missed the same firing pose");
        }
    }
    Check(single&&triple,"Timing audit lost a frost cast variant");
    std::cout<<"Boss motion: bounded preparation/follow-through, fixed-duration casts, continuous poses and three synchronized quake strikes passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
