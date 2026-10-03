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
struct AudioSystemTestAccess {
    static double Last(const AudioSystem& audio,AudioCue cue){return audio.clips_[static_cast<int>(cue)].lastPlayed;}
    static void Age(AudioSystem& audio){for(auto& clip:audio.clips_)clip.lastPlayed=-100;}
    static bool Playing(const AudioSystem& audio){for(const auto& clip:audio.clips_)for(int i=0;i<clip.voiceCount;++i)if(IsSoundPlaying(clip.voices[i]))return true;return false;}
};
struct WorldPreviewTestAccess {
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
        SetMasterVolume(0); // Automated runs retain playback, without audible test bursts.
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
        if(argc>1&&std::string_view(argv[1])=="--exploration-attack-audit") {
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
