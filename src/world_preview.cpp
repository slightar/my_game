#include "game.h"
#include "world_layout.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Color Cyan{85,205,209,255}, Paper{232,240,243,255}, Muted{155,174,187,255};
int NearestPort(const std::vector<WorldLayout::Port>& ports,Vector2 p) {
    int best=-1;float distance=48;
    for(int i=0;i<int(ports.size());++i)if(std::abs(p.y+39-ports[i].feetY)<12&&std::abs(ports[i].x-p.x)<distance){best=i;distance=std::abs(ports[i].x-p.x);}
    return best;
}
const WorldLayout::Port* Forward(const std::vector<WorldLayout::Port>& ports) {
    for(const auto& p:ports)if(p.route->kind==WorldLayout::RouteKind::Main&&!p.returning)return &p;
    return nullptr;
}
float Ease(float t){t=std::clamp(t,0.0F,1.0F);return t*t*(3-2*t);}
// Advance by code point, never through the middle of a Chinese UTF-8 character.
std::string Revealed(const std::string& text,int count) {
    size_t end=0;
    while(end<text.size()&&count-->0) {
        ++end;
        while(end<text.size()&&(static_cast<unsigned char>(text[end])&0xC0)==0x80)++end;
    }
    return text.substr(0,end);
}
void DrawNarration(const UiFont& font,const std::string& text,float time,bool caution,bool upper,bool combat,float fade,const char* channel) {
    const float opacity=Ease(time/.22F)*fade, y=(upper?420:combat?164:104)+(1-opacity)*8;
    const float shift=combat?224.0F:0;
    const Color ink{32,42,49,255}, accent=caution?Color{191,134,57,255}:Cyan;
    const auto tint=[&](Color c){return Fade(c,opacity);};
    DrawRectangleRec({32+shift,y+5,108,108},tint(Color{0,0,0,95}));
    DrawRectangleRec({28+shift,y,108,108},tint(Color{15,22,29,245}));
    font.Skin().Draw("prts_avatar",{31+shift,y+3,102,102},tint(WHITE));
    DrawRectangleLinesEx({28+shift,y,108,108},1,tint(Color{142,161,171,255}));
    DrawRectangleRec({28+shift,y+105,108,3},tint(accent));
    DrawRectangleRec({158+shift,y+5,540,108},tint(Color{0,0,0,95}));
    DrawRectangleRec({154+shift,y,540,108},tint(Color{236,241,242,248}));
    DrawTriangle({154+shift,y+38},{140+shift,y+48},{154+shift,y+58},tint(Color{236,241,242,248}));
    DrawRectangleRec({154+shift,y,540,29},tint(Color{30,40,48,255}));
    font.Draw("PRTS",170+shift,y+5,19,tint(WHITE));
    font.Draw(channel,242+shift,y+9,11,tint(Color{159,178,186,255}));
    DrawCircle(int(679+shift),int(y+14),3,tint(accent));
    const auto visible=Revealed(text,int(std::max(0.0F,time-.15F)*30));
    float x=174+shift,row=y+44;
    // Wrap at complete glyph boundaries; leave a stable two-line bubble.
    for(size_t i=0;i<visible.size();) {
        size_t end=i+1;
        while(end<visible.size()&&(static_cast<unsigned char>(visible[end])&0xC0)==0x80)++end;
        const auto glyph=visible.substr(i,end-i);const float w=font.Measure(glyph.c_str(),20);
        if(x+w>672+shift){x=174+shift;row+=27;}
        font.Draw(glyph.c_str(),x,row,20,tint(ink));x+=w;i=end;
    }
    if(visible.size()<text.size())DrawRectangleRec({x+3,row+3,2,17},tint(accent));
}
}

