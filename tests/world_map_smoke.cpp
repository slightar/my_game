#include "game.h"
#include "world_layout.h"
#include "file_path.h"
#include "world_depth.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <functional>
#include <set>
#include <cstdint>
#include <fstream>
struct BossAudioTestAccess {
    static std::size_t Count(const BossAudio& audio){return audio.music_.size();}
    static float Level(const BossAudio& audio,const char* track){
        for(const auto& layer:audio.music_)if(layer.track==track)return layer.level;
        return 0;
    }
    static const void* Buffer(const BossAudio& audio,const char* track){
        for(const auto& layer:audio.music_)if(layer.track==track)return layer.stream.stream.buffer;
        return nullptr;
    }
    static bool Paused(const BossAudio& audio){
        return std::all_of(audio.music_.begin(),audio.music_.end(),[](const auto& layer){
            return layer.paused&&!IsMusicStreamPlaying(layer.stream);
        });
    }
};
struct AudioSystemTestAccess {
    static double Last(const AudioSystem& audio,AudioCue cue){return audio.clips_[static_cast<int>(cue)].lastPlayed;}
    static void Age(AudioSystem& audio){for(auto& clip:audio.clips_)clip.lastPlayed=-100;}
    static bool Playing(const AudioSystem& audio){for(const auto& clip:audio.clips_)for(int i=0;i<clip.voiceCount;++i)if(IsSoundPlaying(clip.voices[i]))return true;return false;}
};
struct WorldPreviewTestAccess {
    // Keep menu reapplication of settings silent as well as direct playback.
    // This is an in-memory fixture setting; it never touches the user's preferences.
    static void Mute(Game& game){const_cast<GameSettings&>(game.mainMenu_.Settings()).AdjustMaster(-100);SetMasterVolume(0);}
    static DeveloperRoom& Lab(Game& game){return game.developerRoom_;}
    static bool LabActive(Game& game){return game.developerRoomActive_;}
    static void Home(Game& game){game.mainMenu_.OpenHome();}
    static MainMenu& Menu(Game& game){return game.mainMenu_;}
    static bool LabGlyphs(Game& game){return game.developerRoom_.GlyphsSupported(game.uiFont_);}
    static bool VarietyGlyphs(Game& game){return game.uiFont_.SupportsText("后撤 · 扇形飞刀连环爆破 · 寻找空隙安全空隙低空榴弹 · 跳跃避开扇形冰刃 · 变换站位落矛阵 · 离开标记冰雨顺序会变化");}
    static bool ShadowGlyphs(Game& game){return game.uiFont_.SupportsText("跳跃避开向右避开向左避开离开路线离开红圈←→");}
    static bool CrownEffects(Game& game){return game.bossRenderer_.EffectsReady()&&game.storyBossRenderer_.Ready(StoryBossKind::Crownslayer);}
    static void ObserveDemo(Game& game){for(auto& op:game.operators_)op.Reset(OperatorKind::Exusiai);game.activeOperator_=0;game.player_=&game.operators_[0];game.player_->PlaceAt({1800,605},1);}
    static bool LabArt(Game& game){for(int i=0;i<int(kEnemyKindCount);++i)if(!game.enemyRenderer_.HasSprite(static_cast<EnemyKind>(i)))return false;for(int i=1;i<=kStoryBossCount;++i)if(!game.storyBossRenderer_.Ready(static_cast<StoryBossKind>(i)))return false;return game.storyBossRenderer_.EffectsReady();}
    static void LabShot(Game& game,Vector2 position){Bullet b;b.position=position;b.radius=35;b.damage=10000;b.lifetime=1;game.bullets_.push_back(b);}
    static void SealShot(Game& game,float x){Bullet b;b.position={x,GameConfig::kFloorY-46};b.radius=8;b.damage=1;b.lifetime=1;game.bullets_.push_back(b);}
    static bool WinterAudio(Game& game){return game.bossAudio_.Ready()&&game.bossAudio_.MusicReady();}
    static const std::string& WinterTrack(Game& game){return game.bossAudio_.CurrentTrack();}
    static bool WinterGlyphs(Game& game){return game.uiFont_.SupportsText(StoryBoss::Glyphs()+std::string("霜星 · 生命解放投枪 · 正在瞄准倒地重生 · 第二条生命尚未解除封锁"));}
    static bool Cold(Game& game){return game.player_->IsCold();}
    static bool Frozen(Game& game){return game.player_->IsFrozen();}
    static int Health(Game& game){return game.player_->Health();}
    static float Camera(Game& game){return game.cameraX_;}
    static void FocusBoss(Game& game){game.cameraX_=std::clamp(game.storyBoss_.Position().x,640.0F,WorldLayout::Find(game.worldRegion_)->width-640);}
    static void RestoreCamera(Game& game,float x){game.cameraX_=x;}
    static bool DemoActive(const Game& game){return game.inBattle_&&game.bossActive_&&!game.enemyTrial_;}
    static const Boss& DemoBoss(const Game& game){return game.boss_;}
    static AudioSystem& Audio(Game& game){return game.audio_;}
    static void Switch(Game& game,int slot){game.SwitchOperator(slot);}
    static void Enter(Game& game,const std::string& id,const std::string& from){game.EnterWorldRegion(id,from);}
    static const PrtsNarrator& Narrator(const Game& game){return game.prts_;}
    static bool NarrationGlyphs(const Game& game){return game.uiFont_.SupportsText(PrtsNarrator::GlyphText());}
    static void TestThreat(Game& game){game.enemies_.Spawn(EnemyKind::Shield,game.player_->Position().x+950);}
    static void CompleteBattleFixture(Game& game){
        // Advance real waves and resolve lethal test projectiles through the
        // game's collision/update path, so the production clear event fires.
        game.player_->PlaceAt({WorldLayout::Find(game.worldRegion_)->width*.64F,605},1);
        for(int frame=0;frame<2000&&!game.worldEncounter_.Cleared(game.enemies_);++frame) {
            for(const auto& unit:game.enemies_.Units())if(unit.Targetable()) {
                Bullet shot;shot.position=unit.Center();shot.lifetime=1;shot.radius=35;shot.damage=10000;
                game.bullets_.push_back(shot);
            }
            game.Update(1.0F/60);
        }
        if(!game.worldEncounter_.Cleared(game.enemies_))throw std::runtime_error("Battle fixture did not clear actual waves");
    }
    static bool Injure(Game& game,bool corrosion=false) {
        if(!game.player_->TakeDamage({game.player_->Position().x+30,605},corrosion))return false;
        game.prts_.Notify(PrtsNarrator::Event::Damage);
        if(corrosion)game.prts_.Notify(PrtsNarrator::Event::Corrosion);
        return true;
    }
    static void Place(Game& game,Vector2 point){game.player_->PlaceAt(point,1);}
    static bool ArtReady(Game& game){return game.worldScenery_.Load(*WorldLayout::Find(game.worldRegion_));}
    static bool DepthReady(Game& game){return game.worldScenery_.DepthReady();}
    static bool ColdReady(Game& game){return game.worldScenery_.ColdReady();}
    static void DepthCamera(Game& game,float camera){game.cameraX_=camera;game.player_->PlaceAt({camera,605},1);}
    static void Action(Game& game,const std::string& id){game.worldDesignPreview_=false;game.Reset();game.EnterWorldRegion(id);}
    static void AuditScene(Game& game,const std::string& id){game.Reset();game.worldDesignPreview_=true;game.EnterWorldRegion(id);}
    static int Pending(Game& game){return game.enemies_.Pending();}
    static int Alive(Game& game){return game.enemies_.Remaining()-game.enemies_.Pending();}
    static bool Cleared(Game& game){return game.worldEncounter_.Cleared(game.enemies_);}
    static bool InCombat(Game& game){return game.worldEncounter_.InCombat(game.enemies_);}
    static bool Transition(Game& game){return game.worldTransition_>=0;}
    static const std::vector<Bullet>& Shots(const Game& game){return game.bullets_;}
    static const PlayerActions& Actions(const Game& game){return game.player_->Actions();}
    static void BindAttack(Game& game,int key){if(!const_cast<GameSettings&>(game.mainMenu_.Settings()).Bind(GameAction::Attack,key))throw std::runtime_error("Attack rebind rejected");}
    static bool Note(Game& game){return game.worldNoteTime_>0;}
    static const StoryBoss& BossFight(const Game& game){return game.storyBoss_;}
    static int FirstOperatorHealth(const Game& game){return game.operators_[0].Health();}
    static const AdaptiveDifficulty& Adaptive(const Game& game){return game.adaptiveDifficulty_;}
    static const EnemySystem& Patrol(const Game& game){return game.enemies_;}
    static const CombatDifficulty& Difficulty(const Game& game){return game.encounterDifficulty_;}
    static bool AdaptiveGlyphs(const Game& game){return game.uiFont_.SupportsText("自适应缓和标准进阶高压");}
    static bool BossArt(const Game& game){return game.storyBossRenderer_.Ready(game.storyBoss_.Kind());}
    static void BossShot(Game& game,float damage) {
        Bullet bullet;bullet.position=game.storyBoss_.Position();bullet.radius=35;bullet.damage=damage;bullet.lifetime=1;
        game.bullets_.push_back(bullet);
    }
    static void DefeatOperators(Game& game) {
        for(auto& op:game.operators_){const auto position=op.Position();op.Reset(op.Kind());op.PlaceAt(position,1);op.TakeDamage(position,false,100);}
    }
    static void ObserveW(Game& game,float distance) {
        // Observation fixture keeps a spectator alive while collecting all
        // attacks. Damage/collision is checked by the encounter/unit audits.
        for(auto& op:game.operators_)if(op.IsDead())op.Reset(op.Kind());
        game.activeOperator_=0;game.player_=&game.operators_[0];
        game.player_->Reset(OperatorKind::Exusiai);
        game.player_->PlaceAt({std::clamp(game.storyBoss_.Feet().x-distance,game.storyBoss_.Left()+45,game.storyBoss_.Right()-45),605},1);
    }
    static void CompleteStoryBattle(Game& game) {
        if(!game.storyBoss_.Present()||game.storyBoss_.Defeated())return;
        game.player_->PlaceAt({game.storyBoss_.TriggerX()+70,605},1);
        for(int i=0;i<1200&&!game.storyBoss_.Defeated();++i) {
            if(game.storyBoss_.TwoLives())ObserveW(game,500);
            if(game.storyBoss_.Targetable())BossShot(game,10000);
            game.Update(1.0F/60);
        }
        if(!game.storyBoss_.Defeated())throw std::runtime_error("Boss fixture did not clear via actual game collision");
    }
    static void TickSafe(Game& game){game.Update(1.0F/60);ClearPatrol(game);}
    static bool Design(const Game& game){return game.worldDesignPreview_;}
    static void ClearPatrol(Game& game){
        for(const auto& u:game.enemies_.Units())if(u.Targetable())
            game.enemies_.Damage(u.id,10000,DamageType::Arts,{u.feet.x-100,u.feet.y});
    }
    static void Open(Game& game) { game.mainMenu_.OpenWorldMap("clinic"); }
};
int main(int argc,char** argv){
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1280,720,"City traversal smoke");InitAudioDevice();
    int result=0;
    try {
        ProgressSystem fixture(Utf8Path(GetApplicationDirectory()));
        if(!fixture.ResetProgress())throw std::runtime_error("Fixture reset failed");
        Game game;
        WorldPreviewTestAccess::Mute(game);
        const auto key=[](int k,bool down){PlayAutomationEvent({0,down?2U:1U,{k,0,0,0}});};
        const auto press=[&](int k){key(k,true);game.Update(1.0F/60);key(k,false);game.Update(1.0F/60);};
        const auto check=[](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);};
        const auto output=Utf8Path(GetApplicationDirectory())/"world-qa";std::filesystem::create_directories(output);
        MainMenu map(output/"menu-fixture");map.OpenWorldMap();
        const auto mouse=[](unsigned type,int a,int b=0){PlayAutomationEvent({0,type,{a,b,0,0}});};
        const auto click=[&](int x,int y){mouse(7,x,y);mouse(6,MOUSE_BUTTON_LEFT);map.Update();mouse(5,MOUSE_BUTTON_LEFT);return map.Update();};
        click(600,227);check(map.SelectedWorldRegion()=="bridge","Node click failed");
        mouse(7,600,227);mouse(6,MOUSE_BUTTON_LEFT);map.Update();mouse(7,200,227);map.Update();
        mouse(5,MOUSE_BUTTON_LEFT);check(map.Update()==MenuAction::None&&map.SelectedWorldRegion()=="bridge","Drag activated node");
        click(840,166);check(map.SelectedWorldRegion()=="prts","System navigation failed");
        check(click(1090,594)==MenuAction::StartWorldPreview,"Room entry button failed");
        const auto capture=[&](const std::string& name){auto t=LoadRenderTexture(1280,720);BeginTextureMode(t);game.Draw(1,{});EndTextureMode();
            auto im=LoadImageFromTexture(t.texture);ImageFlipVertical(&im);check(ExportImage(im,(output/(name+".png")).string().c_str()),"Screenshot failed");UnloadImage(im);UnloadRenderTexture(t);};
        if(argc>1&&std::string_view(argv[1])=="--w-mine-audit") {
            WorldPreviewTestAccess::Enter(game,"wtower","approach_wtower");
            const auto& w=WorldPreviewTestAccess::BossFight(game);
            check(WorldPreviewTestAccess::BossArt(game),"Complete W mine sprite missing");
            WorldPreviewTestAccess::Place(game,{w.TriggerX()+10,605});game.Update(1.0F/60);
            for(int i=0;i<2400&&w.Mines().empty();++i) {
                WorldPreviewTestAccess::ObserveW(game,420);game.Update(1.0F/60);
            }
            check(!w.Mines().empty(),"W mine fixture never spawned");
            const float x=w.Mines().front().position.x;
            const auto shot=[&](const char* name){
                const float saved=WorldPreviewTestAccess::Camera(game);
                WorldPreviewTestAccess::RestoreCamera(game,std::clamp(x,640.0F,WorldLayout::Find("wtower")->width-640));
                capture(name);WorldPreviewTestAccess::RestoreCamera(game,saved);
            };
            shot("w-mine-fixed-deployed");
            for(int i=0;i<60;++i){WorldPreviewTestAccess::Place(game,{x-200,605});game.Update(1.0F/60);}
            check(!w.Mines().empty()&&w.Mines().front().age>=.8F&&w.Mines().front().fuse<0,"Mine failed to arm");
            shot("w-mine-fixed-armed");
            WorldPreviewTestAccess::Place(game,{x,605});game.Update(1.0F/60);
            check(w.Mines().front().fuse>0,"Mine lost proximity fuse");
            shot("w-mine-fixed-fuse");
            for(int i=0;i<55;++i)game.Update(1.0F/60);
            check(std::none_of(w.Mines().begin(),w.Mines().end(),[&](const auto& mine){return std::abs(mine.position.x-x)<1;}),
                "Detonated mine sprite persisted");
            shot("w-mine-fixed-detonated");
            std::cout<<"W complete mine sprite deployment, foot anchor, armed/fuse display and detonation cleanup audit passed\n";
            return 0;
        } else if(argc>1&&std::string_view(argv[1])=="--w-fx-audit") {
            WorldPreviewTestAccess::Enter(game,"wtower","approach_wtower");
            auto& w=WorldPreviewTestAccess::BossFight(game);
            WorldPreviewTestAccess::Place(game,{w.TriggerX()+10,605});game.Update(1.0F/60);
            check(WorldPreviewTestAccess::BossArt(game),"W client effect assets incomplete");
            std::set<std::string> seen;
            bool phaseTwoC4Armed=false;
            const auto shot=[&](const std::string& name,float x){
                if(!seen.insert(name).second)return;
                const float saved=WorldPreviewTestAccess::Camera(game);
                WorldPreviewTestAccess::RestoreCamera(game,std::clamp(x,640.0F,WorldLayout::Find("wtower")->width-640));
                capture(name);WorldPreviewTestAccess::RestoreCamera(game,saved);
            };
            const auto inspect=[&]{
                for(const auto& g:w.Grenades())if(g.age>=.25F&&g.age<.30F)shot("w-fx-flight",g.position.x);
                for(const auto& v:w.Visuals()) {
                    if(v.kind==BossFx::WMuzzle&&v.age>=.05F&&v.age<.08F)shot("w-fx-muzzle",v.position.x);
                    const bool c4=v.kind==BossFx::WC4Explosion;
                    if(!c4&&v.kind!=BossFx::WExplosion)continue;
                    if(c4&&!phaseTwoC4Armed)continue;
                    for(const float age:{.05F,.18F,.50F,.95F,1.4F,3.0F}) {
                        if(!c4&&age>1.4F)continue;
                        if(v.age>=age&&v.age<age+.02F)
                            shot(std::string(c4?"w-fx-c4-":"w-fx-grenade-")+std::to_string(int(age*100)),v.position.x);
                    }
                }
            };
            for(int i=0;i<660;++i){WorldPreviewTestAccess::ObserveW(game,420);game.Update(1.0F/60);inspect();}
            WorldPreviewTestAccess::BossShot(game,w.MaxHealth()*.55F);game.Update(1.0F/60);
            check(w.PhaseTwo(),"W effect fixture did not enter phase two");
            std::erase_if(seen,[](const auto& name){return name.starts_with("w-fx-c4-");});
            for(int i=0;i<2400&&!(seen.contains("w-fx-c4-300")&&seen.contains("w-fx-c4-warning"));++i) {
                WorldPreviewTestAccess::ObserveW(game,310);game.Update(1.0F/60);inspect();
                if(std::count_if(w.Blasts().begin(),w.Blasts().end(),[](const auto& b){return b.c4&&b.timer>1;})==3)
                    phaseTwoC4Armed=true;
                for(const auto& blast:w.Blasts())if(blast.c4&&blast.timer>.2F&&blast.timer<.5F)
                    shot("w-fx-c4-warning",blast.position.x);
            }
            for(const char* name:{"w-fx-flight","w-fx-muzzle","w-fx-grenade-18","w-fx-grenade-95",
                "w-fx-c4-warning","w-fx-c4-18","w-fx-c4-50","w-fx-c4-300"})
                check(seen.contains(name),name);
            std::cout<<"W original muzzle/trail assets, flight, grenade fire/smoke, C4 warning/flash/embers render audit passed\n";
            return 0;
        } else if(argc>1&&std::string_view(argv[1])=="--frost-weather-audit") {
            WorldPreviewTestAccess::Enter(game,"ice","approach_ice");
            const auto& b=WorldPreviewTestAccess::BossFight(game);
            const float camera=std::clamp(b.TriggerX()+180,640.0F,WorldLayout::Find("ice")->width-640);
            const auto advance=[&](int frames){for(int i=0;i<frames;++i){WorldPreviewTestAccess::ObserveW(game,500);game.Update(1.0F/60);}};
            const auto shot=[&](const char* name){WorldPreviewTestAccess::RestoreCamera(game,camera);capture(name);};
            check(WorldPreviewTestAccess::ColdReady(game),"Cold background shader unavailable");
            WorldPreviewTestAccess::Place(game,{b.TriggerX()-140,605});
            for(int i=0;i<120;++i)game.Update(1.0F/60);
            check(!b.Active()&&b.FrostWeather()==0,"Street became cold before meeting FrostNova");
            shot("frost-weather-before");
            WorldPreviewTestAccess::Place(game,{b.TriggerX()+20,605});game.Update(1.0F/60);
            advance(60);shot("frost-weather-arrival");
            check(b.FrostWeather()>.2F&&b.FrostWeather()<.3F,"Arrival weather jumped to full strength");
            advance(130);shot("frost-weather-phase1");
            WorldPreviewTestAccess::BossShot(game,10000);advance(1);
            check(b.State()==StoryBossState::Revive,"Weather revival fixture failed");
            advance(440);shot("frost-weather-phase2");
            check(b.PhaseTwo()&&b.FrostWeather()>.99F,"Second-phase snow did not strengthen");
            WorldPreviewTestAccess::BossShot(game,10000);advance(1);
            check(b.Defeated()&&b.FrostWeather()>.9F,"Weather vanished at defeat");
            advance(400);shot("frost-weather-cleared");
            check(b.FrostWeather()==0,"Street never recovered after defeat");
            WorldPreviewTestAccess::Enter(game,"industry","");
            check(WorldPreviewTestAccess::BossFight(game).FrostWeather()==0,"Weather leaked to another room");
            std::cout<<"Frost weather: normal street, gradual arrival, layered snow, second phase, defeat fade and region reset passed\n";
            return 0;
        }
        if(argc>1&&std::string_view(argv[1])=="--music-fade-audit") {
            BossAudio audio;
            const auto sync=[&](const char* track,int frames,bool paused=false,float volume=1){
                for(int i=0;i<frames;++i)audio.Sync(nullptr,paused,volume,track,1.0F/60);
            };
            sync("home",60);const void* original=BossAudioTestAccess::Buffer(audio,"home");
            check(audio.MusicReady()&&BossAudioTestAccess::Level(audio,"home")>.99F,"Initial music fade-in failed");
            audio.Sync(nullptr,false,1,"planning",0);
            check(BossAudioTestAccess::Count(audio)==2&&BossAudioTestAccess::Level(audio,"home")>.99F&&
                  BossAudioTestAccess::Level(audio,"planning")==0,"Track change cut the outgoing music");
            sync("planning",24);
            const float old=BossAudioTestAccess::Level(audio,"home"),next=BossAudioTestAccess::Level(audio,"planning");
            check(old>.45F&&old<.55F&&next>.45F&&next<.55F,"Crossfade did not overlap at half volume");
            sync("planning",30,true);
            check(BossAudioTestAccess::Paused(audio)&&BossAudioTestAccess::Level(audio,"home")==old&&
                  BossAudioTestAccess::Level(audio,"planning")==next,"Pause advanced or played the crossfade");
            audio.Sync(nullptr,true,1,"home",0);
            check(BossAudioTestAccess::Buffer(audio,"home")==original&&BossAudioTestAccess::Count(audio)==2&&
                  BossAudioTestAccess::Level(audio,"home")==old,"Quick return restarted or jumped the original stream");
            sync("home",60);
            check(BossAudioTestAccess::Count(audio)==1&&BossAudioTestAccess::Level(audio,"home")>.99F,
                  "Completed crossfade retained the outgoing stream");
            for(int i=0;i<30;++i)sync(i%3==0?"planning":i%3==1?"quiet":"home",3);
            check(BossAudioTestAccess::Count(audio)<=3,"Rapid navigation duplicated music streams");
            sync("battle",60,false,0);sync("battle",1,false,1);
            check(audio.MusicReady()&&BossAudioTestAccess::Count(audio)==1,"Muted transition failed to settle");
            sync("",60);check(BossAudioTestAccess::Count(audio)==0,"Fade to silence leaked music streams");
            StoryBoss boss;boss.StartSandbox(StoryBossKind::FrostNova,2600,1600);
            sync("home",60);audio.Sync(&boss,false,1,"home",0);
            check(audio.CurrentTrack()=="frost_phase1"&&audio.MusicReady()&&BossAudioTestAccess::Count(audio)==2,
                  "Boss music bypassed crossfade");
            for(int i=0;i<60;++i)audio.Sync(&boss,false,1,"home",1.0F/60);
            sync("home",60);check(BossAudioTestAccess::Count(audio)==1&&audio.CurrentTrack()=="home","Leaving boss did not finish the crossfade");
            std::cout<<"Music crossfade: overlap, pause/resume, rapid reversal, stream reuse/cleanup, mute and boss handoff passed\n";
            return 0;
        }
        if(argc>1&&std::string_view(argv[1])=="--mobility-music-audit") {
            auto& menu=WorldPreviewTestAccess::Menu(game);
            const auto music=[&](const char* expected){game.Update(1.0F/60);
                check(WorldPreviewTestAccess::WinterTrack(game)==expected&&WorldPreviewTestAccess::WinterAudio(game),
                      "Page music routing or decoding failed");};
            menu.OpenHome();music("home");menu.OpenSettings();music("home");
            menu.OpenCharacterSelect();music("planning");menu.OpenEquipment();music("planning");
            menu.OpenArchive();music("quiet");menu.OpenWorldMap();music("planning");
            menu.OpenHome();music("home");
            press(KEY_F12);press(KEY_F1);music("planning");
            auto& lab=WorldPreviewTestAccess::Lab(game);
            for(int index:{33,34}) {
                lab.Clear();lab.Mark(lab.Width()*.62F);lab.Select(index);press(KEY_ENTER);
                const bool frost=index==33;float target=lab.Width()*.35F;
                const auto step=[&](){
                    if(!frost&&lab.Boss().State()!=StoryBossState::LeapWindup&&lab.Boss().State()!=StoryBossState::Leap)
                        target=lab.Boss().Feet().x+(lab.Boss().Feet().x<lab.Width()/2?800:-800);
                    WorldPreviewTestAccess::Place(game,{target,605});game.Update(1.0F/60);
                };
                const auto reach=[&](StoryBossState state){for(int i=0;i<5000&&lab.Boss().State()!=state;++i)step();
                    check(lab.Boss().State()==state,"New winter movement skill never triggered");};
                const auto shot=[&](const char* name){WorldPreviewTestAccess::RestoreCamera(game,
                    std::clamp((lab.Boss().MobilityStart()+lab.Boss().MobilityTarget())/2,640.0F,lab.Width()-640));capture(name);};
                reach(frost?StoryBossState::FrostStepWindup:StoryBossState::LeapWindup);
                check(WorldPreviewTestAccess::WinterTrack(game)==(frost?"frost_phase1":"patriot"),"Boss music did not replace page music");
                for(int i=0;i<12;++i)step();shot(frost?"frost-step-warning":"patriot-leap-warning");
                reach(frost?StoryBossState::FrostStep:StoryBossState::Leap);
                for(int i=0;i<18;++i)step();shot(frost?"frost-step-moving":"patriot-leap-moving");
                reach(frost?StoryBossState::FrostFanWindup:StoryBossState::Recover);
                for(int i=0;i<3;++i)step();shot(frost?"frost-step-followup":"patriot-leap-impact");
                lab.Clear();music("planning");
            }
            press(KEY_ESCAPE);music("home");
            WorldPreviewTestAccess::Enter(game,"clinic","");music("quiet");
            WorldPreviewTestAccess::TestThreat(game);music("battle");
            std::cout<<"Page BGM, boss music takeover/return, Frost step and Patriot leap rendering passed\n";
            return 0;
        }
        if(argc>1&&(std::string_view(argv[1])=="--crown-shadow-audit"||std::string_view(argv[1])=="--crown-client-preview")) {
            check(game.WorldPlayerPosition().x>=0,"Player fixture unavailable");
            WorldPreviewTestAccess::Home(game);press(KEY_F12);press(KEY_F1);
            check(WorldPreviewTestAccess::ShadowGlyphs(game),"Signature glyphs missing");
            check(WorldPreviewTestAccess::CrownEffects(game),"Original Crown client textures/timelines missing");
            auto& lab=WorldPreviewTestAccess::Lab(game);
            lab.Clear();lab.Mark(lab.Width()*.62F);lab.Select(31);press(KEY_ENTER);
            const auto step=[&](){
                const auto& boss=lab.Boss();
                const float x=boss.CustomCrownslayer().ShadowActive()?boss.CustomCrownslayer().ShadowAim().x+370:
                    boss.Feet().x+(boss.Feet().x<lab.Width()*.5F?460:-460);
                WorldPreviewTestAccess::Place(game,{x,605});game.Update(1.0F/60);
            };
            const auto reach=[&](StoryBossState state){
                for(int i=0;i<5000&&lab.Boss().State()!=state;++i)step();
                check(lab.Boss().State()==state,"Signature integrated state missing");
            };
            const auto shot=[&](const char* name){
                const float saved=WorldPreviewTestAccess::Camera(game);
                WorldPreviewTestAccess::RestoreCamera(game,std::clamp(lab.Boss().CustomCrownslayer().ShadowAim().x,640.0F,lab.Width()-640));
                capture(name);WorldPreviewTestAccess::RestoreCamera(game,saved);
            };
            reach(StoryBossState::ShadowWindup);
            if(std::string_view(argv[1])=="--crown-client-preview") {
                const auto aim=lab.Boss().CustomCrownslayer().ShadowAim();
                std::filesystem::create_directories(output/"client-crown-frames");
                for(int i=0;i<330;++i) {
                    WorldPreviewTestAccess::Place(game,{aim.x+370,605});game.Update(1.0F/60);
                    if(i%2==0) {
                        WorldPreviewTestAccess::RestoreCamera(game,std::clamp(aim.x,640.0F,lab.Width()-640));
                        capture(TextFormat("client-crown-frames/frame-%03i",i/2));
                    }
                }
                std::cout<<"Original client Crown combat preview: 165 actual game frames exported at 30 fps\n";
                return 0;
            }
            for(int i=0;i<20;++i)step();shot("crown-shadow-warning");
            for(int slash=0;slash<3;++slash) {
                reach(StoryBossState::ShadowSlash);
                check(lab.Boss().CustomCrownslayer().ShadowStrike()==slash,"Signature slash order changed");
                for(int i=0;i<7;++i)step();shot(("crown-shadow-slash-"+std::to_string(slash+1)).c_str());
                while(lab.Boss().State()==StoryBossState::ShadowSlash)step();
                if(slash<2) {
                    check(lab.Boss().State()==StoryBossState::ShadowPause,"Missing harmless preparation after teleport");
                    for(int i=0;i<12;++i)step();shot(("crown-shadow-prepare-"+std::to_string(slash+2)).c_str());
                }
            }
            reach(StoryBossState::ShadowFinalWindup);for(int i=0;i<12;++i)step();shot("crown-shadow-final-warning");
            reach(StoryBossState::ShadowFinal);for(int i=0;i<11;++i)step();shot("crown-shadow-plunge");
            reach(StoryBossState::Recover);for(int i=0;i<6;++i)step();shot("crown-shadow-impact");
            check(lab.God()&&!lab.Boss().CustomCrownslayer().CanDealContactDamage(),"Signature counterattack window missing");
            // Also enter the real homepage B encounter and run its own controller.
            press(KEY_ESCAPE);WorldPreviewTestAccess::Home(game);press(KEY_B);
            check(WorldPreviewTestAccess::DemoActive(game)==false,"Demo started before its trigger");
            for(int i=0;i<2000&&WorldPreviewTestAccess::DemoBoss(game).CurrentState()!=Boss::State::ShadowWindup;++i) {
                WorldPreviewTestAccess::ObserveDemo(game);game.Update(1.0F/60);
            }
            check(WorldPreviewTestAccess::DemoActive(game)&&WorldPreviewTestAccess::DemoBoss(game).ShadowActive(),"Homepage B signature missing");
            capture("crown-shadow-homepage-b");
            std::cout<<"Crown signature: lab/homepage B, three slashes, aerial finisher, sprites, warnings and glyphs passed\n";
            return 0;
        }
        if(argc>1&&std::string_view(argv[1])=="--boss-variety-audit") {
            WorldPreviewTestAccess::Home(game);press(KEY_F12);press(KEY_F1);
            check(WorldPreviewTestAccess::LabActive(game)&&WorldPreviewTestAccess::VarietyGlyphs(game),"New skill UI or glyphs unavailable");
            auto& lab=WorldPreviewTestAccess::Lab(game);
            const auto step=[&](){
                const auto& boss=lab.Boss();
                const float direction=boss.Feet().x<(boss.Left()+boss.Right())*.5F?1.0F:-1.0F;
                WorldPreviewTestAccess::Place(game,{boss.Feet().x+direction*460,605});game.Update(1.0F/60);
            };
            const auto reach=[&](StoryBossState state,bool low=false){
                for(int i=0;i<10000;++i){
                    if(lab.Boss().State()==state&&(!low||lab.Boss().LowArcVolley()))return;
                    step();
                }
                throw std::runtime_error("Integrated new skill never started: "+std::to_string(int(state)));
            };
            const auto captureLab=[&](const std::string& name,bool aim=false){
                const float saved=WorldPreviewTestAccess::Camera(game);
                const float focus=aim?lab.Boss().Aim().x:lab.Boss().Position().x;
                WorldPreviewTestAccess::RestoreCamera(game,std::clamp(focus,640.0F,lab.Width()-640));
                capture(name);WorldPreviewTestAccess::RestoreCamera(game,saved);
            };
            const auto spawn=[&](int index){lab.Clear();lab.Mark(lab.Width()*.62F);lab.Select(index);press(KEY_ENTER);};
            spawn(31);reach(StoryBossState::FanRetreat);
            for(int i=0;i<18;++i)step();captureLab("variety-crown-retreat-warning");
            while(lab.Boss().State()==StoryBossState::FanRetreat)step();
            for(int i=0;i<8;++i)step();captureLab("variety-crown-fan");
            spawn(32);reach(StoryBossState::BarrageWindup);captureLab("variety-w-barrage-warning",true);
            for(int i=0;i<86;++i)step();captureLab("variety-w-barrage-explosion",true);
            reach(StoryBossState::VolleyWindup,true);captureLab("variety-w-low-volley-warning");
            while(lab.Boss().State()==StoryBossState::VolleyWindup)step();
            for(int i=0;i<20;++i)step();captureLab("variety-w-low-volley");
            spawn(33);reach(StoryBossState::FrostFanWindup);captureLab("variety-frost-fan-warning");
            while(lab.Boss().State()==StoryBossState::FrostFanWindup)step();
            for(int i=0;i<20;++i)step();captureLab("variety-frost-fan");
            spawn(34);lab.Boss().Damage(10000);reach(StoryBossState::SpearRainWindup);
            captureLab("variety-patriot-spear-warning",true);
            while(lab.Boss().State()==StoryBossState::SpearRainWindup)step();
            for(int i=0;i<12;++i)step();captureLab("variety-patriot-spear-rain",true);
            check(lab.God(),"Visual observation fixture lost its invincibility");
            std::cout<<"Integrated new Boss skills, existing sprites, warnings, projectiles, explosions and Chinese glyphs passed\n";
            return 0;
        }
        if(argc>1&&std::string_view(argv[1])=="--winter-boss-audit") {
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            check(WorldPreviewTestAccess::LabArt(game)&&WorldPreviewTestAccess::WinterGlyphs(game),"Winter sprites/effects/glyphs unavailable");
            // Validate real story triggers, both lives, exit locks and continuation.
            for(const char* id:{"ice","industry"}) {
                WorldPreviewTestAccess::Action(game,id);check(WorldLayout::Internal(game.WorldRegion()),"Winter action skipped patrol approach");
                WorldPreviewTestAccess::Enter(game,id,std::string("approach_")+id);
                const auto& b=WorldPreviewTestAccess::BossFight(game);
                check(b.TwoLives()&&!b.Active()&&WorldPreviewTestAccess::BossArt(game),"Wrong winter story encounter");
                WorldPreviewTestAccess::Place(game,{b.TriggerX()+20,605});advance(100);
                check(b.Active()&&b.Targetable()&&WorldPreviewTestAccess::WinterAudio(game),"Winter physical trigger/music failed");
                WorldPreviewTestAccess::Place(game,{450,605});advance(1);
                check(game.WorldPlayerPosition().x< b.TriggerX()-120,"Winter arena still has an invisible left wall");
                WorldPreviewTestAccess::Place(game,{WorldLayout::Find(id)->width-100,605});advance(1);
                check(game.WorldPlayerPosition().x>WorldLayout::Find(id)->width-180&&game.WorldRegion()==id,
                    "Winter arena still has an invisible right wall or open exit");
                WorldPreviewTestAccess::Place(game,{b.Feet().x-260,605});const float hp=b.Health();
                key(KEY_J,true);advance(45);key(KEY_J,false);check(b.Health()<hp,"Real gunfire did not damage winter Boss");
                capture(std::string(id)+"-story-first-life");WorldPreviewTestAccess::BossShot(game,10000);advance(1);
                check(b.State()==StoryBossState::Revive&&!b.Defeated(),"First-life bullet prematurely cleared story");
                const auto ports=WorldLayout::Ports(id,true);const WorldLayout::Port* next=nullptr;
                for(const auto& port:ports)if(port.route->kind==WorldLayout::RouteKind::Main&&!port.returning)next=&port;
                check(next,"Winter Boss has no main exit");WorldPreviewTestAccess::Place(game,{next->x+10,605});press(KEY_F);
                check(game.WorldRegion()==id&&!WorldPreviewTestAccess::Transition(game),"First-life death unlocked exit");
                capture(std::string(id)+"-story-revival");
                for(int i=0;i<600&&!b.Targetable();++i){WorldPreviewTestAccess::ObserveW(game,500);advance(1);}
                check(b.PhaseTwo()&&b.Targetable()&&b.Health()==b.MaxHealth(),"Winter story revival lost full second life");
                advance(1);check(WorldPreviewTestAccess::WinterTrack(game)==(std::string(id)=="ice"?"frost_phase2":"patriot"),"Winter phase music switch failed");
                capture(std::string(id)+"-story-second-life");WorldPreviewTestAccess::BossShot(game,10000);advance(1);
                check(b.Defeated()&&b.Zones().empty()&&b.Missiles().empty(),"Winter final defeat retained active attacks");
                WorldPreviewTestAccess::Place(game,{next->x+10,605});advance(90);
                check(game.WorldRegion()==next->target->id,"Final winter death did not open story continuation");
                WorldPreviewTestAccess::Enter(game,id,next->target->id);check(b.Defeated(),"Returning resurrected winter Boss");
            }
            press(KEY_ESCAPE);WorldPreviewTestAccess::Home(game);press(KEY_F12);check(WorldPreviewTestAccess::LabActive(game),"Winter sandbox entry failed");auto& lab=WorldPreviewTestAccess::Lab(game);press(KEY_F1);
            const auto save=Utf8Path(GetApplicationDirectory())/"save/game_progress.json";
            const auto readSave=[&](){std::ifstream in(save,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};
            const auto saved=readSave();const float ability=WorldPreviewTestAccess::Adaptive(game).Ability();
            // Use god mode solely for sustained art observation; collision is tested separately.
            for(int index:{33,34}) {
                lab.Select(index);press(KEY_ENTER);auto& b=lab.Boss();const bool frost=index==33;
                const std::string prefix=frost?"frost-":"patriot-";std::set<StoryBossState> seen;
                bool seals=false,wave=false,flight=false,groundWave=false,rainImpact=false;capture(prefix+"initial");
                for(int i=0;i<1500;++i) {
                    WorldPreviewTestAccess::Place(game,{b.Feet().x-(frost?310:i<400?420:100),605});advance(1);
                    if((b.State()==StoryBossState::FrostCast||b.State()==StoryBossState::IceRingWindup||b.State()==StoryBossState::IceRing||
                        b.State()==StoryBossState::BlackIceWindup||b.State()==StoryBossState::ComboWindup||b.State()==StoryBossState::Combo||
                        b.State()==StoryBossState::ChargeWindup||b.State()==StoryBossState::Charge||
                        b.State()==StoryBossState::IceRainWindup||b.State()==StoryBossState::IceRain||
                        b.State()==StoryBossState::GuardWindup||b.State()==StoryBossState::Guard)&&b.StateTime()>.12F&&!seen.contains(b.State())) {
                        seen.insert(b.State());capture(prefix+"phase1-skill-"+std::to_string(int(b.State())));
                    }
                    if(!seals)for(const auto& z:b.Zones())if(z.kind==BossFx::BlackIce&&z.delay<=0){seals=true;capture(prefix+"black-ice-active");break;}
                    if(frost&&!rainImpact)for(const auto& z:b.Zones())if(z.kind==BossFx::IceRain&&z.delay<=0){rainImpact=true;capture("frost-ice-rain-impact");break;}
                    if(!frost&&!groundWave)for(const auto& m:b.Missiles())if(m.kind==BossFx::GroundWave){groundWave=true;capture("patriot-ground-wave");break;}
                    if(frost&&seen.contains(StoryBossState::IceRing)&&seals&&rainImpact)break;
                    if(!frost&&seen.contains(StoryBossState::GuardWindup)&&seen.contains(StoryBossState::Guard)&&
                       seen.contains(StoryBossState::ChargeWindup)&&seen.contains(StoryBossState::Charge)&&
                       seen.contains(StoryBossState::Combo)&&groundWave&&b.StateTime()>1.2F)break;
                }
                check(frost?(seen.contains(StoryBossState::IceRing)&&seals&&rainImpact):
                    (seen.contains(StoryBossState::Guard)&&seen.contains(StoryBossState::Charge)&&
                     seen.contains(StoryBossState::Combo)&&groundWave),
                    "Winter first-life effects not observed");
                if(frost) {
                    const auto tile=b.SealedFloor().front();
                    press(KEY_F4);press(KEY_F3);press(KEY_F9);
                    WorldPreviewTestAccess::Place(game,{tile.x+tile.width*.5F,605});advance(45);
                    check(!b.SealedGroundAt(game.WorldPlayerPosition().x)&&WorldPreviewTestAccess::FirstOperatorHealth(game)==1,
                        "Sealed floor did not remove support, injure and rescue the operator");
                    capture("frost-floor-seal-fall");
                    for(int shot=0;shot<4;++shot){WorldPreviewTestAccess::SealShot(game,tile.x+tile.width*.5F);advance(1);}
                    check(!b.SealedGroundAt(tile.x+tile.width*.5F),"Actual game projectiles did not restore sealed floor");
                    capture("frost-floor-seal-repaired");
                    press(KEY_F9);press(KEY_F4);
                }
                WorldPreviewTestAccess::LabShot(game,b.Position());advance(40);capture(prefix+"revival");
                check(b.State()==StoryBossState::Revive&&!b.Defeated(),"Sandbox first death skipped revival");
                for(int i=0;i<500&&!b.PhaseTwo();++i)advance(1);check(b.PhaseTwo(),"Sandbox second life missing");
                capture(prefix+"phase2-guard");seen.clear();
                for(int i=0;i<2000;++i) {
                    // Induce Patriot's distant spear, then his close destruction strike.
                    const float distance=frost?310:flight?170:420;
                    const float side=b.Feet().x<(b.Left()+b.Right())*.5F?1.0F:-1.0F;
                    WorldPreviewTestAccess::Place(game,{b.Feet().x+side*distance,605});advance(1);
                    if((b.State()==StoryBossState::IceRingWindup||b.State()==StoryBossState::IceRing||b.State()==StoryBossState::BlackIceWindup||
                        b.State()==StoryBossState::Combo||b.State()==StoryBossState::SpearWindup)&&b.StateTime()>.20F&&!seen.contains(b.State())) {
                        seen.insert(b.State());capture(prefix+"phase2-skill-"+std::to_string(int(b.State())));
                    }
                    if(b.State()==StoryBossState::SpearWindup&&b.StateTime()>1.20F&&!seen.contains(StoryBossState::Spear)) {
                        seen.insert(StoryBossState::Spear);capture(prefix+"spear-fixed-lock");
                    }
                    for(const auto& m:b.Missiles()) {
                        if(m.kind==BossFx::Spear&&!flight){flight=true;capture(prefix+"spear-flight");}
                        if(m.kind==BossFx::ColdWave&&!wave){wave=true;capture(prefix+"cold-wave");}
                    }
                    if(frost&&wave&&seen.contains(StoryBossState::IceRing)&&seen.contains(StoryBossState::BlackIceWindup))break;
                    if(!frost&&flight&&seen.contains(StoryBossState::Combo))break;
                }
                check(frost?wave&&seen.contains(StoryBossState::IceRing):flight&&seen.contains(StoryBossState::Combo),"Winter second-life effects not observed");
                while(!b.Targetable())advance(1);WorldPreviewTestAccess::LabShot(game,b.Position());advance(1);check(b.Defeated(),"Sandbox second life not killable");
                capture(prefix+"defeated");press(KEY_DELETE);
            }
            // God disabled: actual native ice projectile must inflict both health damage and cold.
            lab.Select(33);press(KEY_ENTER);press(KEY_F9);bool cold=false,frozen=false;
            for(int i=0;i<900&&!frozen;++i){WorldPreviewTestAccess::Place(game,{lab.Boss().Feet().x-230,605});advance(1);cold|=WorldPreviewTestAccess::Cold(game);frozen|=WorldPreviewTestAccess::Frozen(game);}
            check(cold&&frozen&&WorldPreviewTestAccess::FirstOperatorHealth(game)<3,"Integrated cold stacking did not freeze injured player");
            capture("frost-player-frozen");press(KEY_ESCAPE);
            check(readSave()==saved&&WorldPreviewTestAccess::Adaptive(game).Ability()==ability,"Winter sandbox modified story/difficulty save");
            // Retry returns to first life and restores arrival protection.
            WorldPreviewTestAccess::Action(game,"industry");WorldPreviewTestAccess::Enter(game,"industry","approach_industry");
            WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).TriggerX()+20,605});advance(100);
            WorldPreviewTestAccess::DefeatOperators(game);advance(2);press(KEY_ENTER);advance(2);
            check(game.WorldRegion()=="industry"&&!WorldPreviewTestAccess::BossFight(game).Active()&&!WorldPreviewTestAccess::BossFight(game).PhaseTwo(),"Winter retry retained old phase/progress");
            std::cout<<"Winter story lives/gates/gunfire/retry, client-texture VFX/music, seal repair, sandbox phases, cold stacking and save isolation passed\n";
            return 0;
        }
        if(argc>1&&std::string_view(argv[1])=="--developer-room-audit") {
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            const auto clickGame=[&](int x,int y){mouse(7,x,y);mouse(6,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);mouse(5,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);};
            WorldPreviewTestAccess::Home(game);capture("developer-home");
            const auto save=Utf8Path(GetApplicationDirectory())/"save/game_progress.json";
            const auto readSave=[&](){std::ifstream in(save,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});};
            const auto saved=readSave();const float ability=WorldPreviewTestAccess::Adaptive(game).Ability();
            press(KEY_F12);check(WorldPreviewTestAccess::LabActive(game),"F12 did not enter sandbox");
            auto& lab=WorldPreviewTestAccess::Lab(game);
            check(WorldPreviewTestAccess::LabGlyphs(game)&&WorldPreviewTestAccess::LabArt(game),"Sandbox glyphs/client sprites missing");
            check(lab.God()&&lab.Mobs().Remaining()==0&&!lab.Boss().Present(),"Room did not start empty");
            check(GetMasterVolume()==0,"Menu restored audible automated playback");
            capture("developer-empty");
            mouse(7,180,632);mouse(6,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);
            check(lab.Mobs().Remaining()==0,"Spawn activated on mouse press");
            mouse(7,600,500);game.Update(1.0F/60);mouse(5,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);
            check(lab.Mobs().Remaining()==0,"Dragging off spawn generated enemy");
            clickGame(180,632);check(lab.Mobs().Remaining()==1,"Spawn click failed");
            press(KEY_DELETE);press(KEY_F4);
            for(int i=0;i<int(kEnemyKindCount);++i){lab.Select(i);press(KEY_ENTER);}
            check(lab.Mobs().Remaining()==31,"Catalog spawn missing unit");capture("developer-all-enemies");
            const float frozenTime=lab.Seconds();key(KEY_D,true);advance(20);key(KEY_D,false);
            check(lab.Seconds()==frozenTime,"Frozen simulation advanced");
            press(KEY_F5);check(std::abs(lab.Seconds()-frozenTime-1.0F/60)<.0001F,"Single step failed");
            press(KEY_DELETE);lab.Select(0);clickGame(180,632);
            const auto unit=lab.Mobs().Units().front();WorldPreviewTestAccess::LabShot(game,unit.Center());advance(1);
            check(lab.Mobs().Remaining()==0,"Sandbox projectile missed mob");
            press(KEY_DELETE);press(KEY_ENTER);key(KEY_J,true);advance(90);key(KEY_J,false);advance(1);
            check(lab.Mobs().Remaining()==0,"Real operator gunfire did not hit frozen target");
            lab.Select(31);press(KEY_ENTER);check(lab.Boss().Kind()==StoryBossKind::Crownslayer&&lab.Boss().Targetable(),"Frozen boss not targetable");
            WorldPreviewTestAccess::LabShot(game,lab.Boss().Position());advance(1);
            check(lab.Boss().Defeated(),"Sandbox projectile missed Crownslayer");
            lab.SetFilter(DeveloperFilter::Bosses);clickGame(180,348);check(lab.Selected()==32,"Boss filter row failed");
            clickGame(180,632);check(lab.Boss().Kind()==StoryBossKind::W&&!lab.Boss().Defeated(),"Boss replacement failed");
            capture("developer-w-controls");
            key(KEY_LEFT_SHIFT,true);mouse(7,800,560);mouse(6,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);
            const float markedX=WorldPreviewTestAccess::Camera(game)+160;
            mouse(5,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);key(KEY_LEFT_SHIFT,false);
            check(std::abs(lab.SpawnX(game.WorldPlayerPosition())-markedX)<.01F,"World marker coordinate wrong");
            lab.SetFilter(DeveloperFilter::Mobs);lab.Select(int(EnemyKind::Soldier));lab.AdjustCount(4);press(KEY_ENTER);
            check(lab.Mobs().Remaining()==5&&lab.Boss().Present(),"Mixed encounter failed");
            press(KEY_F1);check(!lab.PanelOpen(),"Hide controls failed");capture("developer-mixed-hidden");press(KEY_F1);
            clickGame(900,169);check(WorldPreviewTestAccess::Health(game)==3&&lab.Mobs().Remaining()==5,"Restore cleared foes");
            press(KEY_DELETE);lab.Select(0);lab.AdjustCount(-100);
            WorldPreviewTestAccess::Place(game,{1300,605});lab.Mark(1330);press(KEY_ENTER);press(KEY_F4);
            advance(240);check(WorldPreviewTestAccess::Health(game)==3,"Default god allowed damage");
            press(KEY_F9);check(!lab.God()&&!lab.Frozen(),"Damage fixture retained god/freeze");
            advance(240);
            check(WorldPreviewTestAccess::FirstOperatorHealth(game)<3,"God off prevented real attacks");
            press(KEY_F4);WorldPreviewTestAccess::DefeatOperators(game);advance(1);capture("developer-dead-restore");
            press(KEY_F3);check(WorldPreviewTestAccess::Health(game)==3&&lab.Mobs().Remaining()==1,"Death blocked restoration");
            press(KEY_DELETE);check(lab.Mobs().Remaining()==0&&!lab.Boss().Present()&&WorldPreviewTestAccess::Shots(game).empty(),"Clear retained hazards/bullets");
            press(KEY_ESCAPE);check(!WorldPreviewTestAccess::LabActive(game)&&!game.InWorldPreview(),"Exit did not restore home");
            check(readSave()==saved&&WorldPreviewTestAccess::Adaptive(game).Ability()==ability,"Sandbox mutated story save/adaptive profile");
            clickGame(300,417);check(WorldPreviewTestAccess::LabActive(game),"Home room button failed");press(KEY_ESCAPE);
            press(KEY_N);check(game.RemainingTrialEnemies()==int(kEnemyKindCount),"Legacy trial broken after sandbox");
            std::cout<<"Developer room UI, original models, mixed combat, freeze/step, restoration and save isolation passed\n";
            return 0;
        }
        if(argc>1&&std::string_view(argv[1])=="--w-mobility-audit") {
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            WorldPreviewTestAccess::Action(game,"wtower");WorldPreviewTestAccess::Enter(game,"wtower","approach_wtower");
            check(WorldPreviewTestAccess::BossArt(game),"W original battle animation sheet not ready");
            WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).TriggerX()+30,605});advance(50);
            std::set<StoryBossState> captured;bool minesCaptured=false,groundMineHit=false;float travel=0;
            for(int frame=0;frame<2100;++frame) {
                const auto& w=WorldPreviewTestAccess::BossFight(game);
                const bool needEvade=!captured.contains(StoryBossState::Evade);
                WorldPreviewTestAccess::ObserveW(game,needEvade?130:310);
                const float previous=w.Feet().x;advance(1);travel+=std::abs(w.Feet().x-previous);
                const auto state=w.State();
                if((state==StoryBossState::Volley||state==StoryBossState::MineWindup||state==StoryBossState::C4||
                    (state==StoryBossState::Evade&&w.Feet().y<GameConfig::kFloorY-100))&&!captured.contains(state)) {
                    captured.insert(state);
                    capture(state==StoryBossState::Volley?"w-three-grenade-volley":state==StoryBossState::MineWindup?"w-minefield-warning":
                        state==StoryBossState::Evade?"w-airborne-reposition":"w-mobile-c4");
                }
                if(!minesCaptured&&!w.Mines().empty()) {capture("w-ground-mine");minesCaptured=true;}
                if(!groundMineHit)for(const auto& mine:w.Mines())if(mine.age>.8F&&mine.fuse<0) {
                    WorldPreviewTestAccess::Place(game,{mine.position.x,605});
                    const int hp=WorldPreviewTestAccess::FirstOperatorHealth(game);
                    for(int n=0;n<110&&WorldPreviewTestAccess::FirstOperatorHealth(game)==hp;++n)advance(1);
                    groundMineHit=WorldPreviewTestAccess::FirstOperatorHealth(game)<hp;capture("w-mine-detonation");
                    advance(5);capture("w-mine-blast-expansion");
                    advance(10);capture("w-mine-blast-fade");break;
                }
                if(captured.size()==4&&groundMineHit)break;
            }
            check(captured.size()==4&&minesCaptured&&groundMineHit&&travel>200,"W moves/hazards did not reach actual gameplay");
            WorldPreviewTestAccess::ObserveW(game,310);WorldPreviewTestAccess::BossShot(game,70);advance(1);
            check(WorldPreviewTestAccess::BossFight(game).PhaseTwo(),"W mobility fight lost phase two");
            bool phaseVolley=false;
            for(int frame=0;frame<1800&&!phaseVolley;++frame) {
                WorldPreviewTestAccess::ObserveW(game,310);advance(1);
                const auto& w=WorldPreviewTestAccess::BossFight(game);
                if(w.State()==StoryBossState::Volley&&w.StateTime()>.5F) {
                    check(w.VolleyCount()==5,"Phase-two volley did not expand to five");capture("w-five-grenade-volley");phaseVolley=true;
                }
            }
            check(phaseVolley,"W did not execute phase-two volley");
            WorldPreviewTestAccess::CompleteStoryBattle(game);
            const auto& w=WorldPreviewTestAccess::BossFight(game);
            check(w.Mines().empty()&&w.Grenades().empty()&&w.Blasts().empty(),"Victory did not remove mobile W hazards");
            capture("w-mobile-defeated");
            std::cout<<"W original animations, real arena mobility, volley/mine/C4 states, player hazard damage, phase-two volley and victory cleanup passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--adaptive-audit") {
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            const float initial=WorldPreviewTestAccess::Adaptive(game).Ability();
            check(WorldPreviewTestAccess::AdaptiveGlyphs(game),"Adaptive labels contain missing glyphs");
            WorldPreviewTestAccess::Action(game,"bridge");advance(600);
            check(WorldPreviewTestAccess::Adaptive(game).Ability()==initial&&!WorldPreviewTestAccess::Adaptive(game).Active(),"Exploration changed adaptive difficulty");
            const float frozen=WorldPreviewTestAccess::Difficulty(game).health;
            WorldPreviewTestAccess::Place(game,{WorldLayout::Find(game.WorldRegion())->width*.34F,605});advance(100);
            check(WorldPreviewTestAccess::Adaptive(game).Active(),"First wave failed to begin performance sampling");
            check(WorldPreviewTestAccess::Patrol(game).Difficulty().health==frozen,"Wave did not receive encounter profile");
            check(WorldPreviewTestAccess::Adaptive(game).Ability()==initial,"An ongoing fight raised difficulty");
            capture("adaptive-first-wave");
            WorldPreviewTestAccess::CompleteBattleFixture(game);
            const float improved=WorldPreviewTestAccess::Adaptive(game).Ability();
            check(improved>initial&&improved-initial<=.08001F,"Real successful patrol failed to increase next difficulty");
            check(WorldPreviewTestAccess::Patrol(game).Difficulty().health==frozen,"Second wave changed the frozen profile");
            WorldPreviewTestAccess::Enter(game,"bridge","approach_bridge");
            check(WorldPreviewTestAccess::BossFight(game).MaxHealth()>80*frozen,"Next Boss did not receive learned difficulty");
            WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).TriggerX()+20,605});advance(60);
            check(WorldPreviewTestAccess::Adaptive(game).Active(),"Boss trigger failed to begin performance sampling");
            WorldPreviewTestAccess::DefeatOperators(game);advance(3);
            const float reduced=WorldPreviewTestAccess::Adaptive(game).Ability();
            check(reduced<improved,"Full-party defeat did not lower difficulty");advance(180);
            check(WorldPreviewTestAccess::Adaptive(game).Ability()==reduced,"Defeat screen repeatedly lowered difficulty");
            press(KEY_ENTER);advance(3);
            check(WorldPreviewTestAccess::BossFight(game).Difficulty().health<frozen,"Retry failed to apply lowered profile");
            check(WorldPreviewTestAccess::Adaptive(game).Ability()==reduced,"Retry reset learned ability");
            capture("adaptive-boss-retry");
            WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).TriggerX()+20,605});advance(70);
            press(KEY_TAB);
            check(!WorldPreviewTestAccess::Adaptive(game).Active()&&WorldPreviewTestAccess::Adaptive(game).Ability()==reduced,"Menu exit retained metrics or punished abandonment");
            WorldPreviewTestAccess::AuditScene(game,"bridge");
            check(!WorldPreviewTestAccess::BossFight(game).Present()&&WorldPreviewTestAccess::Difficulty(game).health==1,"Design preview acquired adaptive combat");
            std::cout<<"Actual patrol learning, frozen waves, next-Boss scaling, defeat/retry, cancellation and labels passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--story-boss-audit") {
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            const auto captureBoss=[&](const std::string& name){
                const float saved=WorldPreviewTestAccess::Camera(game);
                WorldPreviewTestAccess::FocusBoss(game);capture(name);
                WorldPreviewTestAccess::RestoreCamera(game,saved);
            };
            WorldPreviewTestAccess::Home(game);press(KEY_B);
            WorldPreviewTestAccess::Place(game,{GameConfig::kBossTriggerX+10,605});advance(1);
            check(WorldPreviewTestAccess::DemoActive(game),"Homepage B no longer opens custom Crownslayer");
            for(int i=0;i<90&&WorldPreviewTestAccess::DemoBoss(game).CurrentState()!=Boss::State::Rush;++i)advance(1);
            check(WorldPreviewTestAccess::DemoBoss(game).CurrentState()==Boss::State::Rush,"Homepage B lost its rush");
            press(KEY_ESCAPE);press(KEY_S);press(KEY_ENTER);
            for(const char* id:{"bridge","wtower"}) {
                WorldPreviewTestAccess::Action(game,id);
                check(WorldLayout::Internal(game.WorldRegion()),"Boss action bypassed pre-Boss patrols");
                WorldPreviewTestAccess::Enter(game,id,std::string("approach_")+id);
                check(WorldPreviewTestAccess::BossArt(game),"Original Boss sheet failed to load");
                check(!WorldPreviewTestAccess::BossFight(game).Active(),"Boss started at scene entrance");
                capture(std::string(id)+"-before-trigger");
                const float trigger=WorldPreviewTestAccess::BossFight(game).TriggerX();
                WorldPreviewTestAccess::Place(game,{trigger+10,605});advance(1);
                check(WorldPreviewTestAccess::BossFight(game).Active(),"Boss fight did not start at physical trigger");
                capture(std::string(id)+"-arrival");advance(49);
                const float health=WorldPreviewTestAccess::BossFight(game).Health();
                WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).Feet().x-240,605});
                key(KEY_J,true);advance(48);key(KEY_J,false);
                check(WorldPreviewTestAccess::BossFight(game).Health()<health,"Actual player attack did not damage story Boss");
                capture(std::string(id)+"-gunfire");
                press(KEY_TWO);advance(1);press(KEY_ONE);
                check(game.WorldPlayerPosition().x>=WorldPreviewTestAccess::BossFight(game).Left(),"Character switch bypassed arena limits");
                WorldPreviewTestAccess::Place(game,{WorldLayout::Find(id)->width-90,605});advance(6);
                check(game.WorldRegion()==id&&!WorldPreviewTestAccess::Transition(game),"Living Boss admitted forward exit");
                WorldPreviewTestAccess::Place(game,{140,605});press(KEY_F);advance(3);
                check(game.WorldRegion()==id&&!WorldPreviewTestAccess::Transition(game),"Living Boss admitted retreat");
                WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).Feet().x-270,605});
                if(std::string_view(id)=="wtower") {
                    WorldPreviewTestAccess::BossShot(game,60);advance(1);
                    check(WorldPreviewTestAccess::BossFight(game).PhaseTwo(),"W half-health phase not integrated");
                    for(int i=0;i<240&&WorldPreviewTestAccess::BossFight(game).State()!=StoryBossState::C4;++i) {
                        // The earlier exit checks teleport the fixture across
                        // the arena. Follow mobile W to keep testing C4 in range.
                        WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).Feet().x-310,605});advance(1);
                    }
                    if(WorldPreviewTestAccess::BossFight(game).State()!=StoryBossState::C4) {
                        capture("w-c4-timeout");
                        throw std::runtime_error("W did not launch phase-two C4: state="+std::to_string(int(WorldPreviewTestAccess::BossFight(game).State()))+
                            ", distance="+std::to_string(std::abs(WorldPreviewTestAccess::BossFight(game).Feet().x-game.WorldPlayerPosition().x))+
                            ", player hp="+std::to_string(WorldPreviewTestAccess::FirstOperatorHealth(game)));
                    }
                    capture("w-c4-tracking");advance(105);capture("w-c4-locked");
                } else {
                    bool sawRush=false,sawAir=false,sawDarts=false;
                    for(int i=0;i<1200&&!(sawRush&&sawAir&&sawDarts);++i) {
                        const auto& custom=WorldPreviewTestAccess::BossFight(game);
                        WorldPreviewTestAccess::Place(game,{custom.Feet().x>1450?600.0F:2350.0F,605});
                        advance(1);
                        if(!sawRush&&custom.State()==StoryBossState::Rush){captureBoss("crownslayer-custom-rush");sawRush=true;}
                        if(!sawAir&&custom.State()==StoryBossState::AirSlash&&custom.AnimationTime()>=.16F){captureBoss("crownslayer-custom-air");sawAir=true;}
                        if(!sawDarts&&custom.CustomCrownslayer().DartCount()>0){captureBoss("crownslayer-custom-darts");sawDarts=true;}
                    }
                    check(sawRush&&sawAir&&sawDarts,"Integrated custom Crownslayer lost ranged skills");
                    auto& crown=WorldPreviewTestAccess::BossFight(game);
                    WorldPreviewTestAccess::BossShot(game,crown.MaxHealth()*.55F);
                    advance(1);
                    for(int i=0;i<480&&WorldPreviewTestAccess::BossFight(game).State()!=StoryBossState::Disappear;++i) {
                        // Keep distance to trigger the custom half-health rush.
                        WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).Feet().x+320,605});
                        advance(1);
                    }
                    check(WorldPreviewTestAccess::BossFight(game).State()==StoryBossState::Disappear,"Crownslayer did not blink in the integrated fight");
                    capture("crownslayer-flash");
                    advance(12);
                    check(WorldPreviewTestAccess::BossFight(game).State()==StoryBossState::Disappear,"Crownslayer flash skipped its animation");
                    capture("crownslayer-flash-active");
                    for(int i=0;i<180&&WorldPreviewTestAccess::BossFight(game).State()!=StoryBossState::Strike;++i) {
                        WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).Feet().x+80,605});
                        advance(1);
                    }
                    check(WorldPreviewTestAccess::BossFight(game).State()==StoryBossState::Strike,"Crownslayer never showed the revised slash");
                    capture("crownslayer-slash");
                    advance(3);capture("crownslayer-slash-active");
                    advance(7);capture("crownslayer-slash-fade");
                }
                WorldPreviewTestAccess::CompleteStoryBattle(game);capture(std::string(id)+"-defeated");
                const auto ports=WorldLayout::Ports(id,true);const WorldLayout::Port* next=nullptr;
                for(const auto& p:ports)if(p.route->kind==WorldLayout::RouteKind::Main&&!p.returning)next=&p;
                check(next!=nullptr,"Boss has no main continuation");
                WorldPreviewTestAccess::Place(game,{next->x+10,605});advance(90);
                check(game.WorldRegion()==next->target->id,"Defeated Boss did not unlock main progression");
                WorldPreviewTestAccess::Enter(game,id,next->target->id);
                check(WorldPreviewTestAccess::BossFight(game).Defeated(),"Returning resurrected story Boss");
            }
            WorldPreviewTestAccess::Action(game,"bridge");WorldPreviewTestAccess::Enter(game,"bridge","approach_bridge");
            WorldPreviewTestAccess::Place(game,{WorldPreviewTestAccess::BossFight(game).TriggerX()+20,605});advance(50);
            WorldPreviewTestAccess::DefeatOperators(game);advance(2);capture("boss-defeat-retry");press(KEY_ENTER);advance(2);
            check(game.WorldRegion()=="bridge"&&!WorldPreviewTestAccess::BossFight(game).Defeated()&&!WorldPreviewTestAccess::BossFight(game).Active(),"Retry bypassed Boss or restarted patrol route");
            check(WorldPreviewTestAccess::Actions(game).ranged==0,"Retry leaked attack input");
            std::cout<<"Both story Boss models, trigger, actual gunfire, character switching, arena gates, phase-two C4, defeat/return and retry passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--exploration-attack-audit") {
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            for(const auto& scene:WorldArt::Scenes) {
                WorldPreviewTestAccess::AuditScene(game,scene.id);
                key(KEY_J,true);game.Update(1.0F/60);key(KEY_J,false);
                check(WorldPreviewTestAccess::Actions(game).ranged>0,"Exploration scene disabled gunfire");
                check(!WorldPreviewTestAccess::Shots(game).empty(),"Exploration attack did not create projectile");
                if(std::string_view(scene.id)=="clinic")capture("exploration-gunfire");
                advance(100);
                check(WorldPreviewTestAccess::Shots(game).empty(),"Exploration projectile did not expire");
            }
            WorldPreviewTestAccess::Action(game,"gray");WorldPreviewTestAccess::Switch(game,1);
            key(KEY_J,true);
            bool emptySwing=false;
            for(int i=0;i<150;++i) {
                game.Update(1.0F/60);
                emptySwing|=WorldPreviewTestAccess::Actions(game).melee>0;
            }
            key(KEY_J,false);
            check(emptySwing,"Texas stopped attacking after sword-wave charges ran out");
            capture("exploration-texas-swing");
            WorldPreviewTestAccess::Action(game,"gray");
            WorldLayout::Port secret{};
            for(const auto& p:WorldLayout::Ports("gray",true))if(std::string_view(p.target->id)=="clock")secret=p;
            check(secret.target!=nullptr,"Missing clock entrance");
            WorldPreviewTestAccess::Place(game,{secret.x+80,secret.feetY-39});
            key(KEY_J,true);game.Update(1.0F/60);
            WorldPreviewTestAccess::Place(game,{secret.x,secret.feetY-39});advance(80);key(KEY_J,false);game.Update(1.0F/60);
            check(game.WorldRegion()=="gray"&&!WorldPreviewTestAccess::Transition(game),"Held fire entered hidden passage");
            key(KEY_F,true);advance(90);key(KEY_F,false);game.Update(1.0F/60);
            check(!WorldPreviewTestAccess::Transition(game),"F still opens hidden passage");
            check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Proximity/held fire revealed stage");
            // Test the real combat state, including queued spawns and the wave gap.
            WorldPreviewTestAccess::Action(game,"approach_bridge");
            WorldLayout::Port battleSecret{};
            for(const auto& p:WorldLayout::Ports("approach_bridge",true))if(std::string_view(p.target->id)=="clock")battleSecret=p;
            check(battleSecret.target!=nullptr&&!WorldPreviewTestAccess::InCombat(game),"Pre-battle fixture invalid");
            const float width=WorldLayout::Find("approach_bridge")->width;
            WorldPreviewTestAccess::Place(game,{width*.31F,605});game.Update(1.0F/60);
            check(WorldPreviewTestAccess::InCombat(game)&&WorldPreviewTestAccess::Pending(game)>0,"Queued encounter not treated as combat");
            WorldPreviewTestAccess::Place(game,{battleSecret.x,battleSecret.feetY-39});press(KEY_J);advance(80);
            check(game.WorldRegion()=="approach_bridge"&&!WorldPreviewTestAccess::Transition(game),"Battle attack entered hidden passage");
            check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Blocked attack discovered hidden stage");
            for(int i=0;i<600;++i)WorldPreviewTestAccess::TickSafe(game);
            check(WorldPreviewTestAccess::Alive(game)==0&&WorldPreviewTestAccess::Pending(game)==0&&WorldPreviewTestAccess::InCombat(game),"Wave gap unlocked exploration early");
            key(KEY_J,true);game.Update(1.0F/60);
            check(!WorldPreviewTestAccess::Transition(game),"Wave gap admitted hidden passage");
            WorldPreviewTestAccess::CompleteBattleFixture(game);
            check(!WorldPreviewTestAccess::InCombat(game),"Clear retained combat state");
            WorldPreviewTestAccess::Place(game,{battleSecret.x,battleSecret.feetY-39});game.Update(1.0F/60);
            check(!WorldPreviewTestAccess::Transition(game),"Held battle press opened passage after clear");
            key(KEY_J,false);game.Update(1.0F/60);advance(70);capture("hidden-passage-after-clear");
            press(KEY_J);check(WorldPreviewTestAccess::Transition(game),"Fresh attack after clear did not open passage");
            advance(90);check(game.WorldRegion()=="clock","Hidden passage transition failed");
            check(ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Committed entry did not discover stage");
            capture("hidden-passage-arrival");
            WorldPreviewTestAccess::Action(game,"gray");WorldPreviewTestAccess::BindAttack(game,KEY_H);
            WorldPreviewTestAccess::Place(game,{secret.x,secret.feetY-39});press(KEY_J);
            check(!WorldPreviewTestAccess::Transition(game),"Old attack binding still opens passage");
            press(KEY_H);advance(90);check(game.WorldRegion()=="clock","Rebound attack did not enter hidden passage");
            WorldPreviewTestAccess::BindAttack(game,KEY_J);
            WorldPreviewTestAccess::Action(game,"clinic");
            for(const auto& p:WorldLayout::Ports("clinic",true))if(std::string_view(p.target->id)=="ward")
                WorldPreviewTestAccess::Place(game,{p.x,p.feetY-39});
            press(KEY_J);advance(90);check(game.WorldRegion()=="clinic","Attack bypassed locked ward shortcut");
            WorldPreviewTestAccess::Action(game,"approach_bridge");
            WorldPreviewTestAccess::Place(game,{battleSecret.x,battleSecret.feetY-39});press(KEY_J);advance(90);
            check(game.WorldRegion()=="clock","Pre-battle exploration was blocked by combat-area classification");
            std::cout<<"Exploration attacks in all 20 scenes, projectile expiry, empty Texas swings, fresh/rebound attack entry, combat/spawn/wave-gap blocking, post-clear entry, ward lock and pre-battle entry passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--prts-audit") {
            using Topic=PrtsNarrator::Topic;
            check(WorldPreviewTestAccess::NarrationGlyphs(game),"PRTS dialogue contains unsupported glyphs");
            WorldPreviewTestAccess::Action(game,"approach_bridge");
            const auto advance=[&](int frames){for(int i=0;i<frames;++i)game.Update(1.0F/60);};
            advance(510);WorldPreviewTestAccess::TestThreat(game);
            key(KEY_J,true);advance(220);key(KEY_J,false);
            check(WorldPreviewTestAccess::Narrator(game).CurrentTopic()==Topic::Wave,"Encounter lacks opening briefing");
            const auto opening=WorldPreviewTestAccess::Narrator(game).Revision();
            capture("prts-battle-opening");
            check(WorldPreviewTestAccess::Injure(game),"First damage fixture was rejected");
            advance(120);check(WorldPreviewTestAccess::Injure(game),"Second damage fixture was rejected");
            advance(100);
            check(WorldPreviewTestAccess::Narrator(game).Revision()==opening,"Gunfire or real health changes caused mid-fight commentary");
            capture("prts-combat-quiet");
            WorldPreviewTestAccess::CompleteBattleFixture(game);advance(120);
            check(WorldPreviewTestAccess::Narrator(game).CurrentTopic()==Topic::Ranged,"Whole-battle result lacks actual shooting style");
            check(WorldPreviewTestAccess::Narrator(game).Text().find("生命状态危险")!=std::string::npos,"Post-battle result lost health state");
            capture("prts-post-battle-summary");
            WorldPreviewTestAccess::Action(game,"gray");
            advance(540);
            const auto before=WorldPreviewTestAccess::Narrator(game).Revision();
            for(const auto& p:WorldLayout::Ports("gray",true))if(std::string_view(p.target->id)=="clock")
                WorldPreviewTestAccess::Place(game,{p.x,p.feetY-39});
            advance(30);
            check(WorldPreviewTestAccess::Narrator(game).Revision()==before,"Entrance proximity triggered secret commentary");
            WorldPreviewTestAccess::Enter(game,"clock","gray");advance(110);
            check(WorldPreviewTestAccess::Narrator(game).CurrentTopic()==Topic::Hidden,"Committed discovery lacks PRTS reaction");
            capture("prts-unplanned-entry");
            std::cout<<"PRTS phase integration: opening, quiet combat despite shooting/injury, whole-battle summary, no proximity disclosure, hidden entry and glyphs passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--audio-audit") {
            check(IsAudioDeviceReady(),"Audio device unavailable; playback not verified");
            // Keep automated checks silent while retaining the actual playback
            // path. Effects mute is tested independently below.
            SetMasterVolume(0);
            auto& audio=WorldPreviewTestAccess::Audio(game);
            for(int i=0;i<static_cast<int>(AudioCue::Count);++i) {
                const auto cue=static_cast<AudioCue>(i);
                check(audio.Available(cue),"Original client WAV failed to load");
                check(audio.Play(cue),"Initial cue rejected");
                check(!audio.Play(cue),"Same-frame cue stacking was not limited");
            }
            check(AudioSystemTestAccess::Playing(audio),"Playback did not start");
            audio.SetEffectsVolume(0);
            check(!AudioSystemTestAccess::Playing(audio),"Mute retained playing tails");
            AudioSystemTestAccess::Age(audio);
            check(!audio.Play(AudioCue::Gunshot),"Muted shot started playback");
            audio.SetEffectsVolume(.5F);
            check(audio.Play(AudioCue::Gunshot),"Unmute did not restore playback");
            check(!audio.Play(AudioCue::Count),"Invalid cue was accepted");
            { AudioSystem absent(output/"missing-audio");check(!absent.Play(AudioCue::Gunshot),"Missing asset was not handled safely"); }
            AudioSystemTestAccess::Age(audio);
            WorldPreviewTestAccess::Switch(game,0);
            check(AudioSystemTestAccess::Last(audio,AudioCue::OperatorSwitch)<0,"Same operator emitted deployment sound");
            WorldPreviewTestAccess::Switch(game,1);
            check(AudioSystemTestAccess::Last(audio,AudioCue::OperatorSwitch)>=0,"Operator switch lacks sound");
            map.OpenWorldMap("clinic");
            map.Update();check(map.Feedback()==MenuFeedback::None,"Idle menu emitted feedback");
            click(710,680);check(map.Feedback()==MenuFeedback::None,"Blank click emitted feedback");
            click(600,227);check(map.Feedback()==MenuFeedback::Select,"Node selection lacks feedback");
            click(1090,594);check(map.Feedback()==MenuFeedback::Confirm,"Start action lacks confirmation");
            GameSettings controls(output/"audio-controls");
            Player fighter;fighter.Reset(OperatorKind::Exusiai);std::vector<Bullet> shots;
            const auto tick=[&]{fighter.Update(1.0F/60,{900,605},20,shots,audio,controls);};
            AudioSystemTestAccess::Age(audio);
            key(KEY_R,true);tick();key(KEY_R,false);tick();
            check(AudioSystemTestAccess::Last(audio,AudioCue::Reload)<0,"Full magazine emitted reload sound");
            key(KEY_J,true);tick();key(KEY_J,false);tick();
            check(!shots.empty()&&AudioSystemTestAccess::Last(audio,AudioCue::Gunshot)>=0,"Actual gunfire lacks original sound");
            key(KEY_R,true);tick();key(KEY_R,false);tick();
            check(AudioSystemTestAccess::Last(audio,AudioCue::Reload)>=0,"Valid reload lacks sound");
            key(KEY_Q,true);tick();key(KEY_Q,false);tick();
            check(AudioSystemTestAccess::Last(audio,AudioCue::Skill)>=0,"Skill activation lacks sound");
            fighter.Reset(OperatorKind::Texas);AudioSystemTestAccess::Age(audio);shots.clear();
            key(KEY_J,true);tick();key(KEY_J,false);tick();
            check(!shots.empty()&&AudioSystemTestAccess::Last(audio,AudioCue::Slash)>=0,"Sword attack lacks sound");
            key(KEY_Q,true);tick();key(KEY_Q,false);tick();
            check(AudioSystemTestAccess::Last(audio,AudioCue::SwordRain)>=0,"Sword rain lacks original sound");
            AudioSystemTestAccess::Age(audio);
            WorldPreviewTestAccess::AuditScene(game,"clinic");
            check(AudioSystemTestAccess::Last(audio,AudioCue::SceneEnter)>=0,"Scene entry lacks sound");
            check(AudioSystemTestAccess::Last(audio,AudioCue::HiddenEnter)<0,"Ordinary entry disclosed hidden route");
            WorldPreviewTestAccess::Action(game,"approach_bridge");
            AudioSystemTestAccess::Age(audio);
            WorldPreviewTestAccess::Place(game,{WorldLayout::Find("approach_bridge")->width*.22F,605});
            key(KEY_F,true);for(int i=0;i<75;++i)game.Update(1.0F/60);
            const auto investigationTime=AudioSystemTestAccess::Last(audio,AudioCue::Investigate);
            check(investigationTime>=0,"Completed investigation lacks sound");
            for(int i=0;i<600;++i)game.Update(1.0F/60);
            check(AudioSystemTestAccess::Last(audio,AudioCue::Investigate)==investigationTime,"Held investigation replayed after note expired");
            key(KEY_F,false);game.Update(1.0F/60);
            WorldPreviewTestAccess::Action(game,"clinic");
            WorldPreviewTestAccess::Enter(game,"gray","clinic");
            AudioSystemTestAccess::Age(audio);
            for(const auto& port:WorldLayout::Ports("gray",true))if(std::string_view(port.target->id)=="clock")
                WorldPreviewTestAccess::Place(game,{port.x,port.feetY-39});
            game.Update(1.0F/60);
            check(AudioSystemTestAccess::Last(audio,AudioCue::HiddenEnter)<0,"Proximity disclosed undiscovered entrance");
            WorldPreviewTestAccess::Enter(game,"clock","gray");
            check(game.WorldRegion()=="clock"&&AudioSystemTestAccess::Last(audio,AudioCue::HiddenEnter)>=0,"First hidden entry lacks discovery cue");
            AudioSystemTestAccess::Age(audio);
            WorldPreviewTestAccess::Enter(game,"clock","gray");
            check(AudioSystemTestAccess::Last(audio,AudioCue::HiddenEnter)<0&&AudioSystemTestAccess::Last(audio,AudioCue::SceneEnter)>=0,"Visited entrance repeated discovery cue");
            std::cout<<"20 original client clips loaded; playback, cooldown, mute, missing assets, valid menu/combat/switch/scene triggers passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--depth-audit") {
            // Transparent layers must leave the playable band clear, and every
            // region must load its theme after switching between theme families.
            std::set<std::uint64_t> foregroundFingerprints;
            const auto validateAsset=[&](const std::string& name,bool foreground) {
                const auto path=Utf8Path(GetApplicationDirectory())/"assets/environment/depth"/(name+".png");
                auto image=LoadImage(path.string().c_str());check(image.data!=nullptr,"Missing depth asset");
                auto* pixels=LoadImageColors(image);
                int occupied=0;int opaqueMiddle=0;
                std::uint64_t fingerprint=14695981039346656037ULL;
                for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x) {
                    const auto pixel=pixels[y*image.width+x];const auto alpha=pixel.a;
                    if(alpha>16)++occupied;
                    if(y>image.height*.4F&&y<image.height*.6F&&alpha>16)++opaqueMiddle;
                    // Ignore RGB garbage in fully transparent pixels.
                    const std::uint32_t packed=alpha>16?(std::uint32_t(pixel.r)<<24)|(std::uint32_t(pixel.g)<<16)|(std::uint32_t(pixel.b)<<8)|alpha:0;
                    fingerprint=(fingerprint^packed)*1099511628211ULL;
                }
                UnloadImageColors(pixels);UnloadImage(image);
                if(foreground)check(occupied>0&&occupied<image.width*image.height*.55F,"Foreground lacks transparent surroundings");
                else check(occupied>0&&opaqueMiddle==0,"Distant depth asset blocks middle or contains no artwork");
                if(foreground)check(foregroundFingerprints.insert(fingerprint).second,"Two regions reuse the same foreground image");
            };
            for(const char* theme:{"interior","street","system"})validateAsset(std::string(theme)+"_back",false);
            check(WorldDepth::Offset(960,WorldDepth::Back.speed)>-320&&WorldDepth::Offset(960,WorldDepth::Front.speed)<-320,
                  "Depth ordering does not produce slower far / faster near movement");
            const auto audit=[&](const WorldArt::Scene& scene){
                validateAsset(std::string(scene.id)+"_front_v2",true);
                WorldPreviewTestAccess::AuditScene(game,scene.id);
                check(WorldPreviewTestAccess::DepthReady(game),"Scene depth textures failed to load");
                WorldPreviewTestAccess::DepthCamera(game,640);capture(std::string("depth-")+scene.id+"-left");
                WorldPreviewTestAccess::DepthCamera(game,960);capture(std::string("depth-")+scene.id+"-move");
                WorldPreviewTestAccess::DepthCamera(game,WorldArt::Width(scene.id)*WorldDepth::FrontGroups[1]);capture(std::string("depth-")+scene.id+"-front");
                WorldPreviewTestAccess::DepthCamera(game,WorldArt::Width(scene.id)-640);capture(std::string("depth-")+scene.id+"-right");
            };
            for(const auto& scene:WorldArt::Scenes)audit(scene);
            for(const auto& scene:WorldArt::ApproachScenes)audit(scene);
            check(foregroundFingerprints.size()==26,"Not all regions have independent foregrounds");
            std::cout<<"All 26 unique foregrounds, asset transparency and camera ordering passed\n";
        } else if(argc>1&&std::string_view(argv[1])=="--scale-audit") {
            const auto audit=[&](const WorldArt::Scene& scene){
                WorldPreviewTestAccess::AuditScene(game,scene.id);
                check(WorldPreviewTestAccess::ArtReady(game),"Missing audit scene artwork");
                const float width=WorldArt::Width(scene.id);
                const auto at=[&](float fraction,float feet,const std::string& suffix){
                    WorldPreviewTestAccess::Place(game,{width*fraction,feet-39});
                    for(int i=0;i<50;++i)game.Update(1.0F/60);
                    capture(std::string("audit-")+scene.id+"-"+suffix);
                };
                at(.10F,644,"left");at(.50F,644,"middle");at(.90F,644,"right");
                int index=0;
                for(float fraction:scene.passages)if(fraction>0)
                    at(fraction,scene.stairway?WorldArt::UpperFloor(scene.id):644,"door"+std::to_string(index++));
            };
            for(const auto& scene:WorldArt::Scenes)audit(scene);
            for(const auto& scene:WorldArt::ApproachScenes)audit(scene);
            std::cout<<"All 26 scene scale audit screenshots captured\n";
        } else {
        // Real player flow: existing skinned menu -> action -> room -> discovery -> menu.
        press(KEY_ENTER);press(KEY_ENTER);capture("player-stages-before");
        mouse(7,1060,583);mouse(6,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);mouse(5,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);
        check(game.InWorldPreview()&&game.WorldRegion()=="clinic","Start action click did not enter clinic");
        check(game.WorldPlayerPosition().x<WorldLayout::Find("clinic")->width*.2F,"Entry skips the first half of the scene");
        const auto settle=[&](){for(int i=0;i<80;++i)game.Update(1.0F/60);};
        bool checkedBridgeLayers=false;
        std::function<void(const std::string&)> walkExit;
        walkExit=[&](const std::string& target){
            WorldPreviewTestAccess::CompleteStoryBattle(game);
            const auto ports=WorldLayout::Ports(game.WorldRegion(),!WorldPreviewTestAccess::Design(game));
            WorldLayout::Port port{};for(const auto& p:ports)if(p.target->id==target)port=p;
            if(!port.target)for(const auto& p:ports)if(WorldLayout::Internal(p.target->id)) {
                bool connects=false;for(const auto& out:WorldLayout::Ports(p.target->id,true))if(out.target->id==target)connects=true;
                if(connects){walkExit(p.target->id);walkExit(target);return;}
            }
            if(WorldLayout::Internal(game.WorldRegion())) {
                const float width=WorldLayout::Find(game.WorldRegion())->width;
                for(float fraction:{.31F,.64F}) {
                    WorldPreviewTestAccess::Place(game,{width*fraction,605});
                    for(int i=0;i<900;++i){game.Update(1.0F/60);WorldPreviewTestAccess::ClearPatrol(game);}
                }
            }
            check(port.target!=nullptr,"Missing physical exit");int count=0;
            const auto moveTo=[&](float x){
                int steps=0;
                while(std::abs(game.WorldPlayerPosition().x-x)>5&&steps++<1800) {
                    int k=game.WorldPlayerPosition().x<x?KEY_D:KEY_A;
                    key(k,true);game.Update(1.0F/60);key(k,false);
                }
                check(steps<1800,"Stair approach unreachable");
            };
            if(const auto* stairs=WorldArt::Stairs(game.WorldRegion())) {
                const float width=WorldLayout::Find(game.WorldRegion())->width;
                if(port.feetY<643&&game.WorldPlayerPosition().y>400) {
                    if(!checkedBridgeLayers) {
                        moveTo(width*.245F);press(KEY_J);settle();
                        check(game.WorldRegion()=="entry","Old ground door still active");
                        moveTo(port.x);press(KEY_J);settle();
                        check(game.WorldRegion()=="entry","Upper door activated from street");
                        capture("entry-under-bridge");
                    }
                    moveTo(stairs->right*width+30);
                    moveTo(stairs->right*width-22);settle();
                    check(std::abs(game.WorldPlayerPosition().y+39-644)<2,"Broken stairs can be entered without jumping");
                    capture("entry-broken-stair-ground");
                    key(KEY_SPACE,true);for(int i=0;i<20;++i)game.Update(1.0F/60);key(KEY_SPACE,false);
                    settle();
                    const float stairBottom=644+(stairs->bottom-WorldArt::PaintedGround("entry"))*(644/WorldArt::Find("entry")->ground);
                    check(game.WorldPlayerPosition().y+39<stairBottom+2&&game.WorldPlayerPosition().y+39>port.feetY+40,"Jump did not land on broken stair");
                    capture("entry-broken-stair-landed");
                    moveTo((stairs->left+stairs->right)*width/2);capture("entry-stair-ascent");
                    moveTo(stairs->left*width-12);settle();
                    check(std::abs(game.WorldPlayerPosition().y+39-port.feetY)<2,"Background stairs did not reach landing");
                    if(!checkedBridgeLayers) {
                        press(KEY_SPACE);settle();
                        check(std::abs(game.WorldPlayerPosition().y+39-port.feetY)<2,"Landing jump fell through bridge");
                        press(KEY_TWO);settle();
                        check(std::abs(game.WorldPlayerPosition().y+39-port.feetY)<2,"Operator switch fell through bridge");
                        press(KEY_ONE);checkedBridgeLayers=true;
                    }
                    capture("entry-upper-landing");
                } else if(port.feetY>643&&game.WorldPlayerPosition().y<400) {
                    moveTo(stairs->landingRight*width+40);settle();
                    check(std::abs(game.WorldPlayerPosition().y+39-644)<2,"Stair descent failed");
                }
            }
            while(std::abs(game.WorldPlayerPosition().x-port.x)>12 && count++<1800){int k=game.WorldPlayerPosition().x<port.x?KEY_D:KEY_A;key(k,true);game.Update(1.0F/60);key(k,false);}
            check(count<1800,"Exit unreachable");
            if(port.feetY<643)capture("entry-station-door");
            if(port.route->kind==WorldLayout::RouteKind::Main&&!port.returning){
                key(KEY_D,true);for(int i=0;i<15;++i)game.Update(1.0F/60);key(KEY_D,false);
            } else if(port.route->kind!=WorldLayout::RouteKind::Main)press(KEY_J);
            else press(KEY_F);
            settle();check(game.WorldRegion()==target,"Player traversal failed");
        };
        float wardDoorX=0;for(const auto& p:WorldLayout::Ports("clinic"))if(std::string_view(p.target->id)=="ward")wardDoorX=p.x;
        WorldPreviewTestAccess::Place(game,{wardDoorX,GameConfig::kFloorY-39});settle();capture("ward-door-closed");
        press(KEY_J);settle();
        check(game.WorldRegion()=="clinic"&&!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisited("ward"),"Locked ward shortcut admitted player");
        walkExit("gray");
        check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Hidden region revealed by adjacency");
        press(KEY_TAB);capture("player-stages-undiscovered");press(KEY_ENTER);
        // Proximity and the old F interaction must not reveal a stage.
        float secretX=0;for(const auto& p:WorldLayout::Ports("gray"))if(std::string_view(p.target->id)=="clock")secretX=p.x;
        WorldPreviewTestAccess::Place(game,{secretX,GameConfig::kFloorY-39});settle();
        capture("secret-before-investigation");
        check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Proximity revealed secret");
        key(KEY_F,true);for(int i=0;i<30;++i)game.Update(1.0F/60);key(KEY_F,false);settle();
        check(game.WorldRegion()=="gray"&&!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Old F interaction entered secret");
        walkExit("clock");press(KEY_TAB);capture("player-stages-discovered");
        ProgressSystem saved(Utf8Path(GetApplicationDirectory()));
        check(saved.RegionVisited("clock")&&saved.RegionAvailable("clock")&&!saved.RegionVisible("ward"),"Discovery save failed");
        press(KEY_ENTER);check(game.WorldRegion()=="clock"&&game.InWorldPreview(),"Discovered stage replay failed");
        capture("player-clock-room");press(KEY_TAB);
        { Game reloaded;const auto tick=[&](int k){key(k,true);reloaded.Update(1.0F/60);key(k,false);reloaded.Update(1.0F/60);};
          tick(KEY_ENTER);tick(KEY_ENTER);tick(KEY_ENTER);
          check(reloaded.InWorldPreview()&&reloaded.WorldRegion()=="clock","Restart lost selected discovery"); }
        press(KEY_ENTER);walkExit("bridge");walkExit("entry");walkExit("station");walkExit("ward");
        check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).WardShortcutOpen(),"Visiting ward prematurely opened shortcut");
        walkExit("clinic");
        check(ProgressSystem(Utf8Path(GetApplicationDirectory())).WardShortcutOpen(),"Return through ward did not persist shortcut");
        WorldPreviewTestAccess::Place(game,{wardDoorX,GameConfig::kFloorY-39});settle();capture("ward-door-open");
        walkExit("ward");walkExit("clinic");press(KEY_TAB);
        WorldPreviewTestAccess::Open(game);capture("city-west");press(KEY_ENTER);
        check(game.InWorldPreview()&&game.WorldRegion()=="clinic","Prototype test entry failed");
        capture("clinic");
        // Traverse every outgoing and reversible connection using real movement and attack/F.
        const auto enter=[&](const std::string& id){
            if(game.InWorldPreview())press(KEY_TAB);
            int start=0,target=0;
            for(int i=0;i<int(WorldLayout::Regions.size());++i){if(WorldLayout::Regions[i].id==game.WorldRegion())start=i;if(WorldLayout::Regions[i].id==id)target=i;}
            for(int i=0;i<(target-start+20)%20;++i)press(KEY_RIGHT);
            press(KEY_ENTER);check(game.WorldRegion()==id&&game.InWorldPreview(),"Map selection failed");
        };
        for(const auto& region:WorldLayout::Regions)for(const auto& port:WorldLayout::Ports(region.id)) {
            enter(region.id);
            walkExit(port.target->id);
            for(int i=0;i<5;++i)game.Update(1.0F/60);
            check(game.WorldRegion()==port.target->id,"Arrival bounced back");
        }
        for(const auto& region:WorldLayout::Regions){enter(region.id);check(WorldPreviewTestAccess::ArtReady(game),"Missing scene artwork");capture(std::string("region-")+region.id);}
        enter("gray");settle();capture("gray-street");capture("dialogue-main");
        WorldPreviewTestAccess::Place(game,{100,GameConfig::kFloorY-39});settle();capture("dialogue-return-warning");
        WorldPreviewTestAccess::Place(game,{secretX,GameConfig::kFloorY-39});settle();capture("dialogue-investigation");
        for(const auto* id:{"clinic","ward","shelter"}) {
            enter(id);WorldPreviewTestAccess::Place(game,{WorldLayout::Find(id)->width*.52F,GameConfig::kFloorY-39});
            settle();capture(std::string("scale-")+id);
            press(KEY_TWO);capture(std::string("scale-texas-")+id);
            press(KEY_SPACE);capture(std::string("scale-jump-")+id);settle();press(KEY_ONE);
        }
        enter("gray");
        check(WorldScenery::Platforms(*WorldLayout::Find("gray")).empty(),"Floating platforms remain");
        const float floor=GameConfig::kFloorY-39;
        WorldPreviewTestAccess::Place(game,{1100,floor-120});settle();
        check(std::abs(game.WorldPlayerPosition().y-floor)<1,"Continuous floor landing failed");
        press(KEY_SPACE);check(game.WorldPlayerPosition().y<floor-1,"Floor jump failed");settle();
        // Capture the actual fade, then ensure no movement/key input leaks across it.
        float end=WorldLayout::Find("gray")->width-108;
        WorldPreviewTestAccess::Place(game,{end,floor});game.Update(1.0F/60);
        for(int i=0;i<12;++i)game.Update(1.0F/60);capture("transition-fade");
        check(game.WorldRegion()=="gray","Transition skipped outgoing fade");
        press(KEY_TAB);check(game.InWorldPreview(),"Menu interrupted transition");
        settle();check(game.WorldRegion()=="entry","Automatic main transition failed");
        enter("ice");capture("ice-street");enter("station");capture("station");enter("pipes");capture("pipes");
        enter("core");press(KEY_TAB);capture("city-east");press(KEY_ENTER);capture("core");
        enter("prts");press(KEY_TAB);capture("city-system");press(KEY_ENTER);capture("system-room");
        enter("throne");capture("throne");press(KEY_ESCAPE);check(!game.InWorldPreview(),"Map return failed");
        // Exercise every approach through production entry routing and real input.
        for(const auto& a:WorldLayout::Approaches) {
            WorldPreviewTestAccess::Action(game,a.boss);
            check(game.WorldRegion()==a.region.id,"Boss action skipped approach");
            const float width=a.region.width;
            settle();check(WorldPreviewTestAccess::Alive(game)==0&&WorldPreviewTestAccess::Pending(game)==0,"Patrol present before advancement");
            capture(std::string(a.region.id)+"-entry");
            WorldPreviewTestAccess::Place(game,{width*.22F,605});
            key(KEY_F,true);for(int i=0;i<75;++i)game.Update(1.0F/60);key(KEY_F,false);
            check(WorldPreviewTestAccess::Note(game),"Approach furnishing investigation failed");
            settle();capture(std::string(a.region.id)+"-investigation");
            WorldPreviewTestAccess::Place(game,{width*.31F,605});game.Update(1.0F/60);
            check(WorldPreviewTestAccess::Pending(game)>0&&WorldPreviewTestAccess::Alive(game)==0,"Patrol bypassed red gate delay");
            for(int i=0;i<80;++i)game.Update(1.0F/60);
            check(WorldPreviewTestAccess::Alive(game)==1,"Red gate did not spawn first patrol");
            capture(std::string(a.region.id)+"-patrol");
            // Gunfire must actually defeat a patrol member, not just display effects.
            if(std::string_view(a.boss)=="bridge") {
                key(KEY_J,true);
                for(int i=0;i<1200&&(WorldPreviewTestAccess::Pending(game)>0||WorldPreviewTestAccess::Alive(game)>0);++i) {
                    key(KEY_R,i%240==200);game.Update(1.0F/60);
                }
                key(KEY_R,false);key(KEY_J,false);
                check(WorldPreviewTestAccess::Pending(game)==0&&WorldPreviewTestAccess::Alive(game)==0,"Approach gunfire failed");
                capture("approach-actual-gunfire");
            }
            WorldPreviewTestAccess::Place(game,{width-105,605});
            game.Update(1.0F/60);settle();
            check(game.WorldRegion()==a.region.id,"Uncleared approach admitted boss entrance");
            for(int i=0;i<1200;++i)WorldPreviewTestAccess::TickSafe(game);
            settle();check(game.WorldRegion()==a.boss,"Cleared approach failed to enter boss area");
            capture(std::string(a.region.id)+"-boss-arrival");
            ProgressSystem approachSave(Utf8Path(GetApplicationDirectory()));
            check(!approachSave.RegionVisible(a.region.id)&&!approachSave.RegionAvailable(a.region.id),"Approach leaked into stage selection");
            if(std::string_view(a.boss)!="prts") {
                WorldPreviewTestAccess::Place(game,{110,605});press(KEY_F);settle();
                check(game.WorldRegion()==a.region.id&&WorldPreviewTestAccess::Cleared(game),"Boss return resurrected cleared patrols");
            }
        }
        std::cout<<"Mouse selection/drag, all 45 directed exits, automatic main travel, attack-key side passages, held clue investigations, fades and return passed\n";
        }
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    CloseAudioDevice();CloseWindow();return result;
}
