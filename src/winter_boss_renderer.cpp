#include "story_boss_renderer.h"
#include "ui_font.h"
#include <algorithm>
#include <cmath>

void StoryBossRenderer::DrawWinter(const StoryBoss& b,const UiFont& font) const {
    const bool frost=b.Kind()==StoryBossKind::FrostNova;
    const auto& art=art_[int(b.Kind())-1];const auto feet=b.Feet();
    if(!art.texture.id){font.Draw("Boss 素材缺失",feet.x-50,feet.y-115,18,RED);return;}
    fx_.Draw(b,font);
    if(frost&&frostSmooth_.id){DrawFrostSmooth(b,font);fx_.DrawForeground(b);return;}
    const auto range=[](float time,float duration,int first,int last){return std::clamp(first+int(time/std::max(.01F,duration)*(last-first+1)),first,last);};
    int row=frost?(b.Moving()?1:0):(b.PhaseTwo()?(b.Moving()?4:3):(b.Moving()?1:0));
    int frame=int(b.AnimationTime()*20/std::max(.01F,art.durations[row]))%20;
    switch(b.State()) {
    case StoryBossState::FrostStepWindup:row=3;frame=range(b.StateTime(),.85F,0,8);break;
    case StoryBossState::LeapWindup:row=b.PhaseTwo()?5:2;frame=range(b.StateTime(),1.1F,0,7);break;
    case StoryBossState::Leap:row=b.PhaseTwo()?5:2;frame=range(b.StateTime(),.8F,7,12);break;
    case StoryBossState::FrostCast:row=2;frame=range(b.StateTime(),.8F,0,12);break;
    case StoryBossState::FrostFanWindup:row=2;frame=range(b.StateTime(),1.2F,0,12);break;
    case StoryBossState::IceRingWindup:row=5;frame=range(b.StateTime(),1.15F,0,10);break;
    case StoryBossState::IceRing:row=5;frame=range(b.StateTime(),.65F,11,19);break;
    case StoryBossState::BlackIceWindup:row=3;frame=range(b.StateTime(),2.4F,0,12);break;
    case StoryBossState::BlackIce:row=3;frame=range(b.StateTime(),.7F,13,19);break;
    case StoryBossState::IceRainWindup:row=3;frame=range(b.StateTime(),1.05F,0,10);break;
    case StoryBossState::IceRain:row=3;frame=range(b.StateTime(),1.9F,11,19);break;
    case StoryBossState::Revive:
        if(frost){row=4;frame=range(b.StateTime(),b.RevivalDuration(),0,19);}
        else if(b.StateTime()<1.2F){row=7;frame=range(b.StateTime(),1.2F,0,19);}
        else if(b.StateTime()<3){row=8;frame=int((b.StateTime()-1.2F)*20/art.durations[row])%20;}
        else {row=9;frame=range(b.StateTime()-3,b.RevivalDuration()-3,0,19);}
        break;
    case StoryBossState::ComboWindup:row=b.PhaseTwo()?5:2;frame=range(b.StateTime(),b.PhaseTwo()?.75F:.9F,0,7);break;
    case StoryBossState::Combo:row=b.PhaseTwo()?5:2;frame=b.PhaseTwo()?range(b.StateTime(),.5F,8,19):8+int(b.StateTime()/.36F*12)%12;break;
    case StoryBossState::SpearWindup:row=6;frame=range(b.StateTime(),2,0,14);break;
    case StoryBossState::Spear:row=6;frame=range(b.StateTime(),.5F,15,19);break;
    case StoryBossState::SpearRainWindup:row=6;frame=range(b.StateTime(),1.15F,0,14);break;
    case StoryBossState::SpearRain:row=6;frame=range(b.StateTime(),1.3F,15,19);break;
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
    switch(b.State()) {
    case StoryBossState::FrostStepWindup:clip=3;phase=std::clamp(b.StateTime()/.85F,0.0F,1.0F)*.35F;break;
    case StoryBossState::FrostCast:clip=2;phase=std::clamp(b.StateTime()/.98F,0.0F,1.0F)*.65F;break;
    case StoryBossState::FrostFanWindup:clip=2;phase=std::clamp(b.StateTime()/1.2F,0.0F,1.0F)*.65F;break;
    case StoryBossState::IceRingWindup:clip=5;phase=std::clamp(b.StateTime()/1.15F,0.0F,1.0F)*.65F;break;
    case StoryBossState::IceRing:clip=5;phase=.65F+std::clamp(b.StateTime()/.65F,0.0F,1.0F)*.35F;break;
    case StoryBossState::BlackIceWindup:clip=3;phase=std::clamp(b.StateTime()/2.4F,0.0F,1.0F)*.65F;break;
    case StoryBossState::BlackIce:clip=3;phase=.65F+std::clamp(b.StateTime()/.7F,0.0F,1.0F)*.35F;break;
    case StoryBossState::IceRainWindup:clip=3;phase=std::clamp(b.StateTime()/1.05F,0.0F,1.0F)*.5F;break;
    case StoryBossState::IceRain:clip=3;phase=.5F+std::clamp(b.StateTime()/1.9F,0.0F,1.0F)*.5F;break;
    case StoryBossState::Revive:clip=4;phase=std::clamp(b.StateTime()/b.RevivalDuration(),0.0F,1.0F);break;
    case StoryBossState::Defeated:clip=6;
        if(b.StateTime()>frostClips_[clip].duration)return;
        phase=std::clamp(b.StateTime()/frostClips_[clip].duration,0.0F,1.0F);break;
    default:break;
    }
    const auto& frames=frostClips_[clip].frames;
    const int i=std::clamp(int(phase*frames.size()),0,int(frames.size())-1);
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