void Game::ReportPrtsActions() {
    const auto& a=player_->Actions();
    const auto report=[&](int count,PrtsNarrator::Event event){for(int i=0;i<count;++i)prts_.Notify(event);};
    report(a.ranged,PrtsNarrator::Event::Ranged);report(a.melee,PrtsNarrator::Event::Melee);
    report(a.skills,PrtsNarrator::Event::Skill);report(a.dodges,PrtsNarrator::Event::Dodge);
    report(a.jumps,PrtsNarrator::Event::Jump);report(a.reloads,PrtsNarrator::Event::Reload);
}
void Game::UpdatePrts(float dt,float dx,bool hasForward,bool threat,bool combatArea) {
    const auto revision=prts_.Revision();
    prts_.Update(dt,{dx,hasForward,threat,combatArea,player_->Position().y<GameConfig::kFloorY-140,
        player_->Health()==1,player_->IsCorroded(),operators_[0].IsDead()&&operators_[1].IsDead()});
    if(prts_.Revision()!=revision)audio_.Play(AudioCue::Narration);
}
void Game::DrawPrts(bool upper,bool combat) const {
    if(prts_.Visible())DrawNarration(uiFont_,prts_.Text(),prts_.Age(),prts_.Caution(),upper,combat,prts_.Opacity(),prts_.Channel());
}

void Game::EnterWorldRegion(const std::string& id,const std::string& from) {
    if(!worldDesignPreview_&&from.empty())if(const auto* approach=WorldLayout::ApproachFor(id)) {
        EnterWorldRegion(approach->region.id);return;
    }
    const auto* region=WorldLayout::Find(id);if(!region)return;
    const bool revisit=!worldDesignPreview_&&mainMenu_.RegionVisited(id)&&!from.empty();
    const bool firstHiddenEntry=!worldDesignPreview_&&!from.empty()&&WorldLayout::Hidden(id)&&!mainMenu_.RegionVisited(id);
    if(!worldDesignPreview_ && !WorldLayout::Internal(id) && !mainMenu_.RecordRegionVisit(id,from))return;
    worldScenery_.Load(*region,worldDesignPreview_||mainMenu_.WardShortcutOpen());worldTime_=0;
    worldAttackHeld_=mainMenu_.Settings().Down(GameAction::Attack);
    worldEncounter_.Enter(worldDesignPreview_?"":id,enemies_);worldInspect_=0;worldNoteTime_=0;
    worldRegion_=id;worldPreview_=true;paused_=false;bullets_.clear();
    float x=300,feetY=GameConfig::kFloorY;int facing=1;
    for(const auto& port:WorldLayout::Ports(id,!worldDesignPreview_))if(port.target->id==from) {
        // Enter at the beginning of the route, retaining the full panorama.
        if(port.route->kind==WorldLayout::RouteKind::Main) {
            if(!port.returning){x=region->width-280;facing=-1;}
        } else {facing=port.x>region->width*.65F?-1:1;x=port.x+facing*110;feetY=port.feetY;}
        break;
    }
    x=std::clamp(x,80.0F,region->width-80);
    for(auto& op:operators_) {
        op.SetHorizontalBounds(80,region->width-80);
        op.PlaceAt({x,feetY-39},facing);
    }
    worldTraversal_.Enter(id);
    cameraX_=std::clamp(x,640.0F,region->width-640);
    const auto ports=WorldLayout::Ports(id,!worldDesignPreview_);
    const auto* forward=Forward(ports);
    const auto guidance=forward?"请沿主路继续前进。前往 "+std::string(forward->target->name)+"。":
        region->kind==WorldLayout::Kind::Ending?std::string("任务路线已完成。"):std::string("此处无可用导航。由你决定去向。");
    prts_.Enter({id,guidance,revisit,firstHiddenEntry,region->layer==5,region->kind==WorldLayout::Kind::Ending});
    if(from.empty()){worldTransition_=-1;worldDestination_.clear();}
    audio_.Play(firstHiddenEntry?AudioCue::HiddenEnter:AudioCue::SceneEnter);
}

