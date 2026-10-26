#include "story_boss_renderer.h"
#include "ui_font.h"
#include "boss_animation_timing.h"
#include <algorithm>
#include <cmath>

void StoryBossRenderer::DrawWinter(const StoryBoss& b,const UiFont& font) const {
    const bool frost=b.Kind()==StoryBossKind::FrostNova;
    const auto& art=art_[int(b.Kind())-1];const auto feet=b.Feet();
    if(!art.texture.id){font.Draw("Boss 素材缺失",feet.x-50,feet.y-115,18,RED);return;}
    fx_.Draw(b,font);
    if(frost&&frostSmooth_.id){DrawFrostSmooth(b,font);fx_.DrawForeground(b);return;}
    const auto range=[](float time,float duration,int first,int last){return first+BossAnimationTiming::Frame(std::clamp(time/std::max(.01F,duration),0.0F,1.0F),last-first+1);};
    const auto ready=[&](float pose,float contact){return BossAnimationTiming::Frame(BossAnimationTiming::Windup(b.StateTime(),b.StateDuration(),pose,contact),20);};
    const auto release=[&](float contact,float duration=.55F){return BossAnimationTiming::Frame(BossAnimationTiming::Release(b.StateTime(),contact,duration),20);};
    int row=frost?(b.Moving()?1:0):(b.PhaseTwo()?(b.Moving()?4:3):(b.Moving()?1:0));
    int frame=int(b.AnimationTime()*20/std::max(.01F,art.durations[row]))%20;
    switch(b.State()) {
    case StoryBossState::PrismWindup:row=3;frame=ready(.25F,.40F);break;
    case StoryBossState::Prism:row=3;frame=release(.40F,1.33F);break;
    case StoryBossState::ThornWindup:row=5;frame=ready(.22F,.50F);break;
    case StoryBossState::Thorn:row=5;frame=release(.50F,1.36F);break;
    case StoryBossState::QuakeWindup:
    case StoryBossState::LanceWindup:row=b.PhaseTwo()?5:2;frame=ready(.30F,.48F);break;
    case StoryBossState::Quake:row=b.PhaseTwo()?5:2;frame=BossAnimationTiming::Frame(BossAnimationTiming::Quake(b.StateTime()),20);break;
    case StoryBossState::Lance:row=b.PhaseTwo()?5:2;frame=release(.48F,.50F);break;
    case StoryBossState::FrostStepWindup:row=3;frame=range(b.StateTime(),.85F,0,8);break;
    case StoryBossState::LeapWindup:row=b.PhaseTwo()?5:2;frame=range(b.StateTime(),1.1F,0,7);break;
    case StoryBossState::Leap:row=b.PhaseTwo()?5:2;frame=range(b.StateTime(),.8F,7,12);break;
    case StoryBossState::FrostCast:
    case StoryBossState::FrostFanWindup:row=2;frame=ready(.30F,.50F);break;
    case StoryBossState::IceRingWindup:row=5;frame=ready(.22F,.50F);break;
    case StoryBossState::IceRing:row=5;frame=release(.50F,1.36F);break;
    case StoryBossState::BlackIceWindup:row=3;frame=ready(.25F,.40F);break;
    case StoryBossState::BlackIce:row=3;frame=release(.40F,1.33F);break;
    case StoryBossState::IceRainWindup:row=3;frame=ready(.25F,.40F);break;
    case StoryBossState::IceRain:row=3;frame=release(.40F,1.33F);break;
    case StoryBossState::Revive:
        if(frost){row=4;frame=range(b.StateTime(),b.RevivalDuration(),0,19);}
        else if(b.StateTime()<1.2F){row=7;frame=range(b.StateTime(),1.2F,0,19);}
        else if(b.StateTime()<3){row=8;frame=int((b.StateTime()-1.2F)*20/art.durations[row])%20;}
        else {row=9;frame=range(b.StateTime()-3,b.RevivalDuration()-3,0,19);}
        break;
    case StoryBossState::ComboWindup:row=b.PhaseTwo()?5:2;frame=ready(.30F,b.PhaseTwo()?.53F:.48F);break;
    case StoryBossState::Combo:row=b.PhaseTwo()?5:2;frame=BossAnimationTiming::Frame(b.PhaseTwo()?BossAnimationTiming::Release(b.StateTime(),.53F,.5F):
        b.StateTime()<1.08F?BossAnimationTiming::Pulse(b.StateTime(),.36F,.48F,.64F,.35F):BossAnimationTiming::Release(b.StateTime()-1.08F,.48F,.48F),20);break;
    case StoryBossState::SpearWindup:
    case StoryBossState::SpearRainWindup:row=6;frame=ready(.35F,.62F);break;
    case StoryBossState::Spear:row=6;frame=release(.62F,1.0F);break;
    case StoryBossState::SpearRain:row=6;frame=release(.62F,1.0F);break;
    case StoryBossState::Recover:
        if(!frost&&b.RecoveryFrom()==StoryBossState::Spear&&b.StateTime()<.5F)
            {row=6;frame=BossAnimationTiming::Frame(BossAnimationTiming::Release(b.StateTime()+.5F,.62F,1.0F),20);}
        break;
    case StoryBossState::ChargeWindup:row=b.PhaseTwo()?5:2;frame=range(b.StateTime(),b.PhaseTwo()?.75F:1.0F,0,7);break;
    case StoryBossState::Charge:row=b.PhaseTwo()?4:1;frame=int(b.AnimationTime()*20/std::max(.01F,art.durations[row]))%20;break;
    case StoryBossState::GuardWindup:row=b.PhaseTwo()?3:0;frame=range(b.StateTime(),.9F,0,9);break;
    case StoryBossState::Guard:row=b.PhaseTwo()?3:0;frame=int(b.AnimationTime()*20/std::max(.01F,art.durations[row]))%20;break;
    case StoryBossState::Defeated:
        row=frost?6:10;if(b.StateTime()>art.durations[row])return;
        frame=range(b.StateTime(),art.durations[row],0,19);break;
    default:break;
    }
    const bool flip=b.Facing()<0;
    const float cell=float(art.cell),anchorX=flip?cell-art.anchor.x:art.anchor.x;
    const Color tint=b.Flash()>0?Color{255,168,168,255}:b.Invulnerability()>0?Color{195,220,250,255}:WHITE;
    DrawEllipse(int(feet.x),int(GameConfig::kFloorY),frost?28:44,6,Fade(BLACK,.28F));
    DrawTexturePro(art.texture,{frame*cell,row*cell,flip?-cell:cell,cell},
        {feet.x-anchorX/cell*art.size,feet.y-art.anchor.y/cell*art.size,art.size,art.size},{},0,tint);
    fx_.DrawForeground(b);
}
void StoryBossRenderer::DrawFrostSmooth(const StoryBoss& b,const UiFont&) const {
    int clip=b.Moving()?1:0;
    float phase=std::fmod(b.AnimationTime(),frostClips_[clip].duration)/frostClips_[clip].duration;
    const auto ready=[&](float pose,float contact){return BossAnimationTiming::Windup(b.StateTime(),b.StateDuration(),pose,contact);};
    const auto release=[&](float contact,float duration=0,float lead=0) {
        if(duration<=0)duration=std::clamp((1-contact)*frostClips_[clip].duration/1.35F,.50F,1.40F);
        return BossAnimationTiming::Release(b.StateTime()+lead,contact,duration);
    };
    switch(b.State()) {
    case StoryBossState::PrismWindup:clip=3;phase=ready(.25F,.40F);break;
    case StoryBossState::Prism:clip=3;phase=release(.40F);break;
    case StoryBossState::ThornWindup:clip=5;phase=ready(.22F,.50F);break;
    case StoryBossState::Thorn:clip=5;phase=release(.50F);break;
    case StoryBossState::FrostStepWindup:clip=3;phase=std::clamp(b.StateTime()/.85F,0.0F,1.0F)*.35F;break;
    case StoryBossState::FrostCast:
    case StoryBossState::FrostFanWindup:clip=2;phase=ready(.30F,.50F);break;
    case StoryBossState::IceRingWindup:clip=5;phase=ready(.22F,.50F);break;
    case StoryBossState::IceRing:clip=5;phase=release(.50F);break;
    case StoryBossState::BlackIceWindup:clip=3;phase=ready(.25F,.40F);break;
    case StoryBossState::BlackIce:clip=3;phase=release(.40F);break;
    case StoryBossState::IceRainWindup:clip=3;phase=ready(.25F,.40F);break;
    case StoryBossState::IceRain:clip=3;phase=release(.40F);break;
    case StoryBossState::Recover:
        if((b.RecoveryFrom()==StoryBossState::FrostCast||b.RecoveryFrom()==StoryBossState::FrostFanWindup)&&b.StateTime()<.60F)
            {clip=2;phase=release(.50F);}
        else if(b.RecoveryFrom()==StoryBossState::IceRing&&b.StateTime()<.72F)
            {clip=5;phase=release(.50F,0,.65F);}
        else if(b.RecoveryFrom()==StoryBossState::BlackIce&&b.StateTime()<.65F)
            {clip=3;phase=release(.40F,0,.7F);}
        break;
    case StoryBossState::Revive:clip=4;phase=std::clamp(b.StateTime()/b.RevivalDuration(),0.0F,1.0F);break;
    case StoryBossState::Defeated:clip=6;
        if(b.StateTime()>frostClips_[clip].duration)return;
        phase=std::clamp(b.StateTime()/frostClips_[clip].duration,0.0F,1.0F);break;
    default:break;
    }
    const auto& frames=frostClips_[clip].frames;
    const int i=BossAnimationTiming::Frame(phase,int(frames.size()));
    const auto& f=frames[i];if(f.source.width<=0||f.source.height<=0)return;
    const auto feet=b.Feet();const float scale=art_[int(StoryBossKind::FrostNova)-1].size/512.0F;
    const bool flip=b.Facing()<0;
    const float left=flip?feet.x+(frostSmoothAnchor_.x-f.crop.x-f.source.width)*scale:
        feet.x-(frostSmoothAnchor_.x-f.crop.x)*scale;
    const float top=feet.y-(frostSmoothAnchor_.y-f.crop.y)*scale;
    const Color tint=b.Flash()>0?Color{255,168,168,255}:b.Invulnerability()>0?Color{195,220,250,255}:WHITE;
    DrawEllipse(int(feet.x),int(GameConfig::kFloorY),28,6,Fade(BLACK,.28F));
    DrawTexturePro(frostSmooth_,{f.source.x,f.source.y,flip?-f.source.width:f.source.width,f.source.height},
        {left,top,f.source.width*scale,f.source.height*scale},{},0,tint);
}
