#include "game.h"
#include "terminal_ui.h"
#include <algorithm>
#include <cmath>

void Game::EnterDeveloperRoom() {
    enemyTrial_=false;worldPreview_=false;worldDesignPreview_=false;inBattle_=false;
    Reset();
    const auto& region=*WorldLayout::Find("approach_wtower");
    developerRoom_.Enter(region.width);
    developerSceneryRegion_="approach_wtower";
    worldScenery_.Load(region);
    developerRoomActive_=true;
    for(auto& op:operators_) {
        op.SetHorizontalBounds(80,region.width-80);
        op.PlaceAt({region.width*.45F,GameConfig::kFloorY-39},1);
    }
    cameraX_=std::clamp(player_->Position().x,640.0F,region.width-640);
    audio_.Play(AudioCue::SceneEnter);
}

void Game::RestoreDeveloperOperators() {
    const int slot=activeOperator_,facing=player_->FacingDirection();
    const float x=player_->Position().x;
    ResetOperators();
    for(auto& op:operators_) {
        op.SetHorizontalBounds(80,developerRoom_.Width()-80);
        op.PlaceAt({x,GameConfig::kFloorY-39},facing);
    }
    activeOperator_=slot;player_=&operators_[slot];bullets_.clear();
}

void Game::UpdateDeveloperRoom(float dt) {
    switch(developerRoom_.Controls(dt,cameraX_)) {
    case DeveloperCommand::Leave:
        developerRoom_.Clear();bullets_.clear();developerRoomActive_=false;
        mainMenu_.OpenHome();audio_.Play(AudioCue::UiBack);return;
    case DeveloperCommand::Spawn:
        audio_.Play(developerRoom_.SpawnSelected(player_->Position())?AudioCue::Encounter:AudioCue::Denied);
        if(developerRoom_.SelectedBoss()) {
            developerSceneryRegion_=developerRoom_.Boss().TwoLives()?StoryBossRegion(developerRoom_.Boss().Kind()):"approach_wtower";
            worldScenery_.Load(*WorldLayout::Find(developerSceneryRegion_));
        }
        break;
    case DeveloperCommand::Clear:
        developerRoom_.Clear();bullets_.clear();developerSceneryRegion_="approach_wtower";
        worldScenery_.Load(*WorldLayout::Find(developerSceneryRegion_));audio_.Play(AudioCue::UiConfirm);break;
    case DeveloperCommand::Restore:
        RestoreDeveloperOperators();audio_.Play(AudioCue::UiConfirm);break;
    default:break;
    }
    const float tick=std::clamp(dt,0.0F,.05F)*developerRoom_.Speed();
    if(player_->IsDead()&&!operators_[1-activeOperator_].IsDead())SwitchOperator(1-activeOperator_);
    if(mainMenu_.Settings().Pressed(GameAction::OperatorOne))SwitchOperator(0);
    if(mainMenu_.Settings().Pressed(GameAction::OperatorTwo))SwitchOperator(1);
    auto& mobs=developerRoom_.Mobs();auto& leader=developerRoom_.Boss();
    EnemyTarget target{{player_->Position().x+500*player_->FacingDirection(),player_->Position().y},0};
    if(mobs.Remaining()>0)target=mobs.Target(player_->Position(),player_->FacingDirection());
    if(leader.Targetable()&&(mobs.Remaining()==0||std::abs(leader.Position().x-player_->Position().x)<std::abs(target.position.x-player_->Position().x)))
        target={leader.Position(),26};
    const auto sealedFloor=leader.SealedFloor();
    if(!player_->IsDead())player_->Update(tick,target.position,target.radius,bullets_,audio_,mainMenu_.Settings(),true,false,nullptr,nullptr,&sealedFloor);
    else player_->UpdateDefeatAnimation(tick);
    const auto previousEffect=leader.CustomCrownslayer().EffectRevision();
    const bool advanced=developerRoom_.UpdateEnemies(tick,player_->Position(),player_->FacingDirection());
    if(leader.Kind()==StoryBossKind::Crownslayer&&advanced)audio_.PlayCrownEffect(leader.CustomCrownslayer(),previousEffect);
    UpdateBullets(tick);
    if(leader.Kind()==StoryBossKind::FrostNova&&player_->FellThroughFloor()) {
        const auto source=Vector2{player_->Position().x,GameConfig::kFloorY};
        const float safe=leader.SafeGroundX(player_->Position().x);
        const bool hurt=!developerRoom_.God()&&player_->TakeDamage(source,false,2);
        player_->PlaceAt({safe,GameConfig::kFloorY-39},player_->FacingDirection());
        if(hurt)audio_.PlayPlayerHit();
    }
    if(advanced&&!player_->IsDead()) {
        Vector2 source{};bool corrosive=false;EnemyImpact impact{};
        const bool mobHit=mobs.AttackHits(player_->Hitbox(),player_->ProjectileHitbox(),source,&corrosive,&impact);
        if(mobHit&&!developerRoom_.God()&&player_->TakeDamage(source,corrosive,impact.shatter&&player_->IsFrozen()?impact.frozenDamage:1)) {
            if(impact.cold)player_->ApplyCold();audio_.PlayPlayerHit();
        }
        BossImpact bossImpact{};
        if(leader.AttackHits(player_->Hitbox(),player_->ProjectileHitbox(),source,&bossImpact)&&!developerRoom_.God()&&player_->TakeDamage(source,false,bossImpact.shatter&&player_->IsFrozen()?bossImpact.damage*2:bossImpact.damage)) {
            if(bossImpact.cold)player_->ApplyCold();audio_.PlayPlayerHit();
        }
    }
    float focus=player_->Position().x;
    if(leader.TwoLives()&&leader.Active()&&std::abs(leader.Feet().x-focus)<900)focus+=(leader.Feet().x-focus)*.35F;
    cameraX_+=(std::clamp(focus,640.0F,developerRoom_.Width()-640)-cameraX_)*(1-std::exp(-6.5F*dt));
}

