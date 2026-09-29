#include "enemy_renderer.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "file_path.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace {
// Ranged units aim along a line; melee units sweep an arc. Kept local to the renderer
// because the simulation has its own copy of the same predicate.
bool IsRangedTelegraph(EnemyKind kind) {
    return kind==EnemyKind::Crossbow || kind==EnemyKind::Drone
        || kind==EnemyKind::Caster || kind==EnemyKind::StealthCrossbow
        || kind==EnemyKind::AcidSlug;
}
Color BodyColor(EnemyKind kind) {
    switch(kind) {
        case EnemyKind::SlugHigh: return TacticalUi::kRed;
        // 辐能源石虫 detonates with Arts, so it reads violet-red rather than the
        // 高能源石虫's plain physical red.
        case EnemyKind::IrrSlug: return Color{214,110,214,255};
        case EnemyKind::AcidSlug: return Color{132,204,88,255};
        case EnemyKind::Crossbow:
        case EnemyKind::StealthCrossbow: return TacticalUi::kCyan;
        case EnemyKind::Shield: return Color{180,171,132,255};
        case EnemyKind::Caster: return Color{158,120,220,255};
        case EnemyKind::Hound: return Color{214,96,64,255};
        default: return TacticalUi::kOrange;
    }
}
// The detonation ring matches the damage type: 辐能源石虫's is Arts.
Color BlastColor(EnemyKind kind) {
    return kind==EnemyKind::IrrSlug ? Color{186,132,224,255} : RED;
}
}

