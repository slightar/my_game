#include "boss_client_fx.h"
#include "ui_font.h"
#include "terminal_ui.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Color Ice{166,227,255,255},Fire{255,132,77,255};
constexpr float Pi=3.14159265F;
float Clamp(float v){return std::clamp(v,0.0F,1.0F);}
}
void BossClientFx::DrawExpansion(const StoryBoss& b,const UiFont& font) const {
    const auto label=[&](const char* text,Vector2 at,Color color,float size=17) {
        const float width=font.Measure(text,size)+20;
        TerminalUi::Surface({at.x-width/2,at.y,width,size+12},Color{18,26,34,230},color);
        font.Draw(text,at.x-width/2+10,at.y+4,size,color);
    };
    const auto route=[&](Vector2 from,Vector2 to,float width,Color tint) {
        const auto texture=textures_.find("trail_08");if(texture==textures_.end())return;
        const float length=std::hypot(to.x-from.x,to.y-from.y);
        const float angle=std::atan2(to.y-from.y,to.x-from.x)*180/Pi;
        DrawTexturePro(texture->second,{12,58,102,7},{(from.x+to.x)/2,(from.y+to.y)/2,length,width},
                       {length/2,width/2},angle,tint);
    };
    const BossZone* nextQuake=nullptr;
    for(const auto& z:b.Zones())if(z.kind==BossFx::PatriotQuake&&z.delay>0&&z.remaining>0&&
        (!nextQuake||z.delay<nextQuake->delay))nextQuake=&z;
    for(const auto& blast:b.Blasts())if(blast.airburst&&blast.timer>0) {
        const float pulse=.55F+.25F*std::sin(b.Elapsed()*14);
        // A circular reticle shows the actual blast height and radius, including air targets.
        Quad("zhunxing_01",blast.position,blast.radius*2,blast.radius*2,0,Fade(RED,.8F));
        Quad("cqbw_01",blast.position,38,20,0,WHITE);
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("star_02",blast.position,42,42,b.Elapsed()*25,Fade(Fire,pulse*.5F));
        EndBlendMode();
        font.Draw(TextFormat("%.1fs",blast.timer),blast.position.x-21,blast.position.y-82,16,Fire);
    }
    if(b.State()==StoryBossState::RemoteMineWindup) {
        const int count=b.RemoteMineCount();
        for(int i=0;i<count;++i) {
            const Vector2 p{b.Aim().x+(i-(count-1)*.5F)*145,b.Aim().y};
            GroundWarning(p,84,.6F,ORANGE);
            Quad("w_mine",{p.x,p.y-18},38,38,0,Fade(WHITE,.5F));
        }
        font.Draw("射击拆雷或离开标记",b.Aim().x-100,b.Aim().y-132,18,RAYWHITE);
    }
    for(const auto& mine:b.Mines())if(mine.remote&&mine.age<12)
        font.Draw(TextFormat("可拆除 %.0f/2",std::ceil(mine.durability)),mine.position.x-42,mine.position.y-92,14,SKYBLUE);

    for(const auto& z:b.Zones())if(z.remaining>0) {
        if(z.kind==BossFx::FrostPrism&&z.delay>0) {
            const float charge=Clamp(z.age/1.65F);
            const Vector2 p=z.position;
            Quad("frstar_05",p,55,88,0,Fade(WHITE,.65F+.35F*charge));
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("frstar_07",p,70,100,0,Fade(Ice,.3F+.35F*charge));
            Quad("star_02",p,66,66,z.age*45,Fade(Ice,.25F));
            const Vector2 d{z.target.x-p.x,z.target.y-p.y};
            const float len=std::hypot(d.x,d.y),angle=std::atan2(d.y,d.x);
            for(int i=0;i<5;++i) {
                const float a=i*2*Pi/5+z.age*1.2F;
                Quad("xue_01",{p.x+std::cos(a)*38,p.y+std::sin(a)*45},15,20,i*37.0F,Fade(Ice,.45F));
            }
            EndBlendMode();
            for(int lane=-1;lane<=1;++lane) {
                const float a=angle+lane*.14F;
                route(p,{p.x+std::cos(a)*len,p.y+std::sin(a)*len},lane==0?5:3,Fade(Ice,lane==0?.75F:.32F));
            }
            Quad("zhunxing_01",z.target,32,32,0,Fade(Ice,.6F));
            label(TextFormat("%.1fs · %.0f/2",z.delay,std::ceil(z.durability)),{p.x,p.y-82},Ice,15);
        }
        if(z.kind==BossFx::FrostThorn&&z.delay>0) {
            GroundWarning(z.position,z.radius,.5F,Ice);
            Quad("frstar_05",{z.position.x,z.position.y-40},55,80,0,Fade(Ice,.36F));
            label(TextFormat("%.1fs",z.delay),{z.position.x,z.position.y-108},Ice,14);
        }
        if(&z==nextQuake) {
            GroundWarning(z.position,z.radius,.65F,Fire);
            route({z.position.x-z.radius,z.position.y-5},{z.position.x+z.radius,z.position.y-5},7,Fade(Fire,.7F));
            const int pulse=std::clamp(int(std::round((z.radius-150)/125))+1,1,3);
            label(TextFormat("震地 %d/3 · %.1fs",pulse,z.delay),{z.position.x+std::min(z.radius-40,160.0F),z.position.y-51},Fire,16);
        }
    }
    const auto state=b.State();const auto f=b.Feet();
    if(state==StoryBossState::PrismWindup||state==StoryBossState::Prism) {
        if(state==StoryBossState::PrismWindup)Quad("frstar_03",{f.x,f.y-65},100,160,0,Fade(Ice,.35F));
        label("冰晶锁定散射 · 击碎或换位",{b.Aim().x,GameConfig::kFloorY-158},Ice,18);
    }
    if(state==StoryBossState::ThornWindup||state==StoryBossState::Thorn)
        label("冰棘沿地面推进 · 跳跃避开",{f.x,GameConfig::kFloorY-181},Ice,18);
    if(state==StoryBossState::QuakeWindup||state==StoryBossState::Quake) {
        const float charge=state==StoryBossState::QuakeWindup?Clamp(b.StateTime()/1.3F):1;
        Quad("patrt_06",{f.x,f.y-85},125,185,0,Fade(WHITE,.3F+.35F*charge));
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("patrt_12",{f.x,f.y-60},170,180,0,Fade(Fire,.2F+.3F*charge));
        EndBlendMode();
        label("三次震地 · 范围逐次扩大",{f.x,f.y-249},Fire,18);
        label("跳跃避开或远离标记",{f.x,f.y-212},Fire,17);
    }
    if(state==StoryBossState::LanceWindup) {
        const Vector2 start{f.x+b.Facing()*48.0F,f.y-92};
        const float len=std::abs(b.Aim().x-start.x);
        route(start,b.Aim(),30,Fade(Fire,.24F));
        route(start,b.Aim(),5,Fade(Fire,.85F));
        DrawSpear(start,170,b.Facing()>0?90:-90,Fade(WHITE,.6F));
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("star_55",start,65,65,b.StateTime()*30,Fade(Fire,.4F));
        EndBlendMode();
        label(TextFormat("水平贯穿 · 跳跃或绕后  %.1fs",std::max(0.0F,b.StateDuration()-b.StateTime())),{f.x,f.y-212},Fire,18);
    }
}