void Game::UpdateWorldPreview(float dt) {
    dt=std::clamp(dt,0.0F,.05F);
    if(worldTransition_>=0) {
        worldTransition_+=dt;
        if(worldTransition_>=.36F&&!worldTransitionLoaded_) {
            const std::string from=worldRegion_;
            EnterWorldRegion(worldDestination_,from);
            worldTransitionLoaded_=true;
        }
        if(worldTransition_>=1.1F){worldTransition_=-1;worldDestination_.clear();}
        return;
    }
    if(IsKeyPressed(KEY_TAB)||IsKeyPressed(KEY_M)||IsKeyPressed(KEY_ESCAPE)) {
        worldPreview_=false;inBattle_=false;
        audio_.Play(AudioCue::UiBack);
        if(worldDesignPreview_)mainMenu_.OpenWorldMap(worldRegion_);else mainMenu_.OpenStageSelect();
        return;
    }
    const auto* region=WorldLayout::Find(worldRegion_);if(!region)return;
    if(mainMenu_.Settings().Pressed(GameAction::OperatorOne))SwitchOperator(0);
    if(mainMenu_.Settings().Pressed(GameAction::OperatorTwo))SwitchOperator(1);
    const bool attackHeld=mainMenu_.Settings().Down(GameAction::Attack);
    const bool attackPressed=attackHeld&&!worldAttackHeld_;
    worldAttackHeld_=attackHeld;
    const bool combat=worldEncounter_.Active();
    if(combat&&player_->IsDead()) {
        const int other=1-activeOperator_;
        if(!operators_[other].IsDead())SwitchOperator(other);
        else {
            for(auto& op:operators_)op.UpdateDefeatAnimation(dt);
            UpdatePrts(dt,0,false,true,true);
            if(IsKeyPressed(KEY_ENTER)) {
                const std::string retry=worldRegion_;Reset();EnterWorldRegion(retry);
            }
            return;
        }
    }
    if(worldTime_<.25F&&worldTime_+dt>=.25F)audio_.Play(AudioCue::Narration);
    worldTime_+=dt;worldNoteTime_=std::max(0.0F,worldNoteTime_-dt);
    const auto target=combat?enemies_.Target(player_->Position(),player_->FacingDirection()):
        EnemyTarget{{player_->Position().x+player_->FacingDirection()*500,player_->Position().y},0};
    const float previousX=player_->Position().x;
    player_->Update(dt,target.position,target.radius,bullets_,audio_,mainMenu_.Settings(),combat,false,
                    worldTraversal_.Platforms(),worldTraversal_.Ramp());
    ReportPrtsActions();
    if(combat) {
        const int previousWave=worldEncounter_.Wave();
        const bool previouslyCleared=worldEncounter_.Cleared(enemies_);
        worldEncounter_.Update(player_->Position().x,enemies_);
        enemies_.Update(dt,player_->Position());UpdateBullets(dt);
        Vector2 source{};bool corrosive=false;EnemyImpact impact;
        if(enemies_.AttackHits(player_->Hitbox(),player_->ProjectileHitbox(),source,&corrosive,&impact))
            if(player_->TakeDamage(source,corrosive,impact.shatter&&player_->IsFrozen()?impact.frozenDamage:1)) {
                if(impact.cold)player_->ApplyCold();
                audio_.PlayPlayerHit();prts_.Notify(PrtsNarrator::Event::Damage);
                if(corrosive)prts_.Notify(PrtsNarrator::Event::Corrosion);
            }
        worldEncounter_.Update(player_->Position().x,enemies_);
        if(worldEncounter_.Wave()!=previousWave){audio_.Play(AudioCue::Encounter);prts_.Notify(PrtsNarrator::Event::Wave);}
        if(!previouslyCleared&&worldEncounter_.Cleared(enemies_)){audio_.Play(AudioCue::Clear);prts_.Notify(PrtsNarrator::Event::Clear);}
        if(std::abs(player_->Position().x-region->width*.22F)<65&&
           std::abs(player_->Position().y+39-GameConfig::kFloorY)<12&&IsKeyDown(KEY_F)) {
            // A completed hold stays latched until F is released or the player
            // leaves the object, so holding F cannot repeatedly replay the cue.
            if(worldInspect_>=0) {
                worldInspect_+=dt;
                if(worldInspect_>=1.15F){worldNoteTime_=8;worldInspect_=-1;audio_.Play(AudioCue::Investigate);prts_.Notify(PrtsNarrator::Event::Investigate,region->note);}
            }
        } else worldInspect_=0;
    } else UpdateBullets(dt);
    const float x=player_->Position().x;
    const float desired=std::clamp(x,640.0F,region->width-640);
    cameraX_+=(desired-cameraX_)*(1-std::exp(-7*dt));
    const auto ports=WorldLayout::Ports(worldRegion_,!worldDesignPreview_);
    const auto* forward=Forward(ports);
    const int nearby=NearestPort(ports,player_->Position());
    UpdatePrts(dt,x-previousX,forward!=nullptr,combat&&enemies_.Remaining()>0,combat);
    const auto start=[&](const WorldLayout::Port& p){
        const bool side=p.route->kind!=WorldLayout::RouteKind::Main;
        if(!worldDesignPreview_&&worldRegion_=="clinic"&&std::string_view(p.target->id)=="ward"&&!mainMenu_.WardShortcutOpen()) {
            audio_.Play(AudioCue::Denied);prts_.Notify(PrtsNarrator::Event::Locked);return;
        }
        if(side&&worldEncounter_.InCombat(enemies_)) {
            audio_.Play(AudioCue::Denied);return;
        }
        if(!side&&combat&&!p.returning&&!worldEncounter_.Cleared(enemies_)) {
            if(IsKeyPressed(KEY_F)){audio_.Play(AudioCue::Denied);prts_.Notify(PrtsNarrator::Event::Locked);}
            return;
        }
        worldDestination_=p.target->id;worldTransition_=0;worldTransitionLoaded_=false;
    };
    // Main path continues naturally. Side passages require a fresh attack press
    // at their physical anchor; holding fire while walking past never enters one.
    if(forward&&worldTime_>.65F&&x>=forward->x&&player_->Position().y>=GameConfig::kFloorY-80){start(*forward);return;}
    if(nearby>=0) {
        const auto& p=ports[nearby];
        if(p.route->kind==WorldLayout::RouteKind::Main) {
            if(IsKeyPressed(KEY_F))start(p);
        } else if(attackPressed)start(p);
    }
}