EnemyRenderer::EnemyRenderer() {
    const auto root=Utf8Path(GetApplicationDirectory())/"assets/enemies/mobs";
    nlohmann::json manifest;
    try {std::ifstream in(root/"manifest.json");if(in)in>>manifest;}
    catch(const std::exception& e){TraceLog(LOG_WARNING,"Enemy metadata: %s",e.what());}
    for(const auto& d:EnemyDefinitions()) {
        const auto index=static_cast<unsigned>(d.kind);anchors_[index]={96,180};
        durations_[index]={.833333F,.833333F,1, .8F};
        facing_[index]=d.kind==EnemyKind::Shield?std::array<int,4>{-1,1,-1,-1}:std::array<int,4>{1,1,1,1};
        std::string filename=std::string(d.id)+".png";
        if(manifest.contains(d.id)) {
            const auto& m=manifest.at(d.id);
            filename=m.value("file",filename);
            if(m.contains("durations")) durations_[index]=m.at("durations").get<std::array<float,4>>();
            if(m.contains("facing")) facing_[index]=m.at("facing").get<std::array<int,4>>();
        }
        try { if(manifest.contains(d.id)&&manifest.at(d.id).contains("anchor")) {
            const auto& a=manifest.at(d.id).at("anchor");anchors_[index]={a.at(0).get<float>(),a.at(1).get<float>()};
        }} catch(const std::exception& e){TraceLog(LOG_WARNING,"Enemy anchor: %s",e.what());}
        const auto path=(root/filename).string();
        if(FileExists(path.c_str())) {
            auto t=LoadTexture(path.c_str());
            if(t.id && t.width==1920 && t.height==768) {SetTextureFilter(t,TEXTURE_FILTER_BILINEAR);textures_[index]=t;}
            else if(t.id) UnloadTexture(t);
        }
        if(!textures_[index].id && d.kind!=EnemyKind::Drone)
            TraceLog(LOG_WARNING,"Enemy sprite unavailable: %s",d.id);
    }
    // The baker writes one sheet per model, so verify every kind actually loaded rather
    // than letting a silent 0-id texture fall through to the "缺少素材" placeholder.
    for(const auto& d:EnemyDefinitions()) {
        const auto i=static_cast<unsigned>(d.kind);
        if(!textures_[i].id) TraceLog(LOG_WARNING,"Enemy %s has no baked sheet; placeholder will draw",d.id);
    }
}
EnemyRenderer::~EnemyRenderer(){for(auto t:textures_)if(t.id)UnloadTexture(t);}
bool EnemyRenderer::HasSprite(EnemyKind kind)const{
    return textures_.at(static_cast<unsigned>(kind)).id!=0;
}
void EnemyRenderer::DrawUnit(const EnemyUnit& u,const UiFont& font)const {
    const auto& d=EnemyData(u.kind);const auto i=static_cast<unsigned>(u.kind);
    if(u.state==EnemyState::Dead && u.animationTime>.8F) return;
    const auto color=BodyColor(u.kind);
    if(u.state==EnemyState::Fuse) {
        const float ratio=1-std::clamp(u.timer/d.windup,0.0F,1.0F);
        const Color ring=BlastColor(u.kind);
        DrawCircleV(u.Center(),EnemySystem::kBlastRadius,Fade(ring,.05F+ratio*.12F));
        DrawCircleLines(int(u.feet.x),int(u.Center().y),EnemySystem::kBlastRadius,Fade(ring,.8F));
        DrawCircleV(u.Center(),EnemySystem::kBlastRadius*ratio,Fade(ORANGE,.1F));
        font.Draw(TextFormat("蓄爆 %.1fs",u.timer),u.feet.x-55,u.feet.y-d.height-61,20,ring);
    }
    if(u.state==EnemyState::Blast) {
        const Color ring=BlastColor(u.kind);
        DrawCircleV(u.Center(),EnemySystem::kBlastRadius,Fade(ring,std::clamp(u.timer/.35F,0.0F,1.0F)*.65F));
        DrawCircleLines(int(u.feet.x),int(u.Center().y),EnemySystem::kBlastRadius,RAYWHITE);
        return;
    }
    if(u.state==EnemyState::Windup) {
        if(IsRangedTelegraph(u.kind)) {
            const Color line=u.kind==EnemyKind::Caster?Color{178,140,235,255}
                :u.kind==EnemyKind::AcidSlug?Color{132,204,88,255}:Fade(RED,.6F);
            const float originX=u.feet.x+u.facing*24.0F;
            DrawLineEx({originX,u.feet.y-62},u.aim,2,line);
            DrawCircleLines(int(u.aim.x),int(u.aim.y),12,line);
            // A cloaked archer shows its aim line even though the body is hidden, so the
            // shot is always something the player can read and dodge.
            if(u.cloaked) DrawCircleV({originX,u.feet.y-62},7,line);
        } else {
            DrawRectangleRec({u.facing>0?u.feet.x:u.feet.x-d.reach,u.feet.y-8,d.reach,8},Fade(ORANGE,.5F));
        }
        font.Draw("!",u.feet.x-5,u.feet.y-d.height-62,28,RED);
    }
    int row=u.moving?1:0,frame=int(u.animationTime*10/std::max(.01F,durations_[i][row]))%10;
    if(u.state==EnemyState::Windup){row=2;frame=std::clamp(int((1-u.timer/d.windup)*4),0,3);}
    if(u.state==EnemyState::Strike){row=2;frame=4+std::clamp(int(u.animationTime/.22F*4),0,3);}
    if(u.state==EnemyState::Recover && u.stun<=0){row=2;frame=8+std::min(1,int(u.animationTime/std::max(.01F,d.recovery)*2));}
    if(u.state==EnemyState::Dead){row=3;frame=std::min(9,int(u.animationTime/.8F*10));}
    // shield_v2.png is not a single-direction bake: whatever pipeline produced it left
    // the Idle row mirrored relative to the rig and the Move row un-mirrored, so the
    // Attack row genuinely inherits both. Verified against the model author's own
    // skeleton (cmake-build-debug/enemy-reference/spine/shield/enemy_1006_shield.skel,
    // re-rendered headlessly): for every baked cell we compare it with the raw skeleton
    // render and with that render mirrored - a match to the mirror means the cell was
    // authored the other way round, and the comparison is against the source model so
    // it cannot be circular. Result: Idle all 10 frames match the MIRROR (mirrored at
    // bake time), Move all 10 match the raw render, and Attack splits at frame 4 -
    // f0-3 and f9 match the mirror (drawn like Idle), f4-8 match the raw render (drawn
    // like Move). A single per-row number cannot express that, hence this table.
    //
    // Do NOT "simplify" this by measuring the alpha centroid: the shield's idle clip is
    // a near front-on guard (neighbouring idle frames differ by ~2.5/255), so the
    // centroid reads -5 in the legs band and +4 in the helmet band for the same frame
    // and flips sign depending on which band you sample.
    static constexpr std::array<bool,10> kShieldAttackNativeRight{
        false,false,false,false, true,true,true,true,true, false};
    const int nativeFacing = (i==static_cast<unsigned>(EnemyKind::Shield) && row==2)
                                 ? (kShieldAttackNativeRight[std::clamp(frame,0,9)] ? 1 : -1)
                                 : facing_[i][row];
    const bool flip=u.facing!=nativeFacing;
    const float anchorX=flip?192-anchors_[i].x:anchors_[i].x;
    Color tint=WHITE;
    if(u.flash>0)tint=u.blocked?SKYBLUE:Color{255,134,134,255};
    if(u.state==EnemyState::Fuse && int(u.animationTime*14)%2)tint=ORANGE;
    // Cloak reads as a faint silhouette rather than being invisible: a fully hidden body
    // would be indistinguishable from a bug, and the player still needs to see where it
    // is to decide whether to close in.
    if(u.cloaked)tint=Fade(Color{150,168,190,255},.20F);
    if(u.state==EnemyState::Dead)tint=Fade(tint,1-std::clamp((u.animationTime-.4F)/.4F,0.0F,1.0F));
    DrawEllipse(int(u.feet.x),int(u.feet.y),d.width*.6F,6,Fade(BLACK,.18F));
    if(u.kind==EnemyKind::Drone && !textures_[i].id) {
        const Vector2 c{u.feet.x,u.feet.y-d.height*.52F};
        const Color body=Color{68,78,87,255}, glow=Color{78,204,218,255};
        DrawLineEx({c.x-34,c.y-18},{c.x+34,c.y-18},4,Fade(glow,.65F));
        DrawLineEx({c.x-26,c.y-30},{c.x+26,c.y-30},3,Fade(glow,.50F));
        DrawCircleV(c,26,body);DrawRectangleRec({c.x-35,c.y-9,70,18},body);
        DrawCircleV({c.x-13,c.y-2},5,glow);DrawCircleV({c.x+13,c.y-2},5,glow);
        DrawLineEx({c.x-17,c.y+21},{c.x-31,c.y+31},4,body);DrawLineEx({c.x+17,c.y+21},{c.x+31,c.y+31},4,body);
        DrawCircleV({c.x,c.y+31},6,glow);
    } else if(textures_[i].id) DrawTexturePro(textures_[i],{frame*192.0F,row*192.0F,flip?-192.0F:192.0F,192},
        {u.feet.x-anchorX/192*d.spriteSize,u.feet.y-anchors_[i].y/192*d.spriteSize,d.spriteSize,d.spriteSize},{},0,tint);
    else {DrawRectangleRec(u.Hitbox(),Fade(color,.7F));font.Draw("缺少素材",u.feet.x-36,u.Center().y,14,BLACK);}
    if(u.state==EnemyState::Dead)return;
    DrawRectangle(int(u.feet.x-35),int(u.feet.y-d.height-18),70,5,Fade(BLACK,.65F));
    DrawRectangle(int(u.feet.x-35),int(u.feet.y-d.height-18),int(70*u.health/d.health),5,color);
    const float labelWidth=font.Measure(d.name,15);
    font.Draw(d.name,u.feet.x-labelWidth/2,u.feet.y-d.height-39,15,TacticalUi::kInk);
    if(u.Guarding()) DrawLineEx({u.feet.x+u.facing*(d.width/2+5),u.feet.y-d.height+16},{u.feet.x+u.facing*(d.width/2+5),u.feet.y-5},3,Fade(SKYBLUE,.75F));
    if(u.flash>0 && u.blocked)font.Draw("格挡",u.feet.x-20,u.feet.y-d.height-60,20,BLUE);
    // Only STEALTH units are ever cloaked, so this is unambiguous. Still worth labelling:
    // otherwise the faint silhouette reads as a rendering fault rather than a mechanic.
    if(u.cloaked) {
        DrawCircleLines(int(u.feet.x),int(u.feet.y-d.height/2),d.width*.85F,Fade(Color{150,168,190,255},.45F));
        font.Draw("隐身",u.feet.x-20,u.feet.y-d.height-60,20,Fade(Color{150,168,190,255},.9F));
    }
    // 辐能源石虫's parting gift. Without a cue the speed-up just looks like the enemies
    // moved oddly, and the player has no way to connect it to the slug they killed.
    if(u.haste>0) {
        DrawCircleLines(int(u.feet.x),int(u.feet.y-d.height/2),d.width*.95F,Fade(Color{214,110,214,255},.5F));
        font.Draw("供能",u.feet.x-20,u.feet.y-d.height-60,20,Color{214,110,214,255});
    }
}
void EnemyRenderer::Draw(const EnemySystem& system,const UiFont& font)const {
    for(const auto& u:system.Units())DrawUnit(u,font);
    for(const auto& b:system.Bolts()) if(b.lifetime>0) {
        const float length=std::max(1.0F,std::sqrt(b.velocity.x*b.velocity.x+b.velocity.y*b.velocity.y));
        const Vector2 tail{b.position.x-b.velocity.x/length*24,b.position.y-b.velocity.y/length*24};
        // Caster shots are Arts (violet mote, pierces the guard's physical reduction) and
        // acid slugs spit a corrosive glob (green, shortens the operator's i-frames).
        const Color head=b.corrosive?Color{132,204,88,255}
            :b.arts?Color{178,140,235,255}:TacticalUi::kOrange;
        const Color trail=b.corrosive?Color{74,116,52,255}
            :b.arts?Color{120,96,178,255}:TacticalUi::kInk;
        DrawLineEx(tail,b.position,4,trail);
        DrawCircleV(b.position,b.corrosive?7.0F:4.0F,head);
    }
}