void Game::DrawDeveloperRoom(float scale,Vector2 offset) const {
    const Camera2D ui{offset,{},0,scale};
    const Camera2D camera{{offset.x+640*scale,offset.y+360*scale},{cameraX_+combatFeedback_.Offset().x,360+combatFeedback_.Offset().y},0,scale};
    BeginMode2D(ui);
    worldScenery_.Background(*WorldLayout::Find(developerSceneryRegion_),cameraX_,developerRoom_.Seconds(),developerRoom_.Boss().FrostWeather());
    storyBossRenderer_.DrawWeather(developerRoom_.Boss(),cameraX_);
    EndMode2D();
    BeginMode2D(camera);
    developerRoom_.DrawMarker(uiFont_,player_->Position());
    enemyRenderer_.Draw(developerRoom_.Mobs(),uiFont_);
    storyBossRenderer_.Draw(developerRoom_.Boss(),uiFont_);
    DrawBullets();player_->Draw(characterArt_);combatFeedback_.Draw(characterArt_);
    EndMode2D();
    BeginMode2D(ui);
    storyBossRenderer_.DrawAtmosphere(developerRoom_.Boss(),cameraX_,uiFont_);
    const auto name=mainMenu_.CharacterNameForSelection(mainMenu_.SelectedCharacterIndex(activeOperator_));
    player_->DrawHud(uiFont_,characterArt_,name.c_str(),&mainMenu_.Settings());
    developerRoom_.DrawControls(uiFont_);
    if(player_->IsDead()) {
        TerminalUi::Surface({447,264,797,54},Fade(TerminalUi::Ink,.93F),TerminalUi::Orange);
        TerminalUi::Fit(uiFont_,"全员失去行动能力 · F3 恢复角色",{463,276,760,30},25,TerminalUi::Paper);
    }
    EndMode2D();
}
