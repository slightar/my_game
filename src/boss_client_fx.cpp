#include "boss_client_fx.h"
#include "file_path.h"
#include "ui_font.h"
#include "terminal_ui.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace {
constexpr Color Ice{166,227,255,255},Fire{255,132,77,255};
constexpr float Pi=3.14159265F;
float Saturate(float value){return std::clamp(value,0.0F,1.0F);}
// A short attack followed by a longer release. No random state: pausing freezes VFX.
float Envelope(float progress,float attack=.12F){
    return Saturate(progress/attack)*Saturate((1-progress)/(1-attack));
}
}
BossClientFx::BossClientFx() {
    const auto root=Utf8Path(GetApplicationDirectory())/"assets/enemies/bosses/effects";
    for(const auto& layerRoot:{root,root/"crown_client",root/"w_client"}) {
        try {
            nlohmann::json m;std::ifstream in(layerRoot/"manifest.json");in>>m;
            for(const auto& [id,v]:m.at("textures").items()) {
                const auto name=v.at("name").get<std::string>();if(textures_.contains(name))continue;
                auto texture=LoadTexture((layerRoot/v.at("file").get<std::string>()).string().c_str());
                if(texture.id){SetTextureFilter(texture,TEXTURE_FILTER_BILINEAR);textures_.emplace(name,texture);}
            }
        }catch(const std::exception& e){TraceLog(LOG_WARNING,"Original Boss VFX: %s",e.what());}
        try {
            nlohmann::json m;std::ifstream in(layerRoot/"timelines.json");in>>m;
            if(m.at("fps")!=60)throw std::runtime_error("Boss VFX timeline requires 60 Hz samples");
            for(const auto& [clip,layers]:m.at("clips").items())for(const auto& item:layers) {
                FxLayer layer;layer.texture=item.at("texture");layer.life=item.at("life");layer.delay=item.at("delay");
                layer.columns=item.at("columns");layer.rows=item.at("rows");
                if(layer.life<=0||layer.columns<1||layer.rows<1||!textures_.contains(layer.texture))
                    throw std::runtime_error("Incomplete Boss VFX layer: "+layer.texture);
                for(const auto& frame:item.at("frames")) {
                    const auto byte=[&](int n){return static_cast<unsigned char>(Saturate(frame.at(n).get<float>())*255);};
                    layer.frames.push_back({frame.at(0).get<float>(),{byte(1),byte(2),byte(3),byte(4)},frame.at(5).get<int>()});
                }
                if(layer.frames.empty())throw std::runtime_error("Empty Boss VFX timeline");
                timelines_[clip].emplace(item.at("name").get<std::string>(),std::move(layer));
            }
        }catch(const std::exception& e){if(layerRoot==root)timelines_.clear();TraceLog(LOG_WARNING,"Boss VFX timelines: %s",e.what());}
    }
    const auto knifePath=(Utf8Path(GetApplicationDirectory())/"assets/operators/texas_sword.png").string();
    auto knife=LoadTexture(knifePath.c_str());
    if(IsTextureValid(knife)){SetTextureFilter(knife,TEXTURE_FILTER_BILINEAR);textures_.emplace("crown_dart",knife);}
}

