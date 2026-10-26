#include "story_boss.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cmath>

void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Tick(StoryBoss& boss,float seconds,Vector2 player){for(float time=0;time<seconds;time+=1.0F/120)boss.Update(1.0F/120,player);}
void Start(StoryBoss& boss,const char* region) {
    boss.Enter(region,2900);Check(boss.TryStart(boss.TriggerX()),"Boss trigger did not start");
    Tick(boss,.8F,{boss.TriggerX()+80,605});
}
int main(int argc,char** argv){try {
    StoryBoss boss;
    boss.Enter("gray",2900);Check(!boss.Present()&&!boss.TryStart(1800),"Exploration scene created Boss");
    boss.Enter("bridge",2900);
    Check(boss.Present()&&!boss.Active()&&!boss.Targetable(),"Boss spawned before arena trigger");
    Check(!boss.TryStart(boss.TriggerX()-1)&&!boss.Damage(1000),"Dormant Boss took damage or started early");
    Check(boss.TryStart(boss.TriggerX())&&!boss.Targetable(),"Arrival not protected");
    Tick(boss,.8F,{boss.TriggerX()+80,605});
    Check(boss.Targetable()&&boss.Health()==60,"Crownslayer did not arrive with full health");
    // Every attack uses the exact B-key controller, including positions and timing.
    StoryBoss sandbox;sandbox.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    Boss reference;reference.Reset({80,GameConfig::kBossArena.y,2740,GameConfig::kBossArena.height},{1900,GameConfig::kFloorY-34});
    bool rush=false,air=false,darts=false,shadow=false;
    Vector2 source{};
    for(int i=0;i<1800;++i) {
        const Vector2 target{reference.Position().x>1450?500.0F:2400.0F,605};
        reference.Update(1.0F/120,target,-1);sandbox.Update(1.0F/120,target,-1);
        const auto& actual=sandbox.CustomCrownslayer();
        Check(actual.CurrentState()==reference.CurrentState()&&std::abs(actual.Position().x-reference.Position().x)<.001F&&
            std::abs(actual.Position().y-reference.Position().y)<.001F&&actual.DartCount()==reference.DartCount(),"Sandbox diverged from B-key controller");
        rush|=actual.CurrentState()==Boss::State::Rush;
        air|=actual.CurrentState()==Boss::State::AirSlash;
        darts|=actual.DartCount()==3;
        shadow|=actual.CurrentState()==Boss::State::ShadowFinal;
    }
    Check(rush&&air&&darts,"Custom Crownslayer lost rush, aerial slash or three-dart volley");
    Check(shadow,"B-key shared controller did not reach its signature aerial finisher");
    // Both facing directions must teleport behind the player after half health.
    for(int facing:{-1,1}) {
        sandbox.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
        sandbox.Damage(31);
        for(int i=0;i<100&&sandbox.State()!=StoryBossState::Appear;++i)sandbox.Update(1.0F/120,{1000,605},facing);
        Check(sandbox.State()==StoryBossState::Appear&&std::abs(sandbox.Position().x-(1000-facing*150))<.01F,"Half-health rush did not teleport behind player");
    }
    // The same controller must also drive actual story encounters after arrival.
    bool storyRush=false,storyAir=false,storyDarts=false;
    for(int i=0;i<1800;++i) {
        boss.Update(1.0F/120,{boss.Position().x>1450?500.0F:2400.0F,605});
        storyRush|=boss.State()==StoryBossState::Rush;
        storyAir|=boss.State()==StoryBossState::AirSlash;
        if(boss.CustomCrownslayer().DartCount()>0) {
            storyDarts=true;
            const auto count=boss.CustomCrownslayer().DartCount();
            Check(boss.AttackHits({-500,-500,1,1},{0,0,3000,1000},source),"Custom darts did not damage projectile hitbox");
            Check(boss.CustomCrownslayer().DartCount()==count-1,"Hit dart was not consumed");
            if(count>1)Check(boss.DestroyProjectile(boss.Position(),4000),"Sword wave could not destroy custom darts");
        }
    }
    Check(storyRush&&storyAir&&storyDarts,"Story encounter still uses simplified Crownslayer");
    // Keep melee and contact collision behavior identical to the B-key version.
    sandbox.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    const Vector2 near{1820,610};
    for(int i=0;i<100&&sandbox.State()!=StoryBossState::Strike;++i)sandbox.Update(1.0F/120,near);
    const Rectangle body{1800,580,40,60};
    Check(sandbox.State()==StoryBossState::Strike&&sandbox.AttackHits(body,body,source),"Custom melee lost damage");
    sandbox.Damage(1,.5F);
    Check(!sandbox.AttackHits(body,body,source),"Stunned custom Crownslayer dealt contact or melee damage");
    Bullet slash;slash.position=boss.Position();slash.radius=50;slash.lifetime=1;slash.damage=4;slash.kind=BulletKind::MeleeSlash;
    const float hp=boss.Health();Check(boss.ResolveBullet(slash,slash.position)&&boss.Health()==hp-4,"Player melee did not damage Boss");
    Check(!boss.ResolveBullet(slash,slash.position),"One slash damaged Boss repeatedly");
    Bullet fast;fast.position={boss.Position().x+400,boss.Position().y};fast.radius=4;fast.lifetime=1;fast.damage=5;
    Check(boss.ResolveBullet(fast,{boss.Position().x-400,boss.Position().y}),"Fast projectile tunneled through Boss");
    Bullet finisher;finisher.kind=BulletKind::MeleeSlash;finisher.position=boss.Position();finisher.radius=40;finisher.damage=1000;finisher.lifetime=1;
    Check(boss.ResolveBullet(finisher,finisher.position)&&finisher.impactEvents.size()==1&&finisher.impactEvents.front().killed,"Final Boss death missing kill feedback");
    Check(boss.Defeated()&&boss.Grenades().empty()&&boss.Blasts().empty(),"Defeat retained attacks");
    boss.Enter("gray",2900);boss.Enter("bridge",2900);
    Check(boss.Defeated()&&!boss.TryStart(2000),"Returning resurrected cleared Boss");
    boss.Reset();Start(boss,"bridge");Check(!boss.Defeated()&&boss.Health()==60,"New action did not reset Boss");

    boss.Reset();Start(boss,"wtower");
    const Vector2 target{boss.Feet().x-310,605};
    Tick(boss,1.5F,target);
    Check(!boss.Grenades().empty()&&boss.Grenades().front().gravity>0&&boss.Grenades().front().position.y<GameConfig::kFloorY-68,"W did not launch arcing grenade");
    const auto grenade=boss.Grenades().front();
    Check(boss.DestroyProjectile(grenade.position,20),"Sword wave could not destroy grenade");
    Tick(boss,.9F,target);Check(boss.Grenades().empty()&&boss.Blasts().empty(),"Destroyed grenade still exploded");
    bool shellHit=false;
    for(int i=0;i<420&&!shellHit;++i) {
        boss.Update(1.0F/120,target);
        for(const auto& blast:boss.Blasts())if(!blast.c4&&blast.timer<=0) {
            shellHit=boss.AttackHits({target.x-19,566,38,78},{target.x-12,587,24,36},source);
            if(shellHit)break;
        }
    }
    Check(shellHit,"Grenade landing did not deal real damage");
    boss.Reset();Start(boss,"wtower");
    const Vector2 c4Target{boss.Feet().x-310,605};
    for(int i=0;i<1200&&boss.State()!=StoryBossState::C4;++i)boss.Update(1.0F/120,c4Target);
    Check(boss.State()==StoryBossState::C4,"W never cast C4");
    int count=0;for(const auto& b:boss.Blasts())if(b.c4)++count;
    Check(count==1,"Full-health W installed more than one C4");
    Tick(boss,1.0F,{c4Target.x+90,605});
    float tracked=0;for(const auto& b:boss.Blasts())if(b.c4)tracked=b.position.x;
    Check(std::abs(tracked-c4Target.x-90)<1,"C4 early marker did not track movement");
    Tick(boss,.8F,{c4Target.x+90,605});Tick(boss,.7F,{c4Target.x-90,605});
    for(const auto& b:boss.Blasts())if(b.c4)Check(std::abs(b.position.x-tracked)<1&&b.tracking==0,"C4 did not lock before detonation");
    Tick(boss,.8F,{tracked,605});
    Check(boss.AttackHits({tracked-19,566,38,78},{tracked-12,587,24,36},source),"C4 did not explode after its countdown");
    const auto c4Visuals=[&](){return std::count_if(boss.Visuals().begin(),boss.Visuals().end(),[](const auto& v){return v.kind==BossFx::WC4Explosion;});};
    Check(c4Visuals()==1,"C4 must emit its visual exactly once");
    Tick(boss,.4F,{tracked,605});
    Check(c4Visuals()==1,"C4 smoke disappeared with its damage window or emitted twice");
    for(const auto& blast:boss.Blasts())Check(!blast.c4,"C4 visual tail extended the damage window");
    Tick(boss,3.6F,{tracked,605});Check(c4Visuals()==0,"C4 smoke did not expire");
    boss.Reset();Start(boss,"wtower");
    Check(boss.Damage(56)&&boss.PhaseTwo(),"W did not change phase below half health");
    const Vector2 phaseTarget{boss.Feet().x-260,605};
    for(int i=0;i<300&&boss.State()!=StoryBossState::C4;++i)boss.Update(1.0F/120,phaseTarget);
    count=0;for(const auto& b:boss.Blasts())if(b.c4)++count;
    Check(count==3,"Half-health W did not install three C4 charges");
    boss.Damage(1,.5F);
    for(const auto& b:boss.Blasts())if(b.c4)Check(b.timer==0&&b.tracking==0,"Interrupt did not detonate C4");
    Check(boss.AttackHits({phaseTarget.x-19,566,38,78},{phaseTarget.x-12,587,24,36},source),"Interrupted C4 dealt no damage");
    finisher={};finisher.kind=BulletKind::MeleeSlash;finisher.position=boss.Position();finisher.radius=40;finisher.damage=1000;finisher.lifetime=1;
    Check(boss.ResolveBullet(finisher,finisher.position)&&finisher.impactEvents.size()==1&&finisher.impactEvents.front().killed,"Final Boss death missing kill feedback");
    Check(boss.Defeated()&&boss.Blasts().empty(),"W left live explosives after defeat");
    if(argc==2) {
        const std::filesystem::path root=argv[1];nlohmann::json data;std::ifstream in(root/"manifest.json");in>>data;
        nlohmann::json fx;std::ifstream timeline(root/"effects/timelines.json");timeline>>fx;
        Check(fx.at("fps")==60,"VFX sampling rate changed");
        for(const auto& [clip,layers]:fx.at("clips").items())for(const auto& layer:layers) {
            Check(std::filesystem::exists(root/"effects"/(layer.at("texture").get<std::string>()+".png")),"VFX timeline texture missing");
            const int tiles=layer.at("columns").get<int>()*layer.at("rows").get<int>();
            Check(layer.at("frames").size()>=2&&tiles>0,"Invalid VFX timeline");
            for(const auto& frame:layer.at("frames")) {
                Check(frame.size()==6&&frame[0].get<float>()>=0,"Invalid VFX size sample");
                for(int i=1;i<=4;++i)Check(frame[i].get<float>()>=0&&frame[i].get<float>()<=1,"Invalid VFX color sample");
                Check(frame[5].get<int>()>=0&&frame[5].get<int>()<tiles,"VFX flipbook samples outside atlas");
            }
        }
        for(const char* id:{"crownslayer","w"}) {
            const auto& m=data.at(id);
            Check(m.at("model")==m.at("source").at("model"),"Original Boss source identity mismatch");
            Check(m.at("source").at("skel_sha256").get<std::string>().size()==64,"Missing original skeleton hash");
            Check(std::filesystem::exists(root/(std::string(id)+".png")),"Missing original Boss animation sheet");
            Check(m.at("visible_height")==108,"Boss art out of operator proportions");
        }
    }
    std::cout<<"Custom Crownslayer B-key parity/rush/air/darts/half-health/melee, W grenades/C4/half-health/interrupt, swept hits, excursion progress and original models passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
