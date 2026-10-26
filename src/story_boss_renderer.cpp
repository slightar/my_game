#include "story_boss_renderer.h"
#include "ui_font.h"
#include "boss_animation_timing.h"
#include "ui_theme.h"
#include "file_path.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

StoryBossRenderer::StoryBossRenderer() {
    const auto root=Utf8Path(GetApplicationDirectory())/"assets/enemies/bosses";
    try {
        nlohmann::json metadata;std::ifstream in(root/"manifest.json");in>>metadata;
        for(int i=0;i<kStoryBossCount;++i) {
            const char* id=StoryBossId(static_cast<StoryBossKind>(i+1));
            const auto& m=metadata.at(id);auto& a=art_[i];
            a.cell=m.at("cell").get<int>();
            a.texture=LoadTexture((root/(std::string(id)+".png")).string().c_str());
            if(!a.texture.id||a.texture.width!=20*a.cell||a.texture.height!=m.at("rows").get<int>()*a.cell)
                throw std::runtime_error(std::string("Invalid original Boss sheet: ")+id);
            SetTextureFilter(a.texture,TEXTURE_FILTER_BILINEAR);
            a.anchor={m.at("anchor").at(0).get<float>(),m.at("anchor").at(1).get<float>()};
            a.size=m.at("sprite_size").get<float>();
            for(int row=0;row<m.at("durations").size();++row)a.durations[row]=m.at("durations").at(row);
        }
    }catch(const std::exception& e){TraceLog(LOG_WARNING,"Story Boss assets: %s",e.what());}
    crownBattle_=LoadTexture((root.parent_path()/"crownslayer_battle.png").string().c_str());
    if(crownBattle_.id && crownBattle_.width==2560 && crownBattle_.height==1920)
        SetTextureFilter(crownBattle_,TEXTURE_FILTER_BILINEAR);
    else {
        if(crownBattle_.id)UnloadTexture(crownBattle_);
        crownBattle_={};TraceLog(LOG_WARNING,"Crownslayer B-battle sheet missing or invalid");
    }
    try {
        nlohmann::json m;std::ifstream in(root/"frostnova_smooth.json");in>>m;
        if(m.at("model")!="enemy_1510_frstar2"||m.at("fps").get<int>()<30)throw std::runtime_error("Invalid 30 Hz Winter Scar source");
        frostSmooth_=LoadTexture((root/"frostnova_smooth.png").string().c_str());
        if(!frostSmooth_.id||frostSmooth_.width!=m.at("width").get<int>()||frostSmooth_.height!=m.at("height").get<int>())
            throw std::runtime_error("Invalid Winter Scar trimmed animation atlas");
        SetTextureFilter(frostSmooth_,TEXTURE_FILTER_BILINEAR);
        frostSmoothAnchor_={m.at("anchor").at(0).get<float>(),m.at("anchor").at(1).get<float>()};
        constexpr const char* clips[]{"Idle","Move","Attack","Skill_1","Skill_2","Skill_3","Die"};
        for(int i=0;i<7;++i) {
            const auto& c=m.at("animations").at(clips[i]);auto& dest=frostClips_[i];
            dest.duration=c.at("duration").get<float>();
            for(const auto& f:c.at("frames"))dest.frames.push_back({
                {f.at("x").get<float>(),f.at("y").get<float>(),f.at("w").get<float>(),f.at("h").get<float>()},
                {f.at("bx").get<float>(),f.at("by").get<float>()}});
            if(dest.frames.size()<30)throw std::runtime_error("Incomplete 30 Hz Winter Scar clip");
        }
    }catch(const std::exception& e){TraceLog(LOG_WARNING,"Winter Scar animation: %s",e.what());}
}
StoryBossRenderer::~StoryBossRenderer(){for(auto& a:art_)if(a.texture.id)UnloadTexture(a.texture);if(crownBattle_.id)UnloadTexture(crownBattle_);if(frostSmooth_.id)UnloadTexture(frostSmooth_);}
bool StoryBossRenderer::Ready(StoryBossKind kind) const {
    const int index=int(kind)-1;return index>=0&&index<kStoryBossCount&&
        (kind==StoryBossKind::Crownslayer?crownBattle_.id!=0:art_[index].texture.id!=0)&&
        (kind!=StoryBossKind::FrostNova||frostSmooth_.id!=0)&&fx_.ReadyFor(kind);
}
void StoryBossRenderer::DrawCrownslayer(const StoryBoss& boss,const UiFont& font) const {
    boss.CustomCrownslayer().Draw(font,crownBattle_,&fx_);
}

