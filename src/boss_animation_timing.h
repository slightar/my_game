#pragma once
#include <algorithm>
#include <cmath>

// Atlas phases are poses, not the lifetime of an attack's projectiles or zones.
// Long telegraphs hold a ready pose; follow-through always uses a bounded duration.
namespace BossAnimationTiming {
inline float Mix(float a,float b,float t){return a+(b-a)*std::clamp(t,0.0F,1.0F);}
inline float Windup(float seconds,float duration,float ready,float contact,
                    float preparation=.60F,float lead=.14F) {
    duration=std::max(.01F,duration);lead=std::min(lead,duration*.4F);
    const float start=duration-lead;
    if(seconds>=start)return Mix(ready,contact,(seconds-start)/lead);
    return Mix(0,ready,seconds/std::max(.01F,std::min(preparation,start)));
}
inline float Release(float seconds,float contact,float duration=.55F) {
    return Mix(contact,1,seconds/std::max(.01F,duration));
}
inline float Pulse(float seconds,float period,float contact,float peak,float ready) {
    const float t=std::fmod(std::max(0.0F,seconds),period)/period;
    if(t<.4F)return Mix(contact,peak,t/.4F);
    if(t<.8F)return Mix(peak,ready,(t-.4F)/.4F);
    return Mix(ready,contact,(t-.8F)/.2F);
}
inline float Quake(float seconds) {
    return seconds<1.30F?Pulse(seconds,.65F,.48F,.78F,.30F):Release(seconds-1.30F,.48F,.48F);
}
inline int Frame(float phase,int count){return std::clamp(int(std::round(phase*(count-1))),0,count-1);}
}