void Game::DrawWorldPreview(float scale,Vector2 offset) const {
    const auto* region=WorldLayout::Find(worldRegion_);if(!region)return;
    const Camera2D ui{offset,{},0,scale};
    const Camera2D camera{{offset.x+640*scale,offset.y+360*scale},{cameraX_,360},0,scale};
    BeginMode2D(ui);worldScenery_.Background(*region,cameraX_,worldTime_);EndMode2D();
    BeginMode2D(camera);
    if(worldEncounter_.Active()){enemyRenderer_.DrawEntry(enemies_);enemyRenderer_.Draw(enemies_,uiFont_);}
    DrawBullets();
    player_->Draw(characterArt_);EndMode2D();
    BeginMode2D(ui);
    worldScenery_.Foreground(*region,cameraX_);
    const auto ports=WorldLayout::Ports(worldRegion_,!worldDesignPreview_);
    const auto* forward=Forward(ports);
    const int nearby=NearestPort(ports,player_->Position());
    DrawRectangleGradientV(0,0,1280,115,Color{10,16,22,235},Color{10,16,22,0});
    if(!worldEncounter_.Active()) {
        DrawRectangle(28,26,3,48,Cyan);
        uiFont_.Draw(region->name,43,24,27,Paper);
        uiFont_.Draw(WorldLayout::Layers[region->layer],44,60,14,Cyan);
    }
    uiFont_.Draw("CHERNOBOG / EXPLORATION",923,30,14,Muted);
    if(forward) {
        uiFont_.Draw((std::string(forward->target->name)+"  >").c_str(),960,62,17,Cyan);
    }
    const bool upper=WorldArt::Stairs(worldRegion_)&&player_->Position().y<380;
    DrawPrts(upper,worldEncounter_.Active());
    if(worldEncounter_.Active()&&std::abs(player_->Position().x-region->width*.22F)<65&&worldNoteTime_<=0) {
        const std::string prompt=std::string(WorldArt::Find(region->id)->clue)+" · 长按 F 调查";
        uiFont_.Draw(prompt.c_str(),60,302,18,Paper);
        if(worldInspect_>0)DrawRectangleRec({60,327,400*worldInspect_/1.15F,2},Cyan);
    }
    if(nearby>=0&&worldTransition_<0) {
        const auto& p=ports[nearby];
        const bool unknown=!worldDesignPreview_&&WorldLayout::Hidden(p.target->id)&&!mainMenu_.RegionVisited(p.target->id);
        std::string prompt;
        if(p.route->kind==WorldLayout::RouteKind::Main)prompt=p.returning?"F 原路返回":"向右继续移动";
        else if(!worldDesignPreview_&&worldRegion_=="clinic"&&std::string_view(p.target->id)=="ward"&&!mainMenu_.WardShortcutOpen())
            prompt="门从另一侧锁住了";
        else if(worldEncounter_.InCombat(enemies_))prompt="战斗中无法进入通道";
        else {
            const auto button=GameSettings::KeyName(mainMenu_.Settings().Key(GameAction::Attack));
            prompt=(unknown?std::string(WorldArt::Find(region->id)->clue):std::string(p.target->name))+" · 按 "+button+" 探索";
        }
        const float w=uiFont_.Measure(prompt.c_str(),18);
        const float promptY=upper?548:worldEncounter_.Active()?300:238;
        DrawRectangleRec({640-w/2-18,promptY,w+36,52},Fade(BLACK,.7F));
        uiFont_.Draw(prompt.c_str(),640-w/2,promptY+14,18,Paper);
    }
    DrawRectangleGradientV(0,675,1280,45,BLANK,Fade(BLACK,.9F));
    if(worldEncounter_.Active()) {
        // Reuse the combat HUD with its health, magazine and skill indicators.
        const auto name=mainMenu_.CharacterNameForSelection(mainMenu_.SelectedCharacterIndex(activeOperator_));
        player_->DrawHud(uiFont_,characterArt_,name.c_str(),&mainMenu_.Settings());
        DrawRectangle(435,18,435,127,Color{10,16,22,240});
        uiFont_.Draw(region->name,452,30,25,Paper);
        uiFont_.Draw(TextFormat("巡逻队 %d / 2 · 场上 %d · 待入场 %d",worldEncounter_.Wave(),enemies_.Remaining()-enemies_.Pending(),enemies_.Pending()),452,70,17,Cyan);
        uiFont_.Draw("1 / 2 切换角色 · F 调查 · Tab 返回",452,106,16,Muted);
        if(operators_[0].IsDead()&&operators_[1].IsDead()) {
            DrawRectangle(0,0,1280,720,Fade(BLACK,.7F));
            uiFont_.Draw("全员失去行动能力",455,325,28,Paper);
            uiFont_.Draw("按回车重试本区域 · Tab 返回",439,376,22,Cyan);
        }
    }
    const auto controls="攻击 "+GameSettings::KeyName(mainMenu_.Settings().Key(GameAction::Attack))+" / 非战斗时靠近支路按攻击键探索 · F 调查 / 返回 · Tab / M / Esc 关卡选择";
    uiFont_.Draw(controls.c_str(),35,699,14,Muted);
    if(worldTransition_>=0) {
        const float alpha=worldTransition_<.36F?Ease(worldTransition_/.36F):1-Ease((worldTransition_-.62F)/.48F);
        DrawRectangle(0,0,1280,720,Fade(Color{7,12,17,255},alpha));
        if(worldTransitionLoaded_) {
            const float w=uiFont_.Measure(region->name,28);
            uiFont_.Draw(region->name,640-w/2,330,28,Fade(Paper,alpha));
            uiFont_.Draw("CHERNOBOG / PRTS",556,379,15,Fade(Cyan,alpha));
        }
    }
    EndMode2D();
}
