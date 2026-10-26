#include "adaptive_difficulty.h"
#include "enemy_system.h"
#include "story_boss.h"
#include "world_encounter.h"
#include <iostream>
#include <stdexcept>
#include <limits>

void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Time(AdaptiveDifficulty& d,float seconds,bool threats=true){for(int i=0;i<int(seconds*60);++i)d.Tick(1.0F/60,threats);}
void Win(AdaptiveDifficulty& d,float time,int loss=0,int deaths=0){d.Begin(30,6);Time(d,time);d.ConnectedHit();d.ConnectedHit();d.ObserveDamage(loss,deaths);d.Finish(true);}
int main(){try {
    AdaptiveDifficulty d;const float initial=d.Ability();const auto first=d.Profile();
    Time(d,80);d.ConnectedHit();d.ObserveDamage(5,1);
    Check(!d.Finish(true)&&d.Ability()==initial,"Exploration changed difficulty");
    d.Begin(30,6);Time(d,80,false);Check(d.Seconds()==0,"Wave-gap exploration counted as fighting");
    Time(d,12);const float seconds=d.Seconds();d.Begin(80,2);
    Check(d.Seconds()==seconds,"Reinforcement reset encounter metrics");
    d.ConnectedHit();d.ConnectedHit();
    Check(d.Profile().health==first.health,"Difficulty changed in the middle of a fight");
    d.Finish(true);Check(d.Ability()>initial&&d.Ability()-initial<=.08001F,"Fast clean win did not increase gradually");
    const float improved=d.Ability();Win(d,90,5,1);
    Check(d.Ability()<improved,"Painful slow victory did not reduce pressure");
    d.Begin(30,6);const float beforeFailure=d.Ability();d.Finish(false);
    Check(d.Ability()<beforeFailure&&!d.Finish(false),"Defeat did not lower difficulty once");
    const float afterFailure=d.Ability();d.Begin(30,6);Time(d,1);d.ConnectedHit();d.ConnectedHit();d.Finish(true);
    Check(d.Ability()==afterFailure,"Instant/debug clear raised difficulty");
    d.Begin(30,6);Time(d,10);d.Cancel();Check(!d.Finish(false)&&d.Ability()==afterFailure,"Abandoned battle counted as failure");
    for(int i=0;i<60;++i)Win(d,12);
    Check(d.Ability()<=1&&d.Ability()>.9F,"Success streak escaped cap or failed to progress");
    const auto high=d.Profile();
    for(int i=0;i<60;++i){d.Begin(30,6);d.Finish(false);}
    Check(d.Ability()==0&&d.Profile().health>=.9F,"Repeated failures escaped floor");
    Check(high.health<=1.55F&&high.movement<=1.25F&&high.recovery>=.70F&&high.spawnInterval>=.70F,"Unsafe parameter bounds");
    Check(std::isfinite(CombatDifficulty::FromAbility(std::numeric_limits<float>::quiet_NaN()).health),"Invalid estimate created NaN");
    Check(CombatDifficulty::FromAbility(std::numeric_limits<float>::quiet_NaN()).FormationTier()==1,
          "Invalid estimate created an unsafe roster");

    AdaptiveDifficulty progression;
    Check(WorldEncounter::Formation("approach_bridge",0,progression.Profile()).size()==4,
          "Initial battle did not keep standard roster");
    for(int i=0;i<3;++i)Win(progression,12);
    Check(WorldEncounter::Formation("approach_bridge",0,progression.Profile()).size()==5,
          "Actual clean wins failed to unlock additional troop roles");
    for(int i=0;i<5;++i)Win(progression,12);
    Check(WorldEncounter::Formation("approach_bridge",0,progression.Profile()).size()==6,
          "Sustained clean wins failed to increase wave size");
    for(int i=0;i<6;++i){progression.Begin(30,6);progression.Finish(false);}
    Check(WorldEncounter::Formation("approach_bridge",0,progression.Profile()).size()==3,
          "Repeated defeats failed to reduce troops");

    const auto low=CombatDifficulty::FromAbility(0),hard=CombatDifficulty::FromAbility(1);
    for(const char* region:{"bridge","wtower","ice","industry"}) {
        StoryBoss gentleBoss,hardBoss;
        gentleBoss.Enter(region,4000,low);hardBoss.Enter(region,4000,hard);
        Check(hardBoss.MaxHealth()>gentleBoss.MaxHealth(),"A story Boss ignored adaptive health");
        gentleBoss.TryStart(gentleBoss.TriggerX());hardBoss.TryStart(hardBoss.TriggerX());
        gentleBoss.Update(.25F,{1200,605});hardBoss.Update(.25F,{1200,605});
        Check(gentleBoss.State()==hardBoss.State()&&gentleBoss.StateTime()==hardBoss.StateTime(),
              "Adaptive health changed Boss arrival animation timing");
    }
    WorldEncounter patrol;
    EnemySystem waves;
    auto chosen=hard;
    const float width=WorldLayout::Find("approach_ice")->width;
    patrol.Enter("approach_ice",waves,chosen);
    patrol.Update(width*.29F,waves);
    Check(waves.Pending()==0,"Adaptive roster spawned before player advancement");
    patrol.Update(width*.34F,waves);
    Check(waves.Pending()==6,"World encounter failed to enqueue adaptive six-unit wave");
    waves.Update(.25F,{100,605});
    Check(waves.Units().empty(),"Adaptive roster removed the first-spawn warning");
    chosen=low;
    for(int frame=0;frame<2400&&!waves.Cleared();++frame) {
        waves.Update(1.0F/60,{100,605});
        for(const auto& unit:waves.Units())if(unit.Targetable())
            waves.Damage(unit.id,10000,DamageType::Arts,{unit.feet.x-100,605});
    }
    Check(waves.Cleared(),"First adaptive wave could not be completed");
    patrol.Update(width*.70F,waves);
    Check(waves.Pending()==6&&waves.Difficulty().health==hard.health,
          "Second wave changed frozen troop count or strength");
    patrol.Enter("approach_ice",waves,chosen);
    patrol.Update(width*.34F,waves);
    Check(waves.Pending()==3&&waves.Difficulty().health==low.health,
          "Retry ignored lower strength and troop count");
    EnemySystem easy,challenging;
    easy.ResetEncounter({EnemyKind::Soldier,EnemyKind::Soldier},1800,80,2700,low);
    challenging.ResetEncounter({EnemyKind::Soldier,EnemyKind::Soldier},1800,80,2700,hard);
    easy.Update(.25F,{900,605});challenging.Update(.25F,{900,605});
    for(int i=0;i<60;++i){easy.Update(1.0F/60,{900,605});challenging.Update(1.0F/60,{900,605});}
    Check(easy.Units().size()==1&&challenging.Units().size()==1,"First spawn telegraph changed");
    const auto& e=easy.Units().front();const auto& h=challenging.Units().front();
    Check(h.maxHealth>e.maxHealth&&h.health==h.maxHealth,"Scaled health or full-health bar incorrect");
    Check(h.feet.x<e.feet.x&&h.recoveryDuration<e.recoveryDuration,"Pursuit/recovery did not scale");
    for(int i=0;i<115;++i){easy.Update(1.0F/60,{900,605});challenging.Update(1.0F/60,{900,605});}
    Check(challenging.Pending()==0&&easy.Pending()==1,"Reinforcement timing did not scale");

    EnemySystem archerEasy,archerHard;
    archerEasy.ResetEncounter({},1800,80,2700,low);archerHard.ResetEncounter({},1800,80,2700,hard);
    archerEasy.Spawn(EnemyKind::Crossbow,1800);archerHard.Spawn(EnemyKind::Crossbow,1800);
    for(int i=0;i<20;++i){archerEasy.Update(1.0F/60,{1500,605});archerHard.Update(1.0F/60,{1500,605});}
    Check(archerEasy.Units()[0].state==EnemyState::Windup&&archerHard.Units()[0].state==EnemyState::Windup&&
          std::abs(archerEasy.Units()[0].timer-archerHard.Units()[0].timer)<.001F,"Difficulty shortened attack warning");
    for(int i=0;i<40;++i){archerEasy.Update(1.0F/60,{1500,605});archerHard.Update(1.0F/60,{1500,605});}
    Check(!archerHard.Bolts().empty()&&std::abs(archerHard.Bolts()[0].velocity.x)>std::abs(archerEasy.Bolts()[0].velocity.x),"Projectile pressure unchanged");

    StoryBoss w;w.Enter("wtower",2900,hard);Check(std::abs(w.MaxHealth()-170.5F)<.01F,"Boss health not scaled");
    w.TryStart(w.TriggerX());Vector2 player{w.Feet().x-310,605};
    for(int i=0;i<50;++i)w.Update(1.0F/60,player);
    w.Damage(w.MaxHealth()*.51F);
    for(int i=0;i<300&&w.State()!=StoryBossState::C4;++i)w.Update(1.0F/60,player);
    Check(w.PhaseTwo()&&w.Blasts().size()==3,"Scaled W lost half-health C4");
    Check(w.Blasts()[0].timer>3.19F&&w.Blasts()[0].tracking>1.69F,"Hard mode shortened C4 dodge warning");
    std::cout<<"Adaptive performance learning, 3/4/5/6 troops, frozen waves, all four Bosses, retries and warning timing passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
