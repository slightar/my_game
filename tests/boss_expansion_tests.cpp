#include "story_boss.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
Rectangle Body(Vector2 p){return {p.x-12,p.y-72,24,72};}
Vector2 Follow(const StoryBoss& boss) {
    const float direction=boss.Feet().x<(boss.Left()+boss.Right())*.5F?1.0F:-1.0F;
    return {boss.Feet().x+direction*360,GameConfig::kFloorY};
}
void Reach(StoryBoss& boss,StoryBossState state,float frame) {
    for(int i=0;i<30000;++i) {
        if(boss.State()==state)return;
        boss.Update(frame,Follow(boss));
    }
    throw std::runtime_error("Unreachable expanded skill: "+std::to_string(int(state)));
}
void Advance(StoryBoss& boss,float duration,Vector2 player,float frame) {
    while(duration>.00001F){const float dt=std::min(frame,duration);boss.Update(dt,player);duration-=dt;}
}
void SecondLife(StoryBoss& boss,float frame) {
    boss.Damage(10000);Advance(boss,boss.RevivalDuration()+3.2F,Follow(boss),frame);
    Check(boss.PhaseTwo()&&boss.Targetable(),"Second-life fixture was still protected");
}
}
int main(){try {
    for(float frame:{1.0F/120,1.0F/60,1.0F/30,.1F}) {
        StoryBoss boss;Vector2 source;BossImpact impact;
        boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
        Reach(boss,StoryBossState::MirrorWindup,frame);
        const auto echoes=boss.CustomCrownslayer().Echoes();
        Check(echoes.size()==2,"Mirror skill lost one of its flanks");
        Check(!boss.CustomCrownslayer().CanDealContactDamage(),"Mirror preparation dealt contact damage");
        const auto origin=boss.Position();
        Advance(boss,.3F,{1100,400},frame);
        Check(boss.CustomCrownslayer().Echoes()[0].position.x==echoes[0].position.x&&boss.Position().x==origin.x,
              "Mirror origins tracked after their warning");
        while(boss.State()==StoryBossState::MirrorWindup)boss.Update(frame,{1100,400});
        Check(boss.CustomCrownslayer().DartCount()==6,"Mirror did not launch six crossing knives");
        const auto knife=echoes.front().position;
        const Rectangle knifeBody{knife.x-65,knife.y-65,130,130};
        Check(boss.AttackHits(knifeBody,knifeBody,source),"Mirror knives had no real damage");
        Check(boss.DestroyProjectile(knife,80),"Mirror knives could not be intercepted");

        Reach(boss,StoryBossState::AmbushWindup,frame);
        const auto plan=boss.CustomCrownslayer().Echoes();
        Check(plan.size()==3&&plan.front().delay>1.09F,"Ambush lacked three readable warnings");
        for(const auto& echo:plan)Check(!boss.AttackHits(Body(echo.position),Body(echo.position),source),"Ambush warning dealt damage");
        int hits=0;
        for(int i=0;i<int(2.15F/frame)+1;++i) {
            boss.Update(frame,{2400,GameConfig::kFloorY});
            for(const auto& echo:plan)hits+=boss.AttackHits(Body(echo.position),Body(echo.position),source)?1:0;
        }
        Check(hits==3,"Ambush did not deal one hit per echo");
        Advance(boss,.1F,{2400,GameConfig::kFloorY},frame);
        for(const auto& echo:plan)Check(!boss.AttackHits(Body(echo.position),Body(echo.position),source),"Ambush afterglow retained damage");
        boss.Damage(10000);Check(boss.CustomCrownslayer().Echoes().empty(),"Crown echoes survived defeat");

        for(bool phaseTwo:{false,true}) {
            boss.StartSandbox(StoryBossKind::W,2900,1900);
            if(phaseTwo)boss.Damage(56);
            Reach(boss,StoryBossState::AirburstWindup,frame);
            std::vector<BossBlast> air;
            for(const auto& blast:boss.Blasts())if(blast.airburst)air.push_back(blast);
            Check(air.size()==(phaseTwo?6:4),"Airburst formation did not strengthen at half health");
            const auto locked=boss.Aim();Advance(boss,.3F,{2500,400},frame);
            Check(boss.Aim().x==locked.x,"Airburst retargeted after its warning");
            bool highHit=false,lowHit=false;
            for(int i=0;i<int(2.8F/frame)+1;++i) {
                boss.Update(frame,{2500,GameConfig::kFloorY});
                for(const auto& blast:boss.Blasts())if(blast.airburst&&blast.timer<=0&&!blast.consumed) {
                    if(blast.position.y<GameConfig::kFloorY-100) {
                        Check(!boss.AttackHits(Body({blast.position.x,GameConfig::kFloorY}),Body({blast.position.x,GameConfig::kFloorY}),source),
                              "High airburst hit a grounded player");
                        highHit|=boss.AttackHits(Body(blast.position),Body(blast.position),source);
                    } else lowHit|=boss.AttackHits(Body(blast.position),Body(blast.position),source);
                }
            }
            Check(highHit&&lowHit,"Airburst lacked real damage at both heights");
            Reach(boss,StoryBossState::RemoteMineWindup,frame);
            while(boss.State()==StoryBossState::RemoteMineWindup)boss.Update(frame,{2500,GameConfig::kFloorY});
            Check(boss.Mines().size()==(phaseTwo?4:3),"Remote mine count was incorrect");
            const auto removed=boss.Mines().front().position;
            Bullet bullet;bullet.position={removed.x+60,removed.y-18};bullet.damage=2;bullet.radius=4;bullet.lifetime=1;
            Check(boss.ResolveBullet(bullet,{removed.x-60,removed.y-18}),"A swept bullet could not disarm a remote mine");
            boss.Update(frame,{2500,400});
            Check(std::none_of(boss.Mines().begin(),boss.Mines().end(),[&](const auto& m){return m.position.x==removed.x;}),"Disarmed mine remained armed");
            bool mineHit=false;
            for(int i=0;i<int(3.0F/frame)+1;++i) {
                boss.Update(frame,{2500,400});
                for(const auto& blast:boss.Blasts())if(blast.timer<=0&&!blast.consumed)
                    mineHit|=boss.AttackHits(Body(blast.position),Body(blast.position),source);
            }
            Check(mineHit,"Remote mine chain never exploded");
            Check(std::none_of(boss.Blasts().begin(),boss.Blasts().end(),[&](const auto& b){return b.position.x==removed.x;}),"A disarmed mine exploded later");
            boss.Damage(10000);Check(boss.Mines().empty()&&boss.Blasts().empty(),"W expansion survived defeat");

            boss.StartSandbox(StoryBossKind::FrostNova,2900,1900);
            if(phaseTwo)SecondLife(boss,frame);
            Reach(boss,StoryBossState::PrismWindup,frame);
            std::vector<BossZone> prisms;
            for(const auto& z:boss.Zones())if(z.kind==BossFx::FrostPrism)prisms.push_back(z);
            Check(prisms.size()==(phaseTwo?3:2)&&prisms.front().delay>1.5F,"Prisms lacked delayed, phase-scaled origins");
            const auto removedPrism=prisms.front().position;
            Check(boss.DestroyProjectile(removedPrism,30),"Sword pressure could not shatter a charging prism");
            Advance(boss,.25F,{2400,400},frame);
            const auto& active=*std::find_if(boss.Zones().begin(),boss.Zones().end(),[](const auto& z){return z.kind==BossFx::FrostPrism;});
            Check(active.target.x==prisms.back().target.x,"Prism aiming followed player movement");
            bool cold=false;
            for(int i=0;i<int(2.5F/frame)+1;++i) {
                boss.Update(frame,{2400,400});
                for(const auto& m:boss.Missiles())if(m.kind==BossFx::FrostBolt&&m.life>0)
                    if(boss.AttackHits(Body(m.position),Body(m.position),source,&impact)){cold|=impact.cold;break;}
            }
            Check(cold,"Prism bolts did not inflict cold");
            Reach(boss,StoryBossState::ThornWindup,frame);
            std::vector<BossZone> thorns;
            for(const auto& z:boss.Zones())if(z.kind==BossFx::FrostThorn)thorns.push_back(z);
            Check(!thorns.empty()&&thorns.front().delay>1.15F,"Thorns had no reaction window");
            bool thornHit=false;
            for(int i=0;i<int(3.0F/frame)+1;++i) {
                boss.Update(frame,{2400,400});
                for(const auto& z:boss.Zones())if(z.kind==BossFx::FrostThorn&&z.delay<=0&&!z.consumed) {
                    thornHit|=boss.AttackHits(Body(z.position),Body(z.position),source,&impact)&&impact.cold;
                    break;
                }
            }
            Check(thornHit,"Thorns had no cold damage");
            boss.Damage(10000);if(!phaseTwo){Advance(boss,7.2F,{2400,400},frame);boss.Damage(10000);}
            Check(boss.Defeated()&&boss.Missiles().empty()&&boss.Zones().empty(),"Frost expansion survived defeat");

            boss.StartSandbox(StoryBossKind::Patriot,2900,1900);
            if(phaseTwo)SecondLife(boss,frame);
            Reach(boss,StoryBossState::QuakeWindup,frame);
            Check(std::count_if(boss.Zones().begin(),boss.Zones().end(),[](const auto& z){return z.kind==BossFx::PatriotQuake;})==3,
                  "Quake did not telegraph three widening pulses");
            const auto quakeCenter=boss.Feet();int quakeHits=0;
            for(int i=0;i<int(3.0F/frame)+1;++i) {
                boss.Update(frame,{quakeCenter.x+650,GameConfig::kFloorY});
                const Rectangle airborne{quakeCenter.x-8,GameConfig::kFloorY-270,16,60};
                Check(!boss.AttackHits(airborne,airborne,source),"Quake could not be jumped");
                if(boss.AttackHits(Body(quakeCenter),Body(quakeCenter),source,&impact)) {
                    ++quakeHits;Check(impact.damage==(phaseTwo?2:1),"Quake lost its phase damage scaling");
                }
            }
            Check(quakeHits==3,"Quake did not deliver exactly three pulses");
            Reach(boss,StoryBossState::LanceWindup,frame);
            const auto lanceAim=boss.Aim();Advance(boss,.3F,{2300,400},frame);
            Check(boss.Aim().x==lanceAim.x,"Lance changed direction after warning");
            while(boss.State()==StoryBossState::LanceWindup)boss.Update(frame,{2300,400});
            const auto spear=std::find_if(boss.Missiles().begin(),boss.Missiles().end(),[](const auto& m){return m.kind==BossFx::SpearLance;});
            Check(spear!=boss.Missiles().end(),"Lance had no real projectile");
            const auto point=spear->position;
            Check(boss.AttackHits(Body(point),Body(point),source,&impact)&&impact.damage==2,"Lance did not deal two damage");
            boss.Damage(10000);if(!phaseTwo){Advance(boss,8.6F,{2400,400},frame);boss.Damage(10000);}
            Check(boss.Defeated()&&boss.Missiles().empty()&&boss.Zones().empty(),"Patriot expansion survived defeat");
        }
    }
    StoryBoss transition;
    transition.StartSandbox(StoryBossKind::W,2900,1900);
    Reach(transition,StoryBossState::RemoteMineWindup,1.0F/60);
    transition.Damage(56);
    Check(transition.PhaseTwo()&&transition.RemoteMineCount()==3,"Half-health transition added an unannounced remote mine");
    while(transition.State()==StoryBossState::RemoteMineWindup)transition.Update(1.0F/60,Follow(transition));
    Check(transition.Mines().size()==3,"Deployment did not match the mine warning shown before half health");
    std::cout<<"Eight expanded skills: reachability, locking, damage, interception, disarming, phase scaling and cleanup passed at 120/60/30/10 Hz\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
