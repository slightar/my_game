#include "story_boss.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
constexpr float Step=1.0F/120;
void Tick(StoryBoss& b,float time,Vector2 p){for(float t=0;t<time;t+=Step)b.Update(Step,p);}
void Start(StoryBoss& b,StoryBossKind kind){b.StartSandbox(kind,2600,1600);}
void Reach(StoryBoss& b,StoryBossState state,Vector2 p){
    for(int i=0;i<6000&&b.State()!=state;++i)b.Update(Step,p);
    if(b.State()!=state)throw std::runtime_error("Expected winter skill "+std::to_string(int(state))+" never started; boss="+std::to_string(int(b.Kind()))+" state="+std::to_string(int(b.State())));
}
int Seals(const StoryBoss& b){return int(std::count_if(b.Zones().begin(),b.Zones().end(),[](auto& z){return z.kind==BossFx::BlackIce&&z.remaining>0;}));}
Rectangle Body(float x,float feet=GameConfig::kFloorY){return {x-19,feet-78,38,78};}
void PhaseTwo(StoryBoss& b,Vector2 p){
    b.Damage(10000);Check(b.State()==StoryBossState::Revive&&!b.Defeated()&&!b.PhaseTwo(),"First death cleared encounter");
    Check(!b.Targetable()&&!b.Damage(10000),"Reviving Boss took damage");
    Tick(b,b.RevivalDuration()-.1F,p);Check(!b.PhaseTwo(),"Revival completed prematurely");
    Tick(b,.12F,p);Check(b.PhaseTwo()&&b.Health()==b.MaxHealth(),"Revival failed to refill second life");
    Check(!b.Targetable()&&b.Invulnerability()>0&&!b.Damage(1),"Second-phase guard missing");
    Tick(b,b.Invulnerability()+.05F,p);Check(b.Targetable(),"Revival guard never expired");
}
int main(int argc,char** argv){try {
    StoryBoss b;Vector2 source{};BossImpact impact{};
    b.Enter("ice",2600);Tick(b,8,{200,605});
    Check(b.FrostWeather()==0&&b.WeatherTime()==0,"Snow began before the encounter trigger");
    Check(b.TryStart(b.TriggerX()),"Weather fixture did not enter the encounter");
    Tick(b,1,{1300,605});
    Check(b.FrostWeather()>.20F&&b.FrostWeather()<.30F,"Encounter weather did not build gradually");
    Tick(b,2.2F,{1300,605});
    Check(std::abs(b.FrostWeather()-.72F)<.001F,"First-phase weather failed to settle");
    b.Damage(10000);const float beforeRevive=b.FrostWeather();Tick(b,1,{1300,605});
    Check(b.FrostWeather()>=beforeRevive,"Snow vanished during revival");
    Tick(b,b.RevivalDuration()+2,{1300,605});
    Check(b.PhaseTwo()&&b.FrostWeather()>.99F,"Second-phase storm did not intensify");
    Tick(b,3,{1300,605});b.Damage(10000);Tick(b,1,{1300,605});
    Check(b.Defeated()&&b.FrostWeather()>.7F&&b.FrostWeather()<.95F,"Defeat cut the weather abruptly");
    Tick(b,6,{1300,605});Check(b.FrostWeather()==0,"Defeated weather never faded away");
    b.Enter("industry",2600);Check(b.FrostWeather()==0,"Snow leaked into another region");
    for(float frame:{1.0F/120,1.0F/60,1.0F/30,.1F}) {
        b.Reset();Start(b,StoryBossKind::FrostNova);
        Reach(b,StoryBossState::FrostStepWindup,{220,605});
        const auto from=b.Feet();const float destination=b.MobilityTarget();
        Tick(b,.7F,{2350,605});
        Check(b.State()==StoryBossState::FrostStepWindup&&b.Feet().x==from.x&&b.MobilityTarget()==destination,
              "Frost step moved early or followed the player after warning");
        Reach(b,StoryBossState::FrostStep,{2350,605});
        for(int i=0;i<200&&b.State()==StoryBossState::FrostStep;++i) {
            b.Update(frame,{2350,605});
            Check(b.Feet().x>=b.Left()+35&&b.Feet().x<=b.Right()-35,"Frost step left its arena");
        }
        Check(std::abs(b.Feet().x-from.x)>180&&std::abs(b.Feet().x-destination)<.01F&&
              b.State()==StoryBossState::FrostFanWindup,"Frost step failed to reposition before its blade warning");
        const auto fanAim=b.Aim();Tick(b,.8F,{220,605});
        Check(b.State()==StoryBossState::FrostFanWindup&&b.Aim().x==fanAim.x,"Frost step fired instantly or retargeted its blades");
        Reach(b,StoryBossState::Recover,{220,605});
        Check(std::count_if(b.Missiles().begin(),b.Missiles().end(),[](const auto& m){return m.kind==BossFx::FrostBolt;})>=5,
              "Frost step lost its follow-up fan");

        b.Reset();Start(b,StoryBossKind::Patriot);
        Reach(b,StoryBossState::LeapWindup,{220,605});
        const float start=b.Feet().x,landing=b.MobilityTarget();
        Tick(b,.9F,{2350,605});
        Check(b.State()==StoryBossState::LeapWindup&&b.Feet().x==start&&b.MobilityTarget()==landing,
              "Patriot leap lost its fixed warning");
        Reach(b,StoryBossState::Leap,{2350,605});
        float highest=GameConfig::kFloorY;
        for(int i=0;i<200&&b.State()==StoryBossState::Leap;++i) {
            b.Update(frame,{2350,605});highest=std::min(highest,b.Feet().y);
            Check(b.Feet().x>=b.Left()+45&&b.Feet().x<=b.Right()-45,"Patriot leap left the arena");
        }
        Check(highest<GameConfig::kFloorY-210&&std::abs(b.Feet().x-start)>300&&
              std::abs(b.Feet().x-landing)<.01F&&b.Feet().y==GameConfig::kFloorY,
              "Patriot leap failed to cross the gap and land on its mark");
        Check(!b.AttackHits(Body(landing,GameConfig::kFloorY-175),Body(landing,GameConfig::kFloorY-175),source,&impact),
              "Airborne player could not avoid the landing shock");
        Check(b.AttackHits(Body(landing),Body(landing),source,&impact),"Patriot landing had no damage");
        Check(b.State()==StoryBossState::Recover,"Patriot leap offered no recovery window");
    }
    b.Reset();Start(b,StoryBossKind::Patriot);Reach(b,StoryBossState::Leap,{220,605});
    Tick(b,.2F,{220,605});b.Damage(1,.5F);
    Check(b.State()==StoryBossState::Recover&&b.Feet().y==GameConfig::kFloorY,"Interrupted leap left Patriot suspended");
    b.Reset();Start(b,StoryBossKind::Patriot);Reach(b,StoryBossState::Leap,{220,605});
    Tick(b,.2F,{220,605});b.Damage(10000);
    Check(b.State()==StoryBossState::Revive&&b.Feet().y==GameConfig::kFloorY&&b.Missiles().empty(),"Midair defeat retained leap hazards");
    for(auto kind:{StoryBossKind::FrostNova,StoryBossKind::Patriot}){
        b.Reset();b.Enter(StoryBossRegion(kind),2600);
        Check(b.TwoLives()&&!b.Active()&&!b.TryStart(b.TriggerX()-1),"Winter encounter started early");
        Check(b.TryStart(b.TriggerX())&&!b.Targetable(),"Arrival protection missing");
        Tick(b,1.7F,{1100,605});Check(b.Targetable(),"Winter arrival never ended");
        PhaseTwo(b,{1000,605});
        b.Damage(10000);Check(b.Defeated()&&b.Missiles().empty()&&b.Zones().empty()&&b.Visuals().empty(),"Final death retained live hazards");
        b.Enter("gray",2600);b.Enter(StoryBossRegion(kind),2600);
        Check(b.Defeated()&&!b.TryStart(2300),"Cleared winter Boss resurrected on revisit");
        b.Reset();Start(b,kind);Check(!b.PhaseTwo()&&b.Health()==b.MaxHealth(),"Retry retained second-phase state");
    }
    b.Reset();Start(b,StoryBossKind::FrostNova);b.Damage(1,5);
    Check(b.StunTime()==0,"FrostNova lost original stun immunity");
    const Vector2 p{1300,605};
    Reach(b,StoryBossState::FrostCast,p);Tick(b,.82F,p);
    Check(!b.Missiles().empty(),"Frost basic attack emitted no ice projectile");
    bool coldHit=false;for(int i=0;i<180&&!coldHit;++i){b.Update(Step,p);coldHit=b.AttackHits(Body(p.x),Body(p.x),source,&impact);}
    Check(coldHit&&impact.cold&&impact.damage==1,"Frost projectile lost cold damage");
    Reach(b,StoryBossState::IceRingWindup,p);
    Check(!b.AttackHits(Body(b.Feet().x),Body(b.Feet().x),source,&impact),"Ice-ring warning hurt player");
    Reach(b,StoryBossState::IceRing,p);
    Check(!b.AttackHits(Body(b.Feet().x,GameConfig::kFloorY-160),Body(b.Feet().x,GameConfig::kFloorY-160),source,&impact),"Jump did not avoid ground ice ring");
    Check(b.AttackHits(Body(b.Feet().x+230),Body(b.Feet().x+230),source,&impact)&&impact.shatter&&impact.cold,"Ice ring did not mark frozen damage multiplier");
    Reach(b,StoryBossState::BlackIceWindup,p);Check(Seals(b)==2,"First-life black ice seal count");
    auto z=*std::find_if(b.Zones().begin(),b.Zones().end(),[](auto& v){return v.kind==BossFx::BlackIce;});
    Check(z.delay>2.3F&&b.SealedFloor().empty(),"Ground sealed during warning");
    Tick(b,1.5F,p);
    Check(b.SealedFloor().empty(),"Ground sealed before the readable 2.4 second warning ended");
    Tick(b,1.0F,p);
    Check(b.SealedFloor().size()==2&&b.SealedGroundAt(z.position.x),"Black ice did not seal two actual floor segments");
    Check(!b.SealedGroundAt(b.SafeGroundX(z.position.x)),"Sealed floor rescue leads to another sealed tile");
    Check(!b.AttackHits(Body(z.position.x),Body(z.position.x),source,&impact),"Ground seal still acts like a contact-damage crystal");
    PhaseTwo(b,{750,605});Check(Seals(b)==2,"Revival removed persistent black ice");
    b.Damage(1,5);Check(b.StunTime()==0,"Second-life FrostNova is stunnable");
    Reach(b,StoryBossState::IceRing,{750,605});
    Check(b.AttackHits(Body(b.Feet().x+360),Body(b.Feet().x+360),source,&impact)&&impact.shatter,"Second-life ice ring did not expand");
    Reach(b,StoryBossState::BlackIceWindup,{750,605});Check(Seals(b)==5,"Second-life black ice did not add three seals");
    bool wave=false,left=false,right=false,pincer=false;
    for(int i=0;i<3500;++i){
        b.Update(Step,{950,605});int newWaves=0;
        for(const auto& m:b.Missiles())if(m.kind==BossFx::ColdWave){
            wave=true;left|=m.velocity.x>0;right|=m.velocity.x<0;
            if(m.age<.05F)++newWaves;
        }
        pincer|=newWaves>=2;
    }
    Check(wave&&left&&right&&pincer&&Seals(b)<=6,"Phase-two ice crystal pincer or hazard cap failed");

    b.Reset();Start(b,StoryBossKind::FrostNova);
    Reach(b,StoryBossState::BlackIceWindup,p);
    Tick(b,2.5F,p);
    Check(Seals(b)==2,"Counterplay setup lost sealed tiles");
    const float sealX=b.SealedFloor().front().x+60;
    const float otherSealX=b.SealedFloor().back().x+60;
    for(int shot=0;shot<4;++shot) {
        Bullet bullet{{sealX-95,GameConfig::kFloorY-55},{1050,0},1,7.5F,1};
        const Vector2 before=bullet.position;bullet.position.x=sealX;
        Check(b.BreakSeal(bullet,before)&&bullet.hasHit,"Player attack did not damage the sealed floor");
        if(shot<3)Check(b.SealedGroundAt(sealX),"Sealed floor broke before four hits");
    }
    Check(!b.SealedGroundAt(sealX)&&Seals(b)==1,"Four attacks failed to restore ground support");
    Tick(b,19.0F,{220,605});
    Check(!b.SealedGroundAt(otherSealX),"Sealed floor never thawed after its duration");
    b.Reset();Start(b,StoryBossKind::FrostNova);
    bool volley=false;
    for(int i=0;i<10000&&!volley;++i) {
        const auto before=b.State();b.Update(Step,{b.Feet().x-360,605});
        if(before==StoryBossState::FrostCast&&b.State()==StoryBossState::Recover&&b.Missiles().size()>=3)
            volley=true;
    }
    Check(volley,"FrostNova never escalated to a three-blade volley");
    b.Reset();Start(b,StoryBossKind::FrostNova);
    Reach(b,StoryBossState::IceRainWindup,{1300,605});
    Check(!b.AttackHits(Body(1300),Body(1300),source,&impact),"Ice rain windup damaged player");
    Tick(b,1.1F,{1300,605});
    auto rain=std::find_if(b.Zones().begin(),b.Zones().end(),[](const BossZone& z){return z.kind==BossFx::IceRain;});
    Check(rain!=b.Zones().end()&&rain->delay>1.0F,"FrostNova did not mark delayed ice rain");
    const float rainX=rain->position.x;
    Tick(b,1.32F,{1300,605});
    Check(b.AttackHits(Body(rainX),Body(rainX),source,&impact)&&impact.cold&&!impact.shatter,
        "Ice rain did not land with cold damage");
    b.Damage(10000);Tick(b,b.RevivalDuration()+.05F,{1300,605});
    Reach(b,StoryBossState::IceRainWindup,{1300,605});Tick(b,1.1F,{1300,605});
    Check(std::count_if(b.Zones().begin(),b.Zones().end(),[](const BossZone& z){return z.kind==BossFx::IceRain;})==5,
        "Second-life ice rain did not cover five staggered lanes");
    b.Reset();Start(b,StoryBossKind::FrostNova);
    Reach(b,StoryBossState::IceRainWindup,{b.Left()+55,605});Tick(b,1.1F,{b.Left()+55,605});
    Check(std::count_if(b.Zones().begin(),b.Zones().end(),[](const BossZone& z){return z.kind==BossFx::IceRain;})==3,
        "Ice rain lost lanes when player stood at arena edge");

    b.Reset();Start(b,StoryBossKind::Patriot);
    Reach(b,StoryBossState::ChargeWindup,{b.Feet().x-450,605});
    Check(!b.AttackHits(Body(b.Feet().x),Body(b.Feet().x),source,&impact),"Charge warning damaged player");
    Reach(b,StoryBossState::Charge,{b.Feet().x-450,605});
    const float chargeX=b.Feet().x;Tick(b,.1F,{chargeX-450,605});
    Check(std::abs(b.Feet().x-chargeX)>25,"Patriot charge telegraphed but did not move");
    Check(!b.AttackHits(Body(b.Feet().x,GameConfig::kFloorY-170),Body(b.Feet().x,GameConfig::kFloorY-170),source,&impact),"Jump failed to dodge Patriot charge");
    Check(b.AttackHits(Body(b.Feet().x),Body(b.Feet().x),source,&impact)&&impact.damage==1,"Patriot charge has no contact pressure");
    for(int i=0;i<180&&std::none_of(b.Missiles().begin(),b.Missiles().end(),[](const BossMissile& m){return m.kind==BossFx::GroundWave;});++i)
        b.Update(Step,{chargeX-450,605});
    auto shock=std::find_if(b.Missiles().begin(),b.Missiles().end(),[](const BossMissile& m){return m.kind==BossFx::GroundWave;});
    Check(shock!=b.Missiles().end(),"Patriot charge did not release a ground wave");
    const float shockX=shock->position.x;
    Check(!b.AttackHits(Body(shockX,GameConfig::kFloorY-165),Body(shockX,GameConfig::kFloorY-165),source,&impact),
        "Jump did not avoid the ground wave");
    Check(b.AttackHits(Body(shockX),Body(shockX),source,&impact)&&impact.damage==1&&!impact.cold,
        "Ground wave did not hit a grounded player");
    Tick(b,1.5F,{b.Feet().x-100,605});
    b.Reset();Start(b,StoryBossKind::Patriot);
    for(int i=0;i<7000&&b.State()!=StoryBossState::GuardWindup;++i) {
        const float side=b.Feet().x<(b.Left()+b.Right())*.5F?1.0F:-1.0F;
        b.Update(Step,{b.Feet().x+side*400,605});
    }
    Check(b.State()==StoryBossState::GuardWindup,"Patriot never raised the frontal shield");
    Tick(b,.92F,{b.Feet().x-400,605});Check(b.State()==StoryBossState::Guard,"Patriot shield windup did not become active");
    const float shieldHealth=b.Health();
    Bullet frontal{{b.Feet().x+b.Facing()*110,b.Feet().y-85},{-b.Facing()*900.0F,0},1,8,4};
    const auto frontalPrevious=frontal.position;frontal.position.x=b.Feet().x;
    Check(b.ResolveBullet(frontal,frontalPrevious),"Frontal attack did not hit Patriot shield");
    const float frontalDamage=shieldHealth-b.Health();
    Bullet rear{{b.Feet().x-b.Facing()*110,b.Feet().y-85},{b.Facing()*900.0F,0},1,8,4};
    const auto rearPrevious=rear.position;rear.position.x=b.Feet().x;
    Check(b.ResolveBullet(rear,rearPrevious),"Rear attack did not hit Patriot");
    Check(b.Health()<shieldHealth-frontalDamage*3&&b.GuardIntegrity()<10,
        "Patriot shield does not reward flanking or break under fire");
    for(int i=0;i<4&&b.State()==StoryBossState::Guard;++i) {
        Bullet breakShot{{b.Feet().x+b.Facing()*110,b.Feet().y-85},{-b.Facing()*900.0F,0},1,8,4};
        const auto previous=breakShot.position;breakShot.position.x=b.Feet().x;
        Check(b.ResolveBullet(breakShot,previous),"Frontal shield-breaking shot missed");
    }
    Check(b.State()==StoryBossState::Recover,"Concentrated fire did not break Patriot's shield");
    b.Reset();Start(b,StoryBossKind::Patriot);
    b.Damage(10);b.Damage(10);b.Damage(10);
    Reach(b,StoryBossState::GuardWindup,{b.Feet().x-200,605});
    if(b.Elapsed()>=2.0F)throw std::runtime_error("Sustained damage did not provoke an early shield response: "+std::to_string(b.Elapsed()));
    b.Reset();Start(b,StoryBossKind::Patriot);
    float hp=b.Health();b.Damage(10,0,DamageType::Physical);Check(std::abs(hp-b.Health()-3.5F)<.01F,"Marching physical armor missing");
    hp=b.Health();b.Damage(10,0,DamageType::Arts);Check(std::abs(hp-b.Health()-6.5F)<.01F,"Marching arts resistance wrong");
    b.Damage(1,1);Check(b.StunTime()>0,"Marching Patriot should be stunnable");Tick(b,1,{b.Feet().x-100,605});
    Reach(b,StoryBossState::Combo,{b.Feet().x-100,605});Check(b.ComboCount()==4,"Marching four-hit combo missing");
    int hits=0;for(int i=0;i<170;++i){b.Update(Step,{b.Feet().x-100,605});if(b.AttackHits(Body(b.Feet().x-100),Body(b.Feet().x-100),source,&impact))++hits;}
    Check(hits==4,"Marching combo did not resolve four separate attacks");
    b.Damage(10000);Check(b.State()==StoryBossState::Revive,"Patriot skipped revival");
    Tick(b,1.1F,{1000,605});
    Check(!b.AttackHits(Body(b.Feet().x,GameConfig::kFloorY-100),Body(b.Feet().x,GameConfig::kFloorY-100),source,&impact),"Jump did not avoid revival sacrifice");
    Check(b.AttackHits(Body(b.Feet().x),Body(b.Feet().x),source,&impact)&&!impact.cold,"Patriot revival sacrifice dealt no damage");
    Tick(b,5.0F,{1000,605});Tick(b,2.6F,{1000,605});
    Check(b.PhaseTwo()&&b.Targetable(),"Patriot second life unavailable");
    hp=b.Health();b.Damage(10,5,DamageType::Physical);
    Check(std::abs(hp-b.Health()-7)<.01F&&b.StunTime()==0,"Destruction armor/stun immunity wrong");
    // Maintain ranged pressure while the new leap can close the original gap.
    float spearTarget=0;
    for(int i=0;i<6000&&b.State()!=StoryBossState::SpearWindup;++i) {
        spearTarget=b.Feet().x<1300?b.Feet().x+680:b.Feet().x-680;
        b.Update(Step,{spearTarget,605});
    }
    Check(b.State()==StoryBossState::SpearWindup,"Patriot mobility starved the aimed spear");
    Tick(b,1.15F,{spearTarget-50,605});const auto locked=b.Aim();
    Tick(b,.5F,{700,400});Check(b.Aim().x==locked.x&&b.Aim().y==locked.y,"Spear tracked after final lock");
    Reach(b,StoryBossState::Spear,{700,400});
    bool spearHit=false;for(int i=0;i<180&&!spearHit;++i){b.Update(Step,{700,400});BossImpact candidate{};if(b.AttackHits(Body(locked.x),Body(locked.x),source,&candidate)&&candidate.damage==2)spearHit=true;}
    Check(spearHit,"Locked spear projectile missed actual hitbox/damage");
    Reach(b,StoryBossState::Combo,{b.Feet().x-170,605});Check(b.ComboCount()==1,"Destruction did not replace marching combo");
    if(argc==2){
        const std::filesystem::path root=argv[1];nlohmann::json data;std::ifstream in(root/"manifest.json");in>>data;
        for(const char* id:{"frostnova","patriot"}){
            const auto& a=data.at(id);Check(a.at("source").at("skel_sha256").get<std::string>().size()==64,"Original winter skeleton provenance missing");
            Check(a.at("model")==a.at("source").at("model")&&std::filesystem::exists(root/(std::string(id)+".png")),"Original winter model absent");
        }
        Check(data.at("patriot").at("animations").at(3)=="Idle_2"&&data.at("frostnova").at("model")=="enemy_1510_frstar2","Wrong original second-phase/Winter Scar model");
        nlohmann::json smooth;std::ifstream anim(root/"frostnova_smooth.json");anim>>smooth;
        Check(smooth.at("fps").get<int>()>=30&&smooth.at("animations").at("Idle").at("frames").size()>=120&&
            smooth.at("animations").at("Skill_2").at("frames").size()>=200&&
            std::filesystem::exists(root/"frostnova_smooth.png"),"30 Hz original Spine animation atlas missing");
        std::ifstream fx(root/"effects/manifest.json");nlohmann::json effects;fx>>effects;
        for(const auto& [id,a]:effects.at("textures").items())Check(std::filesystem::exists(root/"effects"/a.at("file").get<std::string>()),"Client effect texture missing");
        for(const char* file:{"frost_phase1.ogg","frost_phase2.ogg","patriot.ogg","ice_ring.wav","ice_seal.wav"})Check(std::filesystem::file_size(root/"audio"/file)>10000,"Original winter audio missing");
    }
    std::cout<<"Winter double lives, guard, armor, cold, expanding rings, sealed floor, 30 Hz Spine, four strikes, ritual and locked spear passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