bool BossClientFx::TimelineLayer(const char* clip,const char* name,float seconds,Vector2 at,float w,float h,
    float rotation,float opacity,bool additive,float exposure,float anchorY,float anchorX) const {
    const auto c=timelines_.find(clip);if(c==timelines_.end())return false;
    const auto it=c->second.find(name);if(it==c->second.end())return false;
    const auto& layer=it->second;const float age=seconds-layer.delay;
    if(age<0||age>=layer.life)return true;
    const float frame=age*60;const int first=std::min(int(frame),int(layer.frames.size())-1);
    const auto& a=layer.frames[first];const auto& b=layer.frames[std::min(first+1,int(layer.frames.size())-1)];
    const float fraction=frame-first,size=a.size+(b.size-a.size)*fraction;
    const auto mix=[&](unsigned char x,unsigned char y){return static_cast<unsigned char>(x+(y-x)*fraction);};
    Color tint{mix(a.color.r,b.color.r),mix(a.color.g,b.color.g),mix(a.color.b,b.color.b),
        static_cast<unsigned char>(mix(a.color.a,b.color.a)*Saturate(opacity))};
    // Client FX shaders use HDR intensity. Keep alpha/timing intact while adapting
    // brightness to raylib's unlit texture shader and the brighter side-view scene.
    const auto exposed=[&](unsigned char channel){return static_cast<unsigned char>(std::clamp(channel*exposure,0.0F,255.0F));};
    tint.r=exposed(tint.r);tint.g=exposed(tint.g);tint.b=exposed(tint.b);
    if(additive)BeginBlendMode(BLEND_ADDITIVE);
    Quad(layer.texture.c_str(),{at.x+(.5F-anchorX)*w*size,at.y+(.5F-anchorY)*h*size},
        w*size,h*size,rotation,tint,a.tile,layer.columns,layer.rows);
    if(additive)EndBlendMode();
    return true;
}
BossClientFx::~BossClientFx(){for(auto& [name,t]:textures_)UnloadTexture(t);}
bool BossClientFx::Ready() const {
    return ReadyFor(StoryBossKind::FrostNova)&&ReadyFor(StoryBossKind::Patriot);
}
bool BossClientFx::ReadyFor(StoryBossKind kind) const {
    const char* clip=kind==StoryBossKind::FrostNova?"frstar2_attack_02_start":kind==StoryBossKind::Patriot?
        "patrt_attack_01_start":kind==StoryBossKind::W?"cqbw_attack_01_hit_01":"crowns_attack_01_start_right";
    if(kind!=StoryBossKind::None&&!timelines_.contains(clip))return false;
    const auto has=[&](std::initializer_list<const char*> names){
        for(const char* name:names)if(!textures_.contains(name))return false;
        return true;
    };
    if(kind==StoryBossKind::FrostNova)return has({"frstar_01","frstar_02","frstar_03","frstar_05","frstar_07","frstar_09",
        "star_02","star_10","star_55","trail_08","dilie_08","water_05","flipbook_06","flipbook_51","xue_01"});
    if(kind==StoryBossKind::Patriot)return has({"patrt_01","patrt_02","patrt_03","patrt_04","patrt_05","patrt_06",
        "patrt_07","patrt_12","patrt_13","patrt_16","patrt_17","zhunxing_01","flow_34","haak_03","trail_07","trail_15","star_02","star_55"});
    if(kind==StoryBossKind::Crownslayer)return has({"daoguang_09","zhunxing_01","trail_08","crown_dart"})&&
        timelines_.contains("red_attack_01_start_r")&&timelines_.contains("texas2_skill_02_hit")&&
        timelines_.contains("texas2_birth_01_start")&&timelines_.contains("texas2_skill_03_hit_02");
    if(kind==StoryBossKind::W)return has({"cqbw_01","w_mine","cqbw_05","cqbw_06","C4_02","water_20","star_02","star_62","trail_08"})&&
        timelines_.contains("cqbw_skill_03_hit")&&timelines_.contains("cqbw_attack_01_start")&&
        timelines_.contains("cqbw_attack_01_start_02")&&timelines_.contains("cqbw_attack_01_trail_01");
    return true;
}
void BossClientFx::Quad(const char* id,Vector2 at,float w,float h,float angle,Color color,int frame,int columns,int rows) const {
    const auto found=textures_.find(id);if(found==textures_.end())return;const auto t=found->second;
    const float sw=float(t.width)/columns,sh=float(t.height)/rows;frame=std::clamp(frame,0,columns*rows-1);
    DrawTexturePro(t,{float(frame%columns)*sw,float(frame/columns)*sh,sw,sh},{at.x,at.y,w,h},{w/2,h/2},angle,color);
}
void BossClientFx::DrawSpear(Vector2 at,float height,float rotation,Color color) const {
    const auto found=textures_.find("patrt_17");if(found==textures_.end())return;
    const auto texture=found->second;
    // The shaft occupies x=111..143 of the original 256-pixel texture.
    // Crop its transparent padding before scaling so the visible spear keeps its proportions.
    const Rectangle source{texture.width*111.0F/256,0,texture.width*33.0F/256,float(texture.height)};
    const float width=height*source.width/source.height;
    DrawTexturePro(texture,source,{at.x,at.y,width,height},{width/2,height/2},rotation,color);
}
void BossClientFx::GroundWarning(Vector2 at,float radius,float pulse,Color color) const {
    // Keep warning geometry on the ground; cqbw_05 is a skull, not an ellipse.
    BeginBlendMode(BLEND_ADDITIVE);
    Quad("water_20",{at.x,at.y-5},radius*2,28,0,Fade(color,.28F+.22F*Saturate(pulse)));
    Quad("trail_08",{at.x,at.y-3},radius*2,7,0,Fade(color,.5F+.25F*Saturate(pulse)));
    EndBlendMode();
}
void BossClientFx::Explosion(Vector2 at,float radius,float seconds,bool c4) const {
    const char* clip=c4?"cqbw_skill_03_hit":"cqbw_attack_01_hit_01";
    const float power=c4?1.22F:1;
    const Vector2 center{at.x,at.y-radius*.48F};
    TimelineLayer(clip,"andi",seconds,center,radius*2.3F,radius*1.9F);
    // fire_23 includes its own dark smoke. Alpha blending preserves that body;
    // additive blending here used to erase it against the sunset background.
    // Source cells have asymmetric padding: the fire base moves from roughly
    // (0.27,0.77) toward (0.51,0.97) before the smoke rises. Keep that base at impact.
    TimelineLayer(clip,"fire",seconds,at,radius*2.05F*power,radius*2.05F*power,0,1,false,1,
        .77F+.20F*Saturate(seconds/.5F),.27F+.24F*Saturate(seconds/.65F));
    TimelineLayer(clip,"baozha_02",seconds,center,radius*2.65F*power,radius*2.25F*power,0,1,true);
    TimelineLayer(clip,"huan",seconds,{at.x,at.y-8},radius*2.65F,radius*.75F,0,.85F,true);
    TimelineLayer(clip,"baoshan",seconds,center,radius*2.1F,radius*1.9F,0,.8F,true,1.6F);
    TimelineLayer(clip,"star",seconds,center,radius*2.7F,radius*2.1F,0,.8F,true);
    const int count=c4?18:11;
    for(int i=0;i<count;++i) {
        const float a=(i+.4F)*Pi/count,speed=(95+23*(i%4))*power;
        const Vector2 point{at.x+std::cos(a)*seconds*speed,
            at.y-12-std::sin(a)*seconds*speed+58*seconds*seconds};
        // Embers stop at the deck rather than falling through its underside.
        if(point.y>GameConfig::kFloorY)continue;
        TimelineLayer(clip,"suikuai",seconds,point,21,21,i*37.0F+seconds*75,1,true,1.7F);
        const float direction=std::atan2(-std::sin(a)*speed+116*seconds,std::cos(a)*speed)*180/Pi;
        TimelineLayer(clip,"suduxian",seconds,point,34+14*(i%3),4,direction,.8F,true,1.5F);
    }
    if(c4) {
        TimelineLayer(clip,"kuloutou",seconds,{at.x,at.y-radius-30*seconds},radius*1.35F,radius*1.2F,0,.9F,true);
        TimelineLayer(clip,"shandian",seconds,center,radius*1.8F,radius*1.7F,0,.8F,true,1.8F);
        for(int i=0;i<9;++i) {
            const float side=(i%2?1.0F:-1.0F),age=seconds;
            const Vector2 point{at.x+side*(14+age*(13+i*4)),at.y-22-age*(28+i*6)};
            TimelineLayer(clip,"suikuai_02",age,point,15,19,i*43.0F+age*30,.8F,true,2);
        }
    }
}
void BossClientFx::ExplosionSmoke(Vector2 at,float radius,float seconds,bool c4) const {
    const char* clip=c4?"cqbw_skill_03_hit":"cqbw_attack_01_hit_01";
    if(at.y>=GameConfig::kFloorY-8)
        TimelineLayer(clip,"dilie",seconds,{at.x,at.y-3},radius*2.4F,52);
    // C4 source smoke provides a soft, rising afterimage behind actors and warnings.
    // A normal grenade compresses that release to its existing 1.5-second lifetime.
    const float smokeTime=c4?seconds:seconds*2.7F;
    for(int i=0;i<3;++i) {
        const float offset=(i-1.0F)*radius*(.23F+.18F*seconds);
        const Vector2 p{at.x+offset,at.y-radius*.55F-seconds*(c4?29:20)};
        TimelineLayer("cqbw_skill_03_hit","smoke_03",smokeTime,p,
            radius*(c4?2.15F:1.6F),radius*(c4?2.1F:1.5F),i*29.0F,c4?.75F:.52F);
        TimelineLayer("cqbw_skill_03_hit","smoke",seconds,p,radius*2,radius*1.65F,i*47.0F,.8F);
    }
}
void BossClientFx::GrenadeTrail(const BossGrenade& g) const {
    const char* clip="cqbw_attack_01_trail_01";
    // Sample previous points on this projectile's ballistic arc. Bound the trail
    // by age so a new shell never gets smoke extending behind its launch point.
    for(int i=14;i>=1;--i) {
        const float age=i*.024F;if(age>g.age)continue;
        const Vector2 p{g.position.x-g.velocity.x*age,
            g.position.y-g.velocity.y*age+.5F*g.gravity*age*age};
        const float angle=std::atan2(g.velocity.y-g.gravity*age,g.velocity.x)*180/Pi;
        TimelineLayer(clip,"smoke_02",age,p,27+age*52,25+age*44,i*23.0F,.75F);
        TimelineLayer(clip,"smoke",age,p,22,19,angle,.65F);
        TimelineLayer(clip,"suduxian",age,p,30,4,angle,.65F,true,1.6F);
    }
}
void BossClientFx::FrostAttackRing(Vector2 at,float radius,float time,float opacity) const {
    const char* clip="frstar2_attack_02_start";
    const float seconds=time*.55F;
    const Vector2 center{at.x,at.y-60};
    if(opacity<.5F) {
        Quad("frstar_01",center,130,85,0,Fade(WHITE,opacity));
        return;
    }
    TimelineLayer(clip,"huan",seconds,center,radius*2,radius*1.15F,0,opacity);
    // The original flash lives for 0.15 s, rather than the full attack duration.
    TimelineLayer(clip,"baoshan",seconds,center,radius*1.8F,radius*1.4F,0,opacity,true);
    for(int i=0;i<18;++i) {
        const float angle=(i+.15F*(i%3))*2*Pi/18;
        const float reach=radius*(.16F+1.65F*seconds);
        const Vector2 point{center.x+std::cos(angle)*reach,center.y+std::sin(angle)*reach*.57F};
        TimelineLayer(clip,"xian",seconds,point,90,22,angle*180/Pi,opacity,true);
        if(i%2==0)TimelineLayer(clip,"sanjiao",seconds,point,32,32,i*31.0F,opacity,true);
    }
}

