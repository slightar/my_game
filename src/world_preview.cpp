#include "game.h"
#include "world_layout.h"
#include "ui_theme.h"
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
    adaptiveDifficulty_.Cancel();
    encounterDifficulty_=worldDesignPreview_?CombatDifficulty{}:adaptiveDifficulty_.Profile();
    encounterDifficultyLabel_=adaptiveDifficulty_.Label();
    worldScenery_.Load(*region,worldDesignPreview_||mainMenu_.WardShortcutOpen());worldTime_=0;
    worldAttackHeld_=mainMenu_.Settings().Down(GameAction::Attack);
    worldEncounter_.Enter(worldDesignPreview_?"":id,enemies_,encounterDifficulty_);worldInspect_=0;worldNoteTime_=0;
    storyBoss_.Enter(worldDesignPreview_?"":id,region->width,encounterDifficulty_);
    worldRegion_=id;worldPreview_=true;paused_=false;bullets_.clear();combatFeedback_.Reset();
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
    const auto guidance=storyBoss_.Present()&&!storyBoss_.Defeated()?
        std::string("前方检测到领袖。击败")+StoryBossName(storyBoss_.Kind())+"后继续前进。":
        forward?"请沿主路继续前进。前往 "+std::string(forward->target->name)+"。":
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
        adaptiveDifficulty_.Cancel();
        worldPreview_=false;inBattle_=false;
        audio_.Play(AudioCue::UiBack);
        if(worldDesignPreview_)mainMenu_.OpenWorldMap(worldRegion_);else mainMenu_.OpenStageSelect();
        return;
    }
    const auto* region=WorldLayout::Find(worldRegion_);if(!region)return;
    const auto teamHealth=[&](){return operators_[0].Health()+operators_[1].Health();};
    const auto knockedOut=[&](){return int(operators_[0].IsDead())+int(operators_[1].IsDead());};
    const int healthBefore=teamHealth(),knockoutsBefore=knockedOut();
    if(mainMenu_.Settings().Pressed(GameAction::OperatorOne))SwitchOperator(0);
    if(mainMenu_.Settings().Pressed(GameAction::OperatorTwo))SwitchOperator(1);
    const bool attackHeld=mainMenu_.Settings().Down(GameAction::Attack);
    const bool attackPressed=attackHeld&&!worldAttackHeld_;
    worldAttackHeld_=attackHeld;
    const bool patrol=worldEncounter_.Active();
    const bool combat=patrol||storyBoss_.Present();
    if(combat&&player_->IsDead()) {
        const int other=1-activeOperator_;
        if(!operators_[other].IsDead())SwitchOperator(other);
        else {
            adaptiveDifficulty_.Finish(false);
            for(auto& op:operators_)op.UpdateDefeatAnimation(dt);
            UpdatePrts(dt,0,false,true,true);
            if(IsKeyPressed(KEY_ENTER)) {
                const std::string retry=worldRegion_;Reset();EnterWorldRegion(retry,retry);
            }
            return;
        }
    }
    if(worldTime_<.25F&&worldTime_+dt>=.25F)audio_.Play(AudioCue::Narration);
    worldTime_+=dt;worldNoteTime_=std::max(0.0F,worldNoteTime_-dt);
    const auto target=storyBoss_.Active()?EnemyTarget{storyBoss_.Position(),26}:patrol?enemies_.Target(player_->Position(),player_->FacingDirection()):
        EnemyTarget{{player_->Position().x+player_->FacingDirection()*500,player_->Position().y},0};
    const float previousX=player_->Position().x;
    const auto sealedFloor=storyBoss_.SealedFloor();
    player_->Update(dt,target.position,target.radius,bullets_,audio_,mainMenu_.Settings(),combat,false,
                    worldTraversal_.Platforms(),worldTraversal_.Ramp(),&sealedFloor);
    ReportPrtsActions();
    bool encounterCleared=false;
    if(patrol) {
        const int previousWave=worldEncounter_.Wave();
        const bool previouslyCleared=worldEncounter_.Cleared(enemies_);
        worldEncounter_.Update(player_->Position().x,enemies_);
        if(previousWave==0&&worldEncounter_.Wave()==1) {
            float workload=0;
            for(int wave=0;wave<2;++wave)for(auto kind:WorldEncounter::Formation(worldRegion_,wave,encounterDifficulty_))
                workload+=EnemyData(kind).health;
            adaptiveDifficulty_.Begin(workload*encounterDifficulty_.health/3.0F,teamHealth());
        }
        adaptiveDifficulty_.Tick(dt,!enemies_.Cleared());
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
        if(!previouslyCleared&&worldEncounter_.Cleared(enemies_)){audio_.Play(AudioCue::Clear);prts_.Notify(PrtsNarrator::Event::Clear);encounterCleared=true;}
        if(std::abs(player_->Position().x-region->width*.22F)<65&&
           std::abs(player_->Position().y+39-GameConfig::kFloorY)<12&&IsKeyDown(KEY_F)) {
            // A completed hold stays latched until F is released or the player
            // leaves the object, so holding F cannot repeatedly replay the cue.
            if(worldInspect_>=0) {
                worldInspect_+=dt;
                if(worldInspect_>=1.15F){worldNoteTime_=8;worldInspect_=-1;audio_.Play(AudioCue::Investigate);prts_.Notify(PrtsNarrator::Event::Investigate,region->note);}
            }
        } else worldInspect_=0;
    } else if(storyBoss_.Present()) {
        if(storyBoss_.TryStart(player_->Position().x)) {
            adaptiveDifficulty_.Begin(storyBoss_.MaxHealth()*(storyBoss_.TwoLives()?2:1)/3.0F,teamHealth());
            for(auto& op:operators_)op.SetHorizontalBounds(storyBoss_.Left(),storyBoss_.Right());
            audio_.Play(AudioCue::Encounter);
            prts_.Notify(PrtsNarrator::Event::Wave,storyBoss_.Briefing());
        }
        const bool defeated=storyBoss_.Defeated();
        adaptiveDifficulty_.Tick(dt,storyBoss_.Active());
        const auto previousEffect=storyBoss_.CustomCrownslayer().EffectRevision();
        storyBoss_.Update(dt,player_->Position(),player_->FacingDirection());UpdateBullets(dt);
        if(storyBoss_.Kind()==StoryBossKind::Crownslayer)audio_.PlayCrownEffect(storyBoss_.CustomCrownslayer(),previousEffect);
        if(storyBoss_.Kind()==StoryBossKind::FrostNova&&player_->FellThroughFloor()) {
            const auto source=Vector2{player_->Position().x,GameConfig::kFloorY};
            const float safe=storyBoss_.SafeGroundX(player_->Position().x);
            const bool hurt=player_->TakeDamage(source,false,2);
            player_->PlaceAt({safe,GameConfig::kFloorY-39},player_->FacingDirection());
            if(hurt){audio_.PlayPlayerHit();prts_.Notify(PrtsNarrator::Event::Damage);}
        }
        Vector2 source{};BossImpact impact{};
        if(storyBoss_.AttackHits(player_->Hitbox(),player_->ProjectileHitbox(),source,&impact)&&player_->TakeDamage(source,false,impact.shatter&&player_->IsFrozen()?impact.damage*2:impact.damage)) {
            if(impact.cold)player_->ApplyCold();
            audio_.PlayPlayerHit();prts_.Notify(PrtsNarrator::Event::Damage);
        }
        if(!defeated&&storyBoss_.Defeated()) {
            encounterCleared=true;
            for(auto& op:operators_)op.SetHorizontalBounds(80,region->width-80);
            audio_.Play(AudioCue::Clear);prts_.Notify(PrtsNarrator::Event::Clear);
        }
    } else UpdateBullets(dt);
    adaptiveDifficulty_.ObserveDamage(healthBefore-teamHealth(),knockedOut()-knockoutsBefore);
    if(knockedOut()==2)adaptiveDifficulty_.Finish(false);
    else if(encounterCleared)adaptiveDifficulty_.Finish(true);
    const float x=player_->Position().x;
    float focus=x;
    if(storyBoss_.TwoLives()&&storyBoss_.Active()&&std::abs(storyBoss_.Feet().x-x)<900)focus+=(storyBoss_.Feet().x-x)*.35F;
    const float desired=std::clamp(focus,640.0F,region->width-640);
    cameraX_+=(desired-cameraX_)*(1-std::exp(-7*dt));
    const auto ports=WorldLayout::Ports(worldRegion_,!worldDesignPreview_);
    const auto* forward=Forward(ports);
    const int nearby=NearestPort(ports,player_->Position());
    UpdatePrts(dt,x-previousX,forward!=nullptr,(patrol&&enemies_.Remaining()>0)||storyBoss_.Active(),combat);
    const auto start=[&](const WorldLayout::Port& p){
        const bool side=p.route->kind!=WorldLayout::RouteKind::Main;
        if(storyBoss_.Active()||(storyBoss_.Present()&&!storyBoss_.Defeated()&&!p.returning&&p.route->kind==WorldLayout::RouteKind::Main)) {
            if(IsKeyPressed(KEY_F)||attackPressed)audio_.Play(AudioCue::Denied);
            return;
        }
        if(!worldDesignPreview_&&worldRegion_=="clinic"&&std::string_view(p.target->id)=="ward"&&!mainMenu_.WardShortcutOpen()) {
            audio_.Play(AudioCue::Denied);prts_.Notify(PrtsNarrator::Event::Locked);return;
        }
        if(side&&worldEncounter_.InCombat(enemies_)) {
            audio_.Play(AudioCue::Denied);return;
        }
        if(!side&&patrol&&!p.returning&&!worldEncounter_.Cleared(enemies_)) {
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
    const Camera2D camera{{offset.x+640*scale,offset.y+360*scale},{cameraX_+combatFeedback_.Offset().x,360+combatFeedback_.Offset().y},0,scale};
    BeginMode2D(ui);
    worldScenery_.Background(*region,cameraX_,worldTime_,storyBoss_.FrostWeather());
    storyBossRenderer_.DrawWeather(storyBoss_,cameraX_);
    EndMode2D();
    BeginMode2D(camera);
    if(worldEncounter_.Active()){enemyRenderer_.DrawEntry(enemies_);enemyRenderer_.Draw(enemies_,uiFont_);}
    storyBossRenderer_.Draw(storyBoss_,uiFont_);
    DrawBullets();
    player_->Draw(characterArt_);combatFeedback_.Draw(characterArt_);EndMode2D();
    BeginMode2D(ui);
    worldScenery_.Foreground(*region,cameraX_);
    storyBossRenderer_.DrawAtmosphere(storyBoss_,cameraX_,uiFont_);
    const auto ports=WorldLayout::Ports(worldRegion_,!worldDesignPreview_);
    const auto* forward=Forward(ports);
    const int nearby=NearestPort(ports,player_->Position());
    DrawRectangleGradientV(0,0,1280,115,Color{10,16,22,235},Color{10,16,22,0});
    const bool combat=worldEncounter_.Active()||storyBoss_.Present();
    if(!combat) {
        DrawRectangle(28,26,3,48,Cyan);
        uiFont_.Draw(region->name,43,24,27,Paper);
        uiFont_.Draw(WorldLayout::Layers[region->layer],44,60,14,Cyan);
    }
    const bool bossBlocked=storyBoss_.Present()&&!storyBoss_.Defeated();
    uiFont_.Draw(bossBlocked?"CHERNOBOG / BOSS ENCOUNTER":"CHERNOBOG / EXPLORATION",923,30,14,Muted);
    if(bossBlocked) {
        uiFont_.Draw("击败领袖后解除封锁",960,62,17,TacticalUi::kRed);
    } else if(forward) {
        uiFont_.Draw((std::string(forward->target->name)+"  >").c_str(),960,62,17,Cyan);
    }
    if(combat)uiFont_.Draw(("自适应 / "+encounterDifficultyLabel_).c_str(),960,90,14,Muted);
    const bool upper=WorldArt::Stairs(worldRegion_)&&player_->Position().y<380;
    DrawPrts(upper,combat);
    if(worldEncounter_.Active()&&std::abs(player_->Position().x-region->width*.22F)<65&&worldNoteTime_<=0) {
        const std::string prompt=std::string(WorldArt::Find(region->id)->clue)+" · 长按 F 调查";
        uiFont_.Draw(prompt.c_str(),60,302,18,Paper);
        if(worldInspect_>0)DrawRectangleRec({60,327,400*worldInspect_/1.15F,2},Cyan);
    }
    if(nearby>=0&&worldTransition_<0) {
        const auto& p=ports[nearby];
        const bool unknown=!worldDesignPreview_&&WorldLayout::Hidden(p.target->id)&&!mainMenu_.RegionVisited(p.target->id);
        std::string prompt;
        if(storyBoss_.Active()||(storyBoss_.Present()&&!storyBoss_.Defeated()&&!p.returning&&p.route->kind==WorldLayout::RouteKind::Main))prompt="击败领袖后解除封锁";
        else if(p.route->kind==WorldLayout::RouteKind::Main)prompt=p.returning?"F 原路返回":"向右继续移动";
        else if(!worldDesignPreview_&&worldRegion_=="clinic"&&std::string_view(p.target->id)=="ward"&&!mainMenu_.WardShortcutOpen())
            prompt="门从另一侧锁住了";
        else if(worldEncounter_.InCombat(enemies_))prompt="战斗中无法进入通道";
        else {
            const auto button=GameSettings::KeyName(mainMenu_.Settings().Key(GameAction::Attack));
            prompt=(unknown?std::string(WorldArt::Find(region->id)->clue):std::string(p.target->name))+" · 按 "+button+" 探索";
        }
        const float w=uiFont_.Measure(prompt.c_str(),18);
        const float promptY=upper?548:combat?300:238;
        DrawRectangleRec({640-w/2-18,promptY,w+36,52},Fade(BLACK,.7F));
        uiFont_.Draw(prompt.c_str(),640-w/2,promptY+14,18,Paper);
    }
    DrawRectangleGradientV(0,675,1280,45,BLANK,Fade(BLACK,.9F));
    if(combat) {
        // Reuse the combat HUD with its health, magazine and skill indicators.
        const auto name=mainMenu_.CharacterNameForSelection(mainMenu_.SelectedCharacterIndex(activeOperator_));
        player_->DrawHud(uiFont_,characterArt_,name.c_str(),&mainMenu_.Settings());
        if(storyBoss_.Present())storyBossRenderer_.DrawHud(storyBoss_,uiFont_,region->name);
        else {
        DrawRectangle(435,18,435,127,Color{10,16,22,240});
        uiFont_.Draw(region->name,452,30,25,Paper);
        uiFont_.Draw(TextFormat("巡逻队 %d / 2 · 场上 %d · 待入场 %d",worldEncounter_.Wave(),enemies_.Remaining()-enemies_.Pending(),enemies_.Pending()),452,70,17,Cyan);
        uiFont_.Draw("1 / 2 切换角色 · F 调查 · Tab 返回",452,106,16,Muted);
        }
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
