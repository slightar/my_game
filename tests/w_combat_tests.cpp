#include "story_boss.h"
#include <iostream>
#include <stdexcept>
#include <set>

void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void Tick(StoryBoss& w,float seconds,Vector2 player){for(int i=0;i<int(seconds*120);++i)w.Update(1.0F/120,player);}
Vector2 Start(StoryBoss& w,float width=2900){w.Reset();w.Enter("wtower",width);w.TryStart(w.TriggerX());const Vector2 p{w.Feet().x-310,605};Tick(w,.8F,p);return p;}
void Wait(StoryBoss& w,StoryBossState state,Vector2 player,float limit=20){
    static int sequence=0;++sequence;
    for(int i=0;i<int(limit*120)&&w.State()!=state;++i)w.Update(1.0F/120,player);
    if(w.State()!=state)throw std::runtime_error("W wait "+std::to_string(sequence)+" never reached state "+std::to_string(int(state))+"; ended in "+std::to_string(int(w.State())));
}
int main(){try {
    StoryBoss w;auto p=Start(w);
    Wait(w,StoryBossState::Recover,p);
    Check(w.Grenades().size()==1,"Flight fixture did not launch a grenade");
    const auto airborne=w.Grenades().front().position;
    const Rectangle aerialBody{airborne.x-19,airborne.y-39,38,78};
    const Rectangle aerialProjectileBody{airborne.x-12,airborne.y-18,24,36};
    Vector2 flightSource{};BossImpact flightImpact{};
    Check(w.AttackHits(aerialBody,aerialProjectileBody,flightSource,&flightImpact)&&flightImpact.damage==1,
          "Flying grenade did not damage an airborne player");
    Check(!w.AttackHits(aerialBody,aerialProjectileBody,flightSource),"Flying grenade dealt damage twice");
    Check(!w.Visuals().empty()&&w.Visuals().back().kind==BossFx::WExplosion,"Flying hit did not show an impact explosion");
    Tick(w,.8F,p);
    Check(w.Grenades().empty()&&w.Blasts().empty(),"Contact-detonated grenade retained a landing explosion");

    p=Start(w);Wait(w,StoryBossState::Recover,p);
    const auto previous=w.Grenades().front().position;
    w.Update(.2F,p);
    Check(w.Grenades().size()==1,"Swept flight fixture landed too early");
    const auto current=w.Grenades().front().position;
    const Vector2 middle{(previous.x+current.x)*.5F,(previous.y+current.y)*.5F};
    const Rectangle crossed{middle.x-1,middle.y-1,2,2};
    Check(!CheckCollisionCircleRec(previous,7,crossed)&&!CheckCollisionCircleRec(current,7,crossed),
          "Swept flight fixture already touches an endpoint");
    Check(!w.AttackHits(crossed,{middle.x-1,middle.y+80,2,2},flightSource),"Flight collision ignored the projectile hitbox");
    Check(w.AttackHits(crossed,crossed,flightSource),"Fast grenade skipped a player between frames");
    Check(CheckCollisionPointRec(flightSource,{crossed.x-7.001F,crossed.y-7.001F,
                                              crossed.width+14.002F,crossed.height+14.002F}),
          "Flying explosion appeared beyond the collision point");

    p=Start(w);Wait(w,StoryBossState::Recover,p);
    const auto intercepted=w.Grenades().front().position;
    Check(w.DestroyProjectile(intercepted,20),"Skill failed to intercept flying grenade");
    Check(!w.AttackHits({intercepted.x-19,intercepted.y-39,38,78},
                       {intercepted.x-12,intercepted.y-18,24,36},flightSource),"Intercepted grenade still damaged player");

    p=Start(w);
    const float start=w.Feet().x;Wait(w,StoryBossState::Recover,p);
    const float recovery=w.Feet().x;Tick(w,.4F,p);
    Check(std::abs(w.Feet().x-recovery)>40&&std::abs(w.Feet().x-start)>60,"W still stands through the attack recovery");
    Wait(w,StoryBossState::VolleyWindup,p);const auto locked=w.GrenadeTargets();
    Check(locked.size()==3,"Full-health volley is not three distinct landing points");
    const float beforeWindup=w.Feet().x;Tick(w,.4F,{p.x-180,p.y});
    Check(w.GrenadeTargets()==locked&&w.Feet().x==beforeWindup,"Volley warning followed player or removed counterattack window");
    Wait(w,StoryBossState::Volley,p);Tick(w,.50F,p);
    Check(w.Grenades().size()==3,"W did not fire the telegraphed three-grenade volley");
    Wait(w,StoryBossState::MineWindup,p);const auto mineTargets=w.MineTargets();Tick(w,.71F,p);
    Check(w.Mines().size()==2&&w.Mines()[0].age<.1F,"Minefield did not deploy two initially unarmed traps");
    Vector2 source{};const Vector2 trap{mineTargets[0],605};
    Check(!w.AttackHits({trap.x-19,566,38,78},{},source),"Fresh trap hit without warning");
    Tick(w,.85F,{trap.x,440});
    for(const auto& mine:w.Mines())if(std::abs(mine.position.x-trap.x)<1)
        Check(mine.age>.8F&&mine.fuse<0,"Jumping above an armed mine triggered it");
    Tick(w,.05F,trap);bool triggered=false;
    for(const auto& mine:w.Mines())triggered|=mine.fuse>=0&&mine.fuse>.55F;
    Check(triggered,"Proximity did not start mine's visible delayed fuse");
    Tick(w,.8F,trap);
    Check(w.AttackHits({trap.x-19,566,38,78},{},source),"Triggered mine did not deal real damage");

    p=Start(w);Tick(w,1.3F,p);p={w.Feet().x-100,605};
    Wait(w,StoryBossState::EvadeWindup,p);const float old=w.Feet().x,destination=w.EvadeTarget();
    Check(destination-old>300,"Close player did not provoke a substantial retreat");
    Tick(w,.25F,p);Wait(w,StoryBossState::Evade,p);Tick(w,.35F,p);
    Check(w.Feet().y<GameConfig::kFloorY-100&&w.Feet().x>old+90&&w.Targetable(),"W back-jump teleported or granted arbitrary invulnerability");
    Check(!w.Mines().empty()&&std::abs(w.Mines()[0].position.x-old)<1,"Back-jump did not leave an escape mine");
    Tick(w,.38F,p);Check(w.State()==StoryBossState::VolleyWindup&&w.Feet().y==GameConfig::kFloorY,"Back-jump did not land into a telegraphed counter-volley");

    p=Start(w);Wait(w,StoryBossState::Recover,p);w.Damage(3);w.Damage(3);w.Damage(3);w.Update(1.0F/120,p);
    Check(w.State()==StoryBossState::EvadeWindup&&std::abs(w.EvadeTarget()-p.x)<=500.1F,"Sustained damage did not provoke an in-range reposition");
    Wait(w,StoryBossState::Evade,p);Tick(w,.25F,p);w.Damage(1,.5F);
    Check(w.Feet().y<GameConfig::kFloorY&&w.State()==StoryBossState::Recover,"Stunning a jump teleported W to the floor");
    Tick(w,.55F,p);Check(w.Feet().y==GameConfig::kFloorY,"Stunned W did not fall to the floor");const float location=w.Feet().x;
    w.Damage(3);w.Damage(3);w.Damage(3);w.Update(1.0F/120,p);
    Check(w.State()!=StoryBossState::EvadeWindup&&std::abs(w.Feet().x-location)<10,"Hit reaction ignored its cooldown");

    p=Start(w,1200);const Vector2 corner{w.Feet().x-140,605};
    Wait(w,StoryBossState::EvadeWindup,corner);
    Check(w.EvadeTarget()<corner.x-200,"Cornered W did not choose the open side across the player");
    Wait(w,StoryBossState::Evade,corner);Tick(w,.72F,corner);
    Check(w.Feet().x>=w.Left()+35&&w.Feet().x<=w.Right()-35,"Reposition escaped arena bounds");

    p=Start(w);w.Damage(56);Wait(w,StoryBossState::C4,p);Tick(w,3.25F,p);
    Wait(w,StoryBossState::VolleyWindup,p);
    Check(w.GrenadeTargets().size()==5,"Half-health phase did not strengthen volley");
    std::set<StoryBossState> actions;float travel=0;bool safeJump=false;
    for(int i=0;i<7200;++i) {
        const Vector2 target{std::clamp(w.Feet().x-310,w.Left()+45,w.Right()-45),i%240<120?440.0F:605.0F};
        const float x=w.Feet().x;w.Update(1.0F/120,target);travel+=std::abs(w.Feet().x-x);actions.insert(w.State());
        Check(w.Mines().size()<=4,"Mine field grew without a limit");
        if(target.y<500)for(const auto& mine:w.Mines())if(mine.age>.8F&&mine.fuse<0)safeJump=true;
    }
    Check(actions.contains(StoryBossState::MineWindup)&&actions.contains(StoryBossState::Evade)&&actions.contains(StoryBossState::C4)&&
          actions.contains(StoryBossState::Volley)&&travel>1500&&safeJump,"Extended W encounter lost attack variety, mobility or aerial mine avoidance");
    w.Damage(10000);Check(w.Defeated()&&w.Mines().empty()&&w.Grenades().empty()&&w.Blasts().empty(),"Victory retained W's new hazards");
    p=Start(w);Wait(w,StoryBossState::VolleyWindup,p);
    const auto beforePhase=w.GrenadeTargets();w.Damage(56);
    Check(w.PhaseTwo()&&w.GrenadeTargets()==beforePhase&&w.VolleyCount()==3,"Half-health transition added untelegraphed volley targets");
    p=Start(w);Tick(w,1.3F,p);p={w.Feet().x-100,605};Wait(w,StoryBossState::EvadeWindup,p);w.Damage(56);
    Wait(w,StoryBossState::C4,p,2);
    Check(std::count_if(w.Blasts().begin(),w.Blasts().end(),[](const auto& b){return b.c4;})==3,"Half-health transition failed to prioritize C4 after landing");
    std::cout<<"W airborne grenade impact/swept collision/interception, strafe/recovery, locked volley, delayed mines, retreat/corner vault, pressure reaction, stun/cooldown, phase-two variety and cleanup passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