void BossClientFx::Revival(const StoryBoss& b,float time,float alpha) const {
    const auto f=b.Feet();const bool frost=b.Kind()==StoryBossKind::FrostNova;
    const bool reviving=b.State()==StoryBossState::Revive;
    const float phase=reviving?Saturate(time/b.RevivalDuration()):1;
    const float breathe=.65F+.15F*std::sin(b.Elapsed()*3);
    const float charge=reviving?Envelope(phase,.7F):breathe*.32F;
    BeginBlendMode(BLEND_ADDITIVE);
    if(frost) {
        Quad("frstar_02",{f.x,f.y-62},130,132,0,Fade(Ice,alpha*charge));
        Quad("frstar_03",{f.x,f.y-62},146,144,0,Fade(Ice,alpha*charge*.4F));
        for(int i=0;i<14;++i) {
            const float p=std::fmod(time*.65F+i*.137F,1.0F);
            const float x=f.x+std::sin(i*2.4F)*(38+26*p);
            Quad("trail_08",{x,f.y-12-p*165},18+25*p,3,-90,Fade(Ice,alpha*charge*Envelope(p)));
        }
    } else {
        Quad("patrt_01",{f.x,f.y-105},130,205,0,Fade(Fire,alpha*charge*.65F));
        Quad("patrt_02",{f.x,f.y-82},160,185,0,Fade(Fire,alpha*charge*.55F));
        Quad("patrt_12",{f.x,f.y-90},155,180,0,Fade(Fire,alpha*charge*.45F));
        Quad("patrt_03",{f.x,f.y-23},260,120,0,Fade(Fire,alpha*charge*.7F));
        Quad("flow_34",{f.x,f.y-30-45*phase},230,140,0,Fade(Fire,alpha*charge*.25F));
    }
    EndBlendMode();
    // Release once near the end of revival, never loop another boss's ice ring.
    if(reviving&&phase>.78F) {
        const float release=(phase-.78F)/.22F;
        if(frost)FrostAttackRing(f,260,release,alpha);
        else {
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("haak_03",{f.x,f.y-10},100+440*release,25+95*release,0,Fade(Fire,alpha*(1-release)));
            EndBlendMode();
        }
    }
}
void BossClientFx::DrawStory(const StoryBoss& b,const UiFont& font) const {
    const auto f=b.Feet();
    if(b.Kind()==StoryBossKind::Crownslayer) {
        // The blade appears at the strike, in front of the character.
        return;
    }
    if(b.Kind()!=StoryBossKind::W)return;
    for(const auto& v:b.Visuals()) {
        if(v.kind==BossFx::WExplosion||v.kind==BossFx::WC4Explosion)
            ExplosionSmoke(v.position,85*v.scale,v.age,v.kind==BossFx::WC4Explosion);
        if(v.kind==BossFx::WMuzzle)
            TimelineLayer("cqbw_attack_01_start_02","smoke",v.age,
                {v.position.x+v.facing*v.age*32,v.position.y-v.age*22},80,65,0,.8F);
    }
    for(const auto& mine:b.Mines()) {
        const Color color=mine.fuse>=0?RED:mine.age<.8F?GRAY:ORANGE;
        // cqbw_04 is the explosion debris atlas, not a complete mine sprite.
        // The baked original rig uses a stable foot anchor at (64,120)/128.
        const auto p=Vector2{mine.position.x,mine.position.y-42*(120.0F/128-.5F)};
        Quad("w_mine",p,42,42,0,WHITE);
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("star_02",{p.x+10,p.y-14},12,12,0,Fade(color,
            mine.fuse>=0?.55F+.4F*std::sin(mine.age*28):.2F+.15F*std::sin(mine.age*5)));
        EndBlendMode();
        GroundWarning(mine.position,84,mine.fuse>=0?.5F+.5F*std::sin(mine.age*24):.15F,color);
        font.Draw(mine.fuse>=0?TextFormat("地雷 %.1fs",mine.fuse):mine.age<.8F?"待激活":"地雷",p.x-27,p.y-43,14,color);
    }
    for(const auto& blast:b.Blasts()) {
        if(blast.timer>0) {
            const Color color=blast.tracking>0?ORANGE:RED;
            BeginBlendMode(BLEND_ADDITIVE);
            const float pulse=.5F+.5F*std::sin((3.2F-blast.timer)*18);
            Quad("star_02",{blast.position.x,blast.position.y-37},72,86,0,Fade(color,.16F+.12F*pulse));
            Quad("C4_02",{blast.position.x,blast.position.y-39},38,56,0,Fade(color,.6F+.3F*pulse));
            Quad("cqbw_05",{blast.position.x,blast.position.y-84},40,36,0,Fade(color,.45F+.4F*pulse));
            EndBlendMode();
            GroundWarning(blast.position,blast.radius,pulse,color);
            font.Draw(TextFormat("%.1fs",blast.timer),blast.position.x-20,blast.position.y-132,18,color);
            font.Draw(blast.tracking>0?"追踪":"锁定",blast.position.x-18,blast.position.y-105,14,color);
        } else {
            // Explosion tails are independent visual events; damage still lasts 0.35 s.
        }
    }
    for(const auto& grenade:b.Grenades())if(grenade.lifetime>0) {
        GroundWarning(grenade.landing,72,.6F,RED);
        GrenadeTrail(grenade);
        const float angle=std::atan2(grenade.velocity.y,grenade.velocity.x)*180/3.14159265F;
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("trail_08",{grenade.position.x-grenade.velocity.x*.025F,grenade.position.y-grenade.velocity.y*.025F},45,6,angle,Fade(Fire,.65F));
        Quad("star_02",grenade.position,30,24,0,Fade(Fire,.55F));
        EndBlendMode();
        Quad("cqbw_01",grenade.position,39,20,angle,WHITE);
    }
    if(b.State()==StoryBossState::GrenadeWindup||b.State()==StoryBossState::VolleyWindup||b.State()==StoryBossState::Volley) {
        const auto targets=b.State()==StoryBossState::GrenadeWindup?std::vector<float>{b.Aim().x}:b.GrenadeTargets();
        for(float x:targets)GroundWarning({x,GameConfig::kFloorY},72,.5F+.5F*std::sin(b.StateTime()*12),RED);
        if(b.State()!=StoryBossState::GrenadeWindup&&b.LowArcVolley()) {
            const char* label="跳跃避开";
            const float width=font.Measure(label,17);
            font.Draw(label,std::clamp(f.x-width/2,b.Left()+8,b.Right()-width-8),f.y-145,17,ORANGE);
        }
    }
    if(b.State()==StoryBossState::BarrageWindup||b.State()==StoryBossState::Barrage) {
        Quad("trail_08",{b.Aim().x,GameConfig::kFloorY-7},80,5,0,SKYBLUE);
        font.Draw("安全空隙",b.Aim().x-28,GameConfig::kFloorY-122,15,SKYBLUE);
    }
    if(b.State()==StoryBossState::MineWindup)for(float x:b.MineTargets()) {
        GroundWarning({x,GameConfig::kFloorY},84,.6F,ORANGE);
    }
    if(b.State()==StoryBossState::EvadeWindup||b.State()==StoryBossState::Evade) {
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("cqbw_06",{b.EvadeTarget(),GameConfig::kFloorY-21},52,48,0,Fade(SKYBLUE,.7F));
        EndBlendMode();
    }
}
void BossClientFx::Draw(const StoryBoss& b,const UiFont& font) const {
    if(!b.TwoLives()||!b.Active())return;const auto f=b.Feet();const bool frost=b.Kind()==StoryBossKind::FrostNova;
    if(b.State()==StoryBossState::Revive)Revival(b,b.StateTime(),.75F);
    else if(b.Invulnerability()>0)Revival(b,b.Elapsed(),.65F);
    if(frost&&b.PhaseTwo()) {
        for(float x:{b.Left()+30,b.Right()-30}) {
            Quad("frstar_05",{x,GameConfig::kFloorY-90},108,205,0,WHITE);
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("frstar_07",{x,GameConfig::kFloorY-93},115,190,0,Fade(Ice,.22F+.12F*std::sin(b.AnimationTime()*2)));
            EndBlendMode();
        }
    }
    for(const auto& z:b.Zones()) {
        if(z.kind==BossFx::SpearRain&&z.delay>0&&z.remaining>0) {
            GroundWarning(z.position,z.radius,.6F,Fire);
            Quad("trail_08",{z.position.x,z.position.y-7},z.radius*2,4,0,Fire);
            Quad("trail_08",{z.position.x,z.position.y-181},348,3,90,Fade(Fire,.6F));
            DrawSpear({z.position.x,z.position.y-180},160,180,Fade(WHITE,.45F));
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("trail_15",{z.position.x,z.position.y-202},350,5,90,Fade(Fire,.35F));
            EndBlendMode();
            TerminalUi::Surface({z.position.x-43,z.position.y-64,86,24},Fade(TerminalUi::Ink,.9F),Fire);
            font.Draw(TextFormat("%.1fs",z.delay),z.position.x-20,z.position.y-60,15,Fire);
        }
        if(z.kind==BossFx::BlackIce&&z.remaining>0) {
            if(z.delay>0) {
                // frstar2_skill_03_range: the client seals ground tiles, not a crystal pillar.
                Quad("water_05",{z.position.x,z.position.y-12},z.radius*2,36,0,Fade(Ice,.45F));
                BeginBlendMode(BLEND_ADDITIVE);
                Quad("trail_08",{z.position.x,z.position.y-2},z.radius*2,10,0,Fade(Ice,.65F));
                EndBlendMode();
                TerminalUi::Surface({z.position.x-57,z.position.y-82,114,24},Fade(TerminalUi::Ink,.88F),Ice);
                font.Draw(TextFormat("%.1fs",z.delay),z.position.x-20,z.position.y-78,13,Ice);
            } else {
                const float growth=std::clamp((z.age-2.4F)/.25F,0.0F,1.0F);
                Quad("dilie_08",{z.position.x,z.position.y+3},z.radius*2,52,0,WHITE);
                BeginBlendMode(BLEND_ADDITIVE);
                Quad("water_05",{z.position.x,z.position.y-8},z.radius*2,42,0,Fade(Ice,.24F+.24F*growth));
                EndBlendMode();
                TerminalUi::Surface({z.position.x-57,z.position.y-92,114,24},Fade(TerminalUi::Ink,.88F),Ice);
                font.Draw(TextFormat("攻击修复 %.0f/4",std::ceil(z.durability)),z.position.x-51,z.position.y-88,13,Ice);
            }
        }
        if(z.kind==BossFx::IceRain&&z.remaining>0) {
            if(z.delay>0) {
                const float warning=1.0F-std::clamp(z.delay/2.15F,0.0F,1.0F);
                BeginBlendMode(BLEND_ADDITIVE);
                Quad("water_05",{z.position.x,z.position.y-10},z.radius*2,38,0,Fade(Ice,.65F+.3F*warning));
                Quad("frstar_07",{z.position.x,z.position.y-143},68,252,180,Fade(Ice,.16F+.2F*warning));
                Quad("xue_01",{z.position.x,z.position.y-155},110,265,0,Fade(Ice,.21F+.2F*warning));
                EndBlendMode();
                TerminalUi::Surface({z.position.x-43,z.position.y-77,86,23},Fade(TerminalUi::Ink,.88F),Ice);
                font.Draw(TextFormat("%.1fs",z.delay),z.position.x-20,z.position.y-73,13,Ice);
            } else {
                const float fade=std::clamp(z.remaining/.36F,0.0F,1.0F);
                Quad("frstar_07",{z.position.x,z.position.y-137},120,275,180,Fade(Ice,fade));
                BeginBlendMode(BLEND_ADDITIVE);
                Quad("frstar_09",{z.position.x,z.position.y-40},135,125,0,Fade(Ice,fade*.7F));
                EndBlendMode();
            }
        }
    }
    for(const auto& v:b.Visuals()) {
        const float t=Saturate(v.age/std::max(.001F,v.duration)),fade=1-t;
        switch(v.kind) {
        case BossFx::IceRing:break; // Draw over the sprite in DrawForeground.
        case BossFx::Ritual: {
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("patrt_03",{v.position.x,v.position.y-38},410,160,0,Fade(Fire,fade*.7F));
            Quad("patrt_13",{v.position.x,v.position.y-60},280,230,0,Fade(Fire,fade*.35F));
            Quad("haak_03",{v.position.x,v.position.y-10},410,60,t*5,Fade(Fire,fade*.45F));EndBlendMode();break;
        }
        case BossFx::PatriotSlash:break;
        case BossFx::Impact: {
            BeginBlendMode(BLEND_ADDITIVE);
            // patrt_14/15 are body silhouettes for the cast, not ground explosions.
            const float extent=(70+160*std::sqrt(t))*v.scale;
            Quad("haak_03",{v.position.x,v.position.y-8},extent,extent*.3F,0,Fade(Fire,fade));
            Quad("patrt_03",{v.position.x,v.position.y-25},extent,80+50*t,0,Fade(Fire,fade*.7F));
            Quad("star_55",{v.position.x,v.position.y-22},90,100,0,Fade(WHITE,Saturate(1-t*4)));
            EndBlendMode();break;
        }
        case BossFx::FrostBolt:
            BeginBlendMode(BLEND_ADDITIVE);Quad("star_55",v.position,75,75,t*90,Fade(Ice,fade*.8F));EndBlendMode();break;
        case BossFx::FrostTrail:
            Quad("xue_01",{v.position.x,v.position.y-20*t},120+60*t,85+45*t,0,Fade(Ice,fade*.55F));
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("frstar_09",v.position,90+70*t,45,0,Fade(Ice,fade*.45F));
            EndBlendMode();break;
        default:break;
        }
    }
    for(const auto& m:b.Missiles())if(m.life>0) {
        if(m.kind==BossFx::Spear) {
            const float angle=std::atan2(m.velocity.y,m.velocity.x)*180/3.14159265F+90;
            const float length=std::max(1.0F,std::hypot(m.velocity.x,m.velocity.y));
            const Vector2 direction{m.velocity.x/length,m.velocity.y/length};
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("trail_08",{m.position.x-direction.x*11.5F,m.position.y-direction.y*11.5F},173,7,angle-90,Fade(Fire,.65F));
            EndBlendMode();
            DrawSpear(m.position,178,angle,WHITE);
        } else if(m.kind==BossFx::ColdWave) {
            Quad("frstar_05",{m.position.x,m.position.y-18},105,115,m.velocity.x>0?25:-25,Fade(WHITE,.8F));
            BeginBlendMode(BLEND_ADDITIVE);Quad("frstar_07",{m.position.x,m.position.y-15},130,145,0,Fade(Ice,.8F));EndBlendMode();
        } else if(m.kind==BossFx::GroundWave) {
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("patrt_03",{m.position.x,m.position.y-12},105,78,0,Fade(Fire,.7F));
            Quad("patrt_07",{m.position.x,m.position.y-9},110,80,m.velocity.x>0?0:180,Fade(WHITE,.65F));
            EndBlendMode();
        } else {
            const float angle=std::atan2(m.velocity.y,m.velocity.x)*180/3.14159265F;
            BeginBlendMode(BLEND_ADDITIVE);
            Quad("trail_08",{m.position.x-m.velocity.x*.065F,m.position.y-m.velocity.y*.065F},80,20,angle,Fade(Ice,.8F));
            Quad("flipbook_06",m.position,48,19,angle,Ice,int(m.age*16)%4,1,4);
            Quad("star_55",m.position,35,35,m.age*90,Fade(Ice,.65F));
            EndBlendMode();
        }
    }
    if(b.State()==StoryBossState::FrostStepWindup||b.State()==StoryBossState::FrostStep) {
        const Vector2 destination{b.MobilityTarget(),GameConfig::kFloorY};
        GroundWarning(destination,60,.8F,Ice);
        if(b.State()==StoryBossState::FrostStepWindup)
            font.Draw(TextFormat("变换站位  %.1fs",std::max(0.0F,.85F-b.StateTime())),f.x-85,f.y-155,18,Ice);
        Quad("xue_01",{f.x,f.y-50},140,135,b.StateTime()*12,Fade(Ice,.45F));
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("frstar_09",{f.x,f.y-28},130,85,0,Fade(Ice,.6F));
        EndBlendMode();
    }
    if(b.State()==StoryBossState::LeapWindup||b.State()==StoryBossState::Leap) {
        const Vector2 destination{b.MobilityTarget(),GameConfig::kFloorY};
        GroundWarning(destination,145,.85F,Fire);
        Quad("zhunxing_01",{destination.x,destination.y-10},290,45,0,Fade(Fire,.9F));
        const float remaining=std::max(0.0F,(b.State()==StoryBossState::LeapWindup?1.9F:.8F)-b.StateTime());
        font.Draw(TextFormat("离开落点  %.1fs",remaining),destination.x-85,destination.y-105,18,Fire);
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("patrt_16",{f.x+b.Facing()*30,f.y-105},40,200,0,Fade(Fire,.7F));
        if(b.State()==StoryBossState::Leap)Quad("patrt_07",{f.x-b.Facing()*45,f.y-75},170,140,b.Facing()>0?0:180,Fade(Fire,.65F));
        EndBlendMode();
    }
    if(b.State()==StoryBossState::IceRingWindup) {
        const float r=b.PhaseTwo()?390:250;
        FrostAttackRing(f,r,0.0F,.22F+.08F*std::sin(b.StateTime()*12));
        font.Draw("跳跃避开",f.x-36,f.y-155,17,Ice);
    }
    if(b.State()==StoryBossState::FrostCast) {
        BeginBlendMode(BLEND_ADDITIVE);Quad("star_55",{f.x+b.Facing()*28,f.y-64},80,80,b.StateTime()*80,Fade(Ice,.65F));EndBlendMode();
    }
    if(b.State()==StoryBossState::FrostFanWindup) {
        const Vector2 start{f.x+b.Facing()*28,f.y-64};
        for(auto v:b.FrostFanVelocities()) {
            const float length=std::max(1.0F,std::hypot(v.x,v.y));
            Quad("trail_08",{start.x+v.x/length*90,start.y+v.y/length*90},180,3,
                 std::atan2(v.y,v.x)*180/Pi,Fade(Ice,.65F));
        }
        font.Draw("变换站位",f.x-36,f.y-152,17,Ice);
    }
    if(b.State()==StoryBossState::IceRainWindup) {
        BeginBlendMode(BLEND_ADDITIVE);
        const int count=b.PhaseTwo()?5:3;
        const float step=b.PhaseTwo()?125.0F:150.0F;
        const float halfSpan=(count-1)*step*.5F;
        const float center=std::clamp(b.Aim().x,b.Left()+55+halfSpan,b.Right()-55-halfSpan);
        for(int i=0;i<count;++i) {
            const float x=center+(i-count/2)*step;
            Quad("water_05",{x,GameConfig::kFloorY-12},100,38,0,Fade(Ice,.7F));
            Quad("frstar_07",{x,GameConfig::kFloorY-130},58,215,180,Fade(Ice,.16F));
        }
        EndBlendMode();
        TerminalUi::Surface({b.Aim().x-86,GameConfig::kFloorY-236,172,28},Fade(TerminalUi::Ink,.9F),Ice);
        font.Draw("离开落点",b.Aim().x-36,GameConfig::kFloorY-231,16,Ice);
    }
    if(b.State()==StoryBossState::ComboWindup) {
        const float reach=b.PhaseTwo()?255:165;
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("trail_07",{f.x+b.Facing()*reach*.5F,f.y-5},reach,20,b.Facing()>0?0:180,Fade(Fire,.65F));
        EndBlendMode();
    }
    if(b.State()==StoryBossState::ChargeWindup) {
        const float length=std::min(670.0F,std::abs(b.Aim().x-f.x)+65.0F);
        const float direction=b.Facing()>0?1.0F:-1.0F;
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("patrt_16",{f.x+direction*30,f.y-106},42,205,0,Fade(Fire,.58F));
        Quad("trail_15",{f.x+direction*length*.5F,f.y-21},length,16,b.Facing()>0?0:180,Fade(Fire,.62F));
        EndBlendMode();
        font.Draw("跳跃或闪避",f.x-45,f.y-208,17,Fire);
    }
    if(b.State()==StoryBossState::Charge) {
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("patrt_07",{f.x-b.Facing()*55,f.y-76},175,140,b.Facing()>0?0:180,Fade(Fire,.75F));
        EndBlendMode();
    }
    if(b.State()==StoryBossState::GuardWindup||b.State()==StoryBossState::Guard) {
        const float alpha=b.State()==StoryBossState::Guard?.85F:.38F+.35F*std::sin(b.StateTime()*12);
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("patrt_04",{f.x+b.Facing()*87,f.y-100},155,190,0,Fade(Fire,alpha*.75F));
        Quad("patrt_12",{f.x+b.Facing()*72,f.y-95},130,175,0,Fade(Fire,alpha*.65F));
        Quad("patrt_06",{f.x+b.Facing()*96,f.y-95},70,200,0,Fade(Fire,alpha));
        EndBlendMode();
        font.Draw(b.State()==StoryBossState::GuardWindup?"绕后":"背击或破盾",
            f.x-45,f.y-210,17,Fire);
    }
    if(b.State()==StoryBossState::SpearWindup) {
        const bool locked=b.StateTime()>=1.05F;const auto target=b.Aim();
        BeginBlendMode(BLEND_ADDITIVE);
        Quad("zhunxing_01",target,84,84,locked?0:b.StateTime()*30,locked?RED:Fire);
        Quad("patrt_16",{f.x+b.Facing()*30,f.y-122},30,195,0,Fade(Fire,.6F));EndBlendMode();
        const Vector2 start{f.x+b.Facing()*40,f.y-135};
        const float angle=std::atan2(target.y-start.y,target.x-start.x)*180/3.14159265F;
        Quad("trail_15",{(start.x+target.x)*.5F,(start.y+target.y)*.5F},std::hypot(target.x-start.x,target.y-start.y),6,angle,Fade(locked?RED:Fire,.45F));
        font.Draw(locked?"已锁定":"正在瞄准",target.x-36,target.y-100,17,locked?RED:Fire);
    }
}
void BossClientFx::DrawForeground(const StoryBoss& b) const {
    if(!b.Active())return;
    for(const auto& v:b.Visuals()) {
        const float rotation=v.facing>0?0.0F:180.0F;
        if(v.kind==BossFx::WMuzzle) {
            const char* clip="cqbw_attack_01_start";
            const Vector2 point{v.position.x+v.facing*16,v.position.y};
            TimelineLayer(clip,"andi",v.age,point,48,36,rotation);
            TimelineLayer(clip,"suduxian_andi",v.age,point,52,22,rotation);
            TimelineLayer(clip,"qiangkou",v.age,point,66,40,rotation,1,true,1.8F);
            TimelineLayer(clip,"baoshan",v.age,point,55,48,rotation,1,true,1.8F);
            for(int i=0;i<4;++i) {
                const Vector2 spark{point.x+v.facing*v.age*(80+i*22),point.y+(i-1.5F)*v.age*35};
                TimelineLayer(clip,"suduxian",v.age,spark,42,4,rotation,.8F,true,1.8F);
                TimelineLayer(clip,"xingdian",v.age,spark,15,15,i*27.0F,.8F,true,1.8F);
            }
        }
        if(v.kind==BossFx::WExplosion||v.kind==BossFx::WC4Explosion)
            Explosion(v.position,85*v.scale,v.age,v.kind==BossFx::WC4Explosion);
        if(v.kind==BossFx::IceRing)FrostAttackRing(v.position,250*v.scale,v.age/.55F,1);
        if(v.kind==BossFx::CrownSlash) {
            const char* clip="crowns_attack_01_start_right";
            TimelineLayer(clip,"andi",v.age,v.position,125,112,rotation);
            TimelineLayer(clip,"liang",v.age,v.position,125,112,rotation,1,true);
            for(int i=0;i<5;++i)TimelineLayer(clip,"suduxian",v.age,
                {v.position.x+v.facing*(i*8+v.age*90),v.position.y-25+i*12},42,4,rotation,1,true);
        }
        if(v.kind==BossFx::PatriotSlash) {
            const char* clip="patrt_attack_01_start";
            // Source attack emits its weapon trail at 0.60 s. The gameplay event
            // already fires on contact, so sample from that source timestamp.
            const float sourceTime=v.age+.60F;
            TimelineLayer(clip,"trail",sourceTime,v.position,190*v.scale,145*v.scale,rotation);
            TimelineLayer(clip,"baoshan",sourceTime,v.position,155*v.scale,90*v.scale,rotation,1,true);
            TimelineLayer(clip,"baoci",sourceTime,v.position,150*v.scale,75*v.scale,rotation,1,true);
            TimelineLayer(clip,"shandian",sourceTime,v.position,100*v.scale,80*v.scale,rotation,1,true);
            for(int i=0;i<6;++i)TimelineLayer(clip,"suduxian",sourceTime,
                {v.position.x+v.facing*v.age*160,v.position.y-26+i*10},90*v.scale,5,rotation,1,true);
        }
    }
}