void BossClientFx::DrawExpansionForeground(const StoryBoss& b) const {
    for(const auto& v:b.Visuals()) {
        const float t=Clamp(v.age/v.duration),fade=1-t;
        if(v.kind==BossFx::FrostPrism) {
            Quad("frstar_09",v.position,100+80*t,140+70*t,t*30,Fade(Ice,fade));
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("star_55",v.position,150,150,t*50,Fade(WHITE,Clamp(1-t*5)*.8F));
            for(int i=0;i<7;++i) {
                const float a=i*2*Pi/7;
                const Vector2 p{v.position.x+std::cos(a)*v.age*140,v.position.y+std::sin(a)*v.age*140};
                Quad("frstar_07",p,18,45,a*180/Pi,Fade(Ice,fade*.6F));
            }
            EndBlendMode();
        }
        if(v.kind==BossFx::FrostThorn) {
            const float rise=Clamp(v.age/.09F),height=135*rise;
            Quad("frstar_05",{v.position.x,v.position.y-height*.5F},76,height,0,Fade(WHITE,fade));
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("frstar_09",{v.position.x,v.position.y-18},110+80*t,70,0,Fade(Ice,fade*.75F));
            Quad("frstar_07",{v.position.x,v.position.y-height*.5F},85,height,0,Fade(Ice,fade*.55F));
            for(int i=0;i<5;++i)
                Quad("xue_01",{v.position.x+(i-2)*v.age*45,v.position.y-25-v.age*(70+i*15)},15,22,i*43.0F,Fade(Ice,fade*.65F));
            EndBlendMode();
        }
        if(v.kind==BossFx::PatriotQuake) {
            const float radius=250*v.scale;
            Quad("patrt_03",{v.position.x,v.position.y-15},radius*2,90,0,Fade(WHITE,fade*.65F));
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("haak_03",{v.position.x,v.position.y-8},radius*2*(1+.08F*t),75,0,Fade(Fire,fade*.85F));
            Quad("star_55",{v.position.x,v.position.y-40},150,140,0,Fade(WHITE,Clamp(1-t*6)*.7F));
            for(int i=0;i<9;++i) {
                const float x=v.position.x+(i-4)*radius/4;
                Quad("patrt_07",{x,v.position.y-22-v.age*(25+i%3*25)},52,70+40*t,0,Fade(Fire,fade*.55F));
                Quad("star_02",{x+(i-4)*v.age*25,v.position.y-25-v.age*(90+i%3*40)},13,16,i*35.0F,Fade(Fire,fade*.65F));
            }
            EndBlendMode();
        }
    }
    for(const auto& m:b.Missiles())if(m.kind==BossFx::SpearLance&&m.life>0) {
        const float facing=m.velocity.x>0?1.0F:-1.0F;
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("trail_15",{m.position.x-facing*70,m.position.y},230,14,facing>0?0:180,Fade(Fire,.8F));
        Quad("star_55",m.position,42,42,0,Fade(WHITE,.6F));
        for(int i=0;i<4;++i)Quad("star_02",{m.position.x-facing*(35+i*30),m.position.y+(i%2?1:-1)*(6+i*2)},14,18,i*31.0F,Fade(Fire,.55F));
        EndBlendMode();
    }
}