void StoryBossRenderer::Draw(const StoryBoss& boss,const UiFont& font) const {
    using enum StoryBossState;
    if(!boss.Present()||boss.State()==Dormant)return;
    if(boss.TwoLives()){DrawWinter(boss,font);return;}
    if(boss.Kind()==StoryBossKind::Crownslayer){DrawCrownslayer(boss,font);return;}
    const bool w=boss.Kind()==StoryBossKind::W;const auto& art=art_[w?1:0];
    const auto feet=boss.Feet();
    fx_.DrawStory(boss,font);
    const float moveRate=w&&boss.Moving()?1.65F:1;
    int row=boss.Moving()?1:0,frame=int(boss.AnimationTime()*moveRate*20/std::max(.01F,art.durations[row]))%20;
    const auto progress=[](float time,float duration,int first,int last){return std::clamp(first+int(time/duration*(last-first+1)),first,last);};
    const auto ready=[&](float pose=.35F,float contact=.60F){return BossAnimationTiming::Frame(BossAnimationTiming::Windup(boss.StateTime(),boss.StateDuration(),pose,contact),20);};
    const auto release=[&](float contact=.60F,float duration=.55F){return BossAnimationTiming::Frame(BossAnimationTiming::Release(boss.StateTime(),contact,duration),20);};
    switch(boss.State()) {
    case Arrival:if(!w){row=4;frame=progress(boss.StateTime(),.75F,0,19);}break;
    case MeleeWindup:row=2;frame=progress(boss.StateTime(),.60F,0,8);break;
    case Strike:row=2;frame=progress(boss.StateTime(),.22F,9,15);break;
    case Disappear:row=3;frame=progress(boss.StateTime(),.40F,0,19);break;
    case Appear:row=4;frame=progress(boss.StateTime(),.38F,0,19);break;
    case GrenadeWindup:
    case AirburstWindup:row=2;frame=ready();break;
    case Airburst:row=2;frame=release();break;
    case RemoteMineWindup:row=3;frame=ready(.35F,.65F);break;
    case RemoteMine:row=3;frame=release(.65F,1.25F);break;
    case VolleyWindup:row=2;frame=ready(.35F,.46F);break;
    case BarrageWindup:row=2;frame=ready(.35F,.60F);break;
    case Barrage:row=2;frame=release();break;
    case Volley:row=2;frame=BossAnimationTiming::Frame(BossAnimationTiming::Pulse(boss.StateTime(),.24F,.46F,.60F,.42F),20);break;
    case MineWindup:row=3;frame=progress(boss.StateTime(),.70F,0,10);break;
    case EvadeWindup:row=1;frame=0;break;
    case Evade:row=1;break;
    case C4:row=3;frame=progress(boss.StateTime(),boss.StateDuration(),0,19);break;
    case Recover:
        if(w&&boss.Moving())break;
        if(boss.RecoveryFrom()==Strike){row=2;frame=progress(boss.StateTime(),.60F*boss.Difficulty().recovery,16,19);}
        else if(boss.RecoveryFrom()==GrenadeWindup){row=2;frame=release();}
        break;
    case Defeated:row=w?4:5;if(boss.StateTime()>art.durations[row])return;frame=progress(boss.StateTime(),art.durations[row],0,19);break;
    default:break;
    }
    if(!art.texture.id){font.Draw("Boss 素材缺失",feet.x-40,feet.y-100,18,RED);return;}
    const bool flip=boss.Facing()<0;
    const float anchorX=flip?256-art.anchor.x:art.anchor.x;
    const Color tint=boss.Flash()>0?Color{255,156,156,255}:boss.StunTime()>0?SKYBLUE:WHITE;
    DrawEllipse(int(feet.x),int(GameConfig::kFloorY),28,5,Fade(BLACK,.2F));
    DrawTexturePro(art.texture,{frame*256.0F,row*256.0F,flip?-256.0F:256.0F,256},
        {feet.x-anchorX/256*art.size,feet.y-art.anchor.y/256*art.size,art.size,art.size},{},0,tint);
    fx_.DrawForeground(boss);
}
void StoryBossRenderer::DrawHud(const StoryBoss& boss,const UiFont& font,const char* region) const {
    DrawRectangle(435,18,435,127,Color{10,16,22,240});
    font.Draw(region,452,30,25,TacticalUi::kPaper);
    const char* name=StoryBossName(boss.Kind());
    font.Draw(TextFormat("BOSS // %s",name),452,68,19,TacticalUi::kRed);
    const char* status=boss.Defeated()?"击败 · 已解除封锁":
        boss.State()==StoryBossState::Dormant?"向前进入战斗":
        boss.State()==StoryBossState::Revive?"倒地重生 · 战斗尚未结束":
        boss.Invulnerability()>0?"暂时无敌 · 避开危险区域":
        boss.TwoLives()?(boss.PhaseTwo()?"第二阶段":"第一阶段"):
        boss.PhaseTwo()?"半血强化":"战斗中";
    font.Draw(status,452,95,16,boss.PhaseTwo()?ORANGE:TacticalUi::kMuted);
    TacticalUi::DrawProgressLine({449,124},402,boss.Health()/boss.MaxHealth(),TacticalUi::kRed,6);
    if(boss.TwoLives()) {
        for(int i=0;i<2;++i)DrawRectangle(799+i*22,48,16,5,(!boss.PhaseTwo()||i==1)&&!boss.Defeated()?TacticalUi::kRed:TacticalUi::kMuted);
        if(boss.Invulnerability()>0)font.Draw(TextFormat("%.1fs",boss.Invulnerability()),804,95,12,SKYBLUE);
    }
}