void BossClientFx::Weather(const StoryBoss& b,float camera,bool foreground) const {
    if(b.Kind()!=StoryBossKind::FrostNova||b.FrostWeather()<=0)return;
    const float strength=b.FrostWeather(),clock=b.WeatherTime();
    const float storm=std::clamp((strength-.72F)/.28F,0.0F,1.0F);
    const int count=foreground?28:110;
    const float parallax=foreground?.32F:.10F;
    const auto wrap=[](float value,float period){return value-std::floor(value/period)*period;};
    // Two source-texture snow layers: gentle far flakes and sparse faster near flakes.
    // Wind is integrated into position so gusts do not teleport the particles.
    const float wind=-clock*(foreground?42.0F:20.0F)+18*std::sin(clock*.7F);
    for(int i=0;i<count;++i) {
        const float seed=float(i)+(foreground?137:0);
        const float speed=(foreground?85.0F:30.0F)+float(i%7)*6;
        const float x=wrap(seed*173.73F+wind*(.8F+(i%5)*.13F)-camera*parallax+
                           std::sin(clock*(.55F+(i%4)*.09F)+seed)*16,1400)-60;
        const float y=wrap(seed*91.17F+clock*speed,790)-60;
        const float edge=std::clamp((y+30)/60,0.0F,1.0F)*std::clamp((690-y)/65,0.0F,1.0F);
        const float size=(foreground?13.0F:6.0F)+i%5*1.6F;
        const float density=i%4==0?strength:std::clamp(strength*1.5F-.18F,0.0F,1.0F);
        Quad("flipbook_51",{x,y},size,size,seed*31+clock*(i%2?11:-9),
             Fade(Color{229,243,255,255},edge*density*(foreground?.56F:.70F)),i%4,2,2);
    }
    if(!foreground) {
        // Original snow-cloud texture hugs the pavement, behind actors and attack markers.
        for(int i=0;i<6;++i) {
            const float x=wrap(i*271.0F-clock*18-camera*.15F,1750)-240;
            Quad("xue_01",{x,GameConfig::kFloorY-28+8*std::sin(clock*.5F+i)},
                 1600,290,0,Fade(Color{169,201,223,255},strength*(.20F+.10F*storm)));
        }
    }
}
void BossClientFx::Atmosphere(const StoryBoss& b,float camera,const UiFont& font) const {
    Weather(b,camera,true);
    if(!b.TwoLives()||!b.Active())return;const bool frost=b.Kind()==StoryBossKind::FrostNova;
    const float clock=b.Elapsed();
    if(!frost) {
    BeginBlendMode(BLEND_ADDITIVE);
    for(int i=0;i<32;++i) {
        const float x=std::fmod(i*147.37F-camera*.14F+clock*(frost?-12:8)+10000,1280.0F);
        const float y=std::fmod(i*86.31F+clock*(frost?28:-18)+10000,650.0F);
        Quad(frost?"flipbook_51":"star_02",{x,y},frost?4:5,frost?5:8,i*37,Fade(frost?Ice:Fire,b.PhaseTwo()?.45F:.22F),frost?i%4:0,frost?2:1,frost?2:1);
    }
    EndBlendMode();
    }
    if(b.State()==StoryBossState::Revive&&b.StateTime()<1.9F) {
        const float alpha=std::clamp(b.StateTime()/.25F,0.0F,1.0F)*std::clamp((1.9F-b.StateTime())/.35F,0.0F,1.0F);
        DrawRectangle(0,0,1280,8,Fade(frost?Ice:Fire,alpha));
        TerminalUi::Surface({434,283,520,95},Fade(TerminalUi::Ink,.92F*alpha),Fade(frost?Ice:Fire,alpha));
        font.Draw(frost?"霜星 · 第二阶段":"爱国者 · 第二阶段",456,298,30,Fade(frost?Ice:Fire,alpha));
        font.Draw("PHASE 01  >  PHASE 02",457,347,16,Fade(WHITE,alpha));
    }
}
