#include "boss_client_fx.h"
#include "ui_font.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float Pi=3.14159265F;
constexpr const char* Slash="red_attack_01_start_r";
constexpr const char* Hit="texas2_skill_02_hit";
constexpr const char* Arrival="texas2_birth_01_start";
constexpr const char* Impact="texas2_skill_03_hit_02";
constexpr Color Warning{238,76,65,255};

void Afterimage(Texture2D sprite,Vector2 p,int facing,float clipTime,float opacity) {
    if(!IsTextureValid(sprite))return;
    const float fw=float(sprite.width)/10,fh=float(sprite.height)/6;
    const float width=120*fw/fh;
    DrawTexturePro(sprite,{fw*std::clamp(int(clipTime*34),0,9),fh*2,facing<0?-fw:fw,fh},
                   {p.x,p.y+34+120*.13F,width,120},{width/2,120},0,Fade(WHITE,opacity));
}
}

void BossClientFx::DrawCrown(const Boss& b,const UiFont& font,Texture2D sprite) const {
    using S=Boss::State;
    if(b.IsDefeated())return;
    // Raw source images are used only for readable previews. Attack layers below
    // play their original alpha/size/color/flipbook curves from the client.
    const auto raw=[&](const char* clip,const char* name,Vector2 at,float w,float h,float rotation,Color color) {
        const auto c=timelines_.find(clip);if(c==timelines_.end())return;
        const auto layer=c->second.find(name);if(layer==c->second.end())return;
        Quad(layer->second.texture.c_str(),at,w,h,rotation,color);
    };
    const auto smoke=[&](Vector2 at,float age,float width,float opacity) {
        TimelineLayer(Arrival,"yanwu_02",age,{at.x-14,at.y-22-age*20},width,width*.85F,-20,opacity);
        TimelineLayer(Arrival,"yanwu",age,{at.x+16,at.y-18-age*15},width*1.3F,width*.8F,18,opacity);
    };
    const auto slash=[&](Vector2 at,float age,float rotation,float width,float opacity) {
        const float sourceTime=age*.7F;
        TimelineLayer(Slash,"andi",sourceTime,at,width,width*.85F,rotation,opacity,false,3);
        TimelineLayer(Slash,"liang",sourceTime,at,width*.98F,width*.85F,rotation,opacity,true,2.5F);
        TimelineLayer("crowns_attack_01_start_right","liang",sourceTime,at,width*.96F,width*.82F,rotation,opacity*.6F,true,1.5F);
    };
    const auto travel=[&](Vector2 start,Vector2 end,float age,float opacity) {
        const float length=std::hypot(end.x-start.x,end.y-start.y);
        if(length<4)return;
        const float angle=std::atan2(end.y-start.y,end.x-start.x)*180/Pi;
        const Vector2 middle{(start.x+end.x)/2,(start.y+end.y)/2-15};
        // Use the source red emitter rather than borrowing Texas's blue trail color.
        TimelineLayer(Slash,"suduxian",age*.7F,middle,std::min(480.0F,length)*2,80,angle,opacity,false,3);
    };
    const auto warnRoute=[&](Vector2 start,Vector2 end) {
        const float angle=std::atan2(end.y-start.y,end.x-start.x)*180/Pi;
        const Vector2 middle{(start.x+end.x)/2,(start.y+end.y)/2};
        const float length=std::hypot(end.x-start.x,end.y-start.y);
        // The source has large transparent margins: sample its bright strip so
        // the warning visibly covers the blade's full 44 px danger width.
        const auto texture=textures_.find("trail_08");
        if(texture!=textures_.end())DrawTexturePro(texture->second,{12,58,102,7},
            {middle.x,middle.y,length,44},{length/2,22},angle,Fade(Warning,.38F));
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("trail_08",middle,length,8,angle,Fade(Warning,.95F));
        EndBlendMode();
        Quad("zhunxing_01",{end.x,end.y+30},46,20,0,Fade(Warning,.95F));
    };
    const float remaining=std::ceil(std::max(0.0F,b.stateTimer_)*10)/10;

    for(const auto& echo:b.echoes_) {
        const bool mirror=echo.consumed&&echo.position.y<b.floorY_-100;
        const float release=std::max(0.0F,.2F-echo.remaining);
        const float opacity=echo.delay>0?.22F+.10F*std::sin(echo.age*9):std::max(0.0F,.8F-release);
        const int facing=b.fanAim_.x>=echo.position.x?1:-1;
        Afterimage(sprite,echo.position,facing,echo.delay>0?.06F:release,opacity);
        smoke({echo.position.x,echo.position.y+20},std::fmod(echo.age,.55F),105,opacity*.65F);
        if(echo.delay>0) {
            if(mirror) {
                const float angle=std::atan2(b.fanAim_.y-echo.position.y,b.fanAim_.x-echo.position.x);
                const float distance=std::hypot(b.fanAim_.x-echo.position.x,b.fanAim_.y-echo.position.y);
                for(int i=-b.mirrorHalf_;i<=b.mirrorHalf_;++i) {
                    const float a=angle+i*.19F;
                    warnRoute(echo.position,{echo.position.x+std::cos(a)*distance,echo.position.y+std::sin(a)*distance});
                }
                raw(Impact,"dilieliang",echo.position,48,48,echo.age*65,Fade(WHITE,.55F));
            } else {
                raw("red_skill_02_start","kuosan",{echo.position.x,b.floorY_-4},144,34,0,Fade(Warning,.75F));
                Quad("zhunxing_01",{echo.position.x,b.floorY_-5},144,25,0,Fade(Warning,.55F));
            }
            font.Draw(TextFormat("%.1fs",echo.delay),echo.position.x-22,echo.position.y-80,16,RAYWHITE);
        } else if(mirror) {
            slash({echo.position.x+facing*18,echo.position.y-8},release,facing>0?-45:135,145,.75F);
            TimelineLayer(Impact,"dilieliang",release+.15F,echo.position,80,65,0,.55F,true,2);
        } else {
            slash({echo.position.x,echo.position.y-12},release,-55+release*160,185,1);
            TimelineLayer(Hit,"smoke_01",release,{echo.position.x,echo.position.y-20},125,95,0,.75F);
            TimelineLayer(Impact,"baoshan",release+.15F,{echo.position.x,echo.position.y-10},145,90,0,.6F,true,2);
        }
    }
    if(b.state_==S::MirrorWindup||b.state_==S::AmbushWindup) {
        font.Draw(b.state_==S::MirrorWindup?"离开交叉路线":"离开标记",b.fanAim_.x-75,b.floorY_-132,19,RAYWHITE);
    }

    for(const auto& dart:b.darts_) {
        const float angle=std::atan2(dart.velocity.y,dart.velocity.x)*180/Pi;
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("trail_08",{dart.position.x-dart.velocity.x*.02F,dart.position.y-dart.velocity.y*.02F},34,5,angle,Fade(WHITE,.5F));
        EndBlendMode();
        Quad("crown_dart",dart.position,5,31,angle+90,WHITE);
    }
    if(b.state_==S::FanRetreat) {
        const Vector2 launch{b.retreatTargetX_+b.facingDirection_*34,b.floorY_-b.kRadius-24};
        const float aim=std::atan2(b.fanAim_.y-launch.y,b.fanAim_.x-launch.x);
        for(float offset:{-.36F,-.18F,0.0F,.18F,.36F}) {
            const float angle=aim+offset;
            Quad("trail_08",{launch.x+std::cos(angle)*75,launch.y+std::sin(angle)*75},150,3,angle*180/Pi,Fade(ORANGE,.55F));
        }
        Quad("zhunxing_01",{b.retreatTargetX_,b.floorY_-4},46,13,0,Fade(ORANGE,.7F));
    }
    if(b.state_==S::Melee) {
        const float age=.18F-b.stateTimer_;
        const Vector2 at{b.position_.x+b.facingDirection_*50,b.position_.y-15};
        const float angle=b.facingDirection_>0?-65.0F:115.0F;
        TimelineLayer("crowns_attack_01_start_right","andi",age,at,155,135,angle);
        TimelineLayer("crowns_attack_01_start_right","liang",age,at,155,135,angle);
    }
    if(b.state_==S::Rush) {
        travel({b.position_.x-b.facingDirection_*170,b.position_.y},b.position_,.07F,.65F);
    }
    if(b.state_==S::AirSlash) {
        slash({b.position_.x,b.position_.y-15},.28F-b.stateTimer_,b.facingDirection_>0?-30:150,175,.8F);
    }
    if(b.state_==S::RushTeleportOut||b.state_==S::RushTeleportIn) {
        smoke(b.position_,b.animationTime_,105,.65F);
    }
    if(b.state_==S::RushTeleportIn||b.state_==S::RushWindup) {
        const Vector2 end{std::clamp(b.position_.x+b.facingDirection_*1040*.82F,
                          b.arena_.x+28+b.kRadius,b.arena_.x+b.arena_.width-28-b.kRadius),b.position_.y};
        warnRoute(b.position_,end);
        font.Draw(TextFormat("跳跃避开  %.1f",remaining),(b.position_.x+end.x)/2-75,b.floorY_-128,20,RAYWHITE);
    }
    if(b.state_==S::AirSlashWindup) {
        warnRoute(b.slashStart_,b.slashTarget_);
        font.Draw(TextFormat("离开路线  %.1f",remaining),b.slashTarget_.x-80,b.floorY_-128,20,RAYWHITE);
    }
    if(b.state_==S::ShadowWindup||b.state_==S::ShadowPause) {
        const Vector2 start=b.shadowStarts_[b.shadowStrike_],end=b.shadowEnds_[b.shadowStrike_];
        warnRoute(start,end);
        smoke({start.x,start.y+20},b.animationTime_,100,.7F);
        // Show only the imminent route, so a player can read one dodge at a time.
        const char* hint=b.shadowStrike_==0?"离开路线":(start.x>end.x?"向右避开 →":"← 向左避开");
        font.Draw(TextFormat("%s  %.1f",hint,remaining),b.shadowAim_.x-105,b.floorY_-128,20,RAYWHITE);
    }
    if(b.state_==S::ShadowSlash) {
        const Vector2 start=b.shadowStarts_[b.shadowStrike_];
        // Stretch the existing client animation along with the first two strikes.
        const float duration=Boss::ShadowSlashDuration(b.shadowStrike_);
        const float age=.18F*(1-b.stateTimer_/duration);
        travel(start,b.position_,age,1);
        for(int i=1;i<=3;++i) {
            const float t=float(i)/4;
            Afterimage(sprite,{start.x+(b.position_.x-start.x)*t,start.y+(b.position_.y-start.y)*t},
                       b.facingDirection_,age,.08F+i*.035F);
        }
        slash({b.position_.x+b.facingDirection_*25,b.position_.y-12},age,
              b.facingDirection_>0?-55+age*170:125-age*170,200,1);
        smoke({start.x,start.y+25},age,100,.6F);
    }
    if(b.state_==S::ShadowFinalWindup) {
        raw("red_skill_02_start","kuosan",{b.shadowLanding_.x,b.floorY_-3},220,38,0,Fade(Warning,.95F));
        warnRoute(b.slashStart_,b.slashTarget_);
        raw(Impact,"dilieliang",{b.position_.x+25,b.position_.y-32},48,48,b.animationTime_*55,Fade(WHITE,.75F));
        smoke({b.position_.x,b.position_.y+25},std::fmod(b.animationTime_,.55F),80,.5F);
        font.Draw(TextFormat("离开红圈  %.1f",remaining),b.shadowLanding_.x-80,b.floorY_-128,20,RAYWHITE);
    }
    if(b.state_==S::ShadowFinal) {
        const float age=.24F-b.stateTimer_;
        travel(b.slashStart_,b.position_,age,1);
        slash({b.position_.x,b.position_.y-12},age,-15+age*220,220,1);
        TimelineLayer(Impact,"baoci",age,{b.position_.x,b.position_.y-35},155,100,90,1);
        Afterimage(sprite,{b.position_.x,b.position_.y-65},b.facingDirection_,age,.2F);
    }
    for(const auto& trace:b.shadowTraces_) {
        if(!trace.final) {
            const float age=trace.age+.18F;
            travel(trace.from,trace.to,age,.65F);
            slash({trace.to.x+trace.facing*25,trace.to.y-12},age,trace.facing>0?-25:95,200,.85F);
            smoke({trace.to.x,trace.to.y+25},trace.age,110,.5F);
            // Client smoke and a short-lived strike flash replace the geometric ribbon.
            TimelineLayer(Hit,"smoke_01",trace.age,{trace.to.x-trace.facing*35,trace.to.y-20-trace.age*20},90,70,10,.7F);
        } else {
            const Vector2 floor{trace.to.x,b.floorY_-3};
            // Align the source prefab's 0.15 s emission lead-in with actual contact.
            const float age=trace.age+.15F;
            TimelineLayer(Impact,"dilie",age,{floor.x,floor.y-2},240,60,0,.85F,false,2);
            TimelineLayer(Impact,"baoan",age,{floor.x,floor.y-28},170,95,0,.85F);
            TimelineLayer(Impact,"baoci",age,{floor.x,floor.y-42},170,70,0,1);
            TimelineLayer(Impact,"dilieliang",age,{floor.x,floor.y-22},180,95,0,.65F,true);
            TimelineLayer(Impact,"baoshan",age,{floor.x,floor.y-26},180,115,0,.6F,true,2);
            TimelineLayer("red_skill_02_start","kuosan",age,{floor.x,floor.y-3},210,55,0,.7F,false,2);
            for(int i=0;i<5;++i) {
                const float side=i-2.0F;
                const Vector2 cloud{floor.x+side*(22+trace.age*45),floor.y-14-std::abs(side)*5-trace.age*18};
                TimelineLayer(Impact,"yanwu_02",age,cloud,135+std::abs(side)*20,100,side*23,.85F);
                TimelineLayer(Impact,"yanwu",age,cloud,160,100,side*15,.75F);
            }
            for(int i=0;i<8;++i) {
                const float a=(i+.5F)*Pi/8;
                const float travel=trace.age*(85+(i%3)*24);
                const Vector2 debris{floor.x+std::cos(a)*travel,floor.y-12-std::sin(a)*travel+70*trace.age*trace.age};
                TimelineLayer(Impact,"lizi_02",age,debris,12,12,i*33+age*150,.85F);
            }
        }
    }
}
