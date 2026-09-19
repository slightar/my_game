#include "enemy_system.h"
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <set>

void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
const EnemyUnit& Unit(const EnemySystem& s,unsigned id){for(const auto& u:s.Units())if(u.id==id)return u;throw std::runtime_error("Missing unit");}
void Tick(EnemySystem& s,float seconds,Vector2 target){for(float t=0;t<seconds;t+=.01F)s.Update(.01F,target);}
int main(){try {
    std::set<std::string> ids;
    for(const auto& d:EnemyDefinitions()) {Check(ids.insert(d.id).second,"Duplicate enemy id");Check(d.health>0&&d.windup>0&&d.speed>0,"Invalid tuning");}
    EnemySystem s;
    auto shield=s.Spawn(EnemyKind::Shield,1500);
    s.Damage(shield,10,DamageType::Physical,{1300,600});
    Check(std::abs(Unit(s,shield).health-34)<.01F,"Frontal physical shield reduction");
    s.Damage(shield,10,DamageType::Physical,{1700,600});
    Check(std::abs(Unit(s,shield).health-24)<.01F,"Rear shield weakness");
    s.Damage(shield,10,DamageType::Arts,{1300,600});
    Check(std::abs(Unit(s,shield).health-14)<.01F,"Arts should bypass shield");
    s.Damage(shield,1,DamageType::Arts,{1300,600},.4F);
    s.Damage(shield,10,DamageType::Physical,{1300,600});
    Check(std::abs(Unit(s,shield).health-3)<.01F,"Stun exposes shield");

    s.Clear();auto slug=s.Spawn(EnemyKind::Slug,1200);
    const Rectangle body{1140,590,40,54};Vector2 source{};
    s.Update(.01F,{1160,610});
    Check(Unit(s,slug).state==EnemyState::Windup,"Slug telegraph missing");
    Check(!s.AttackHits(body,body,source),"Windup caused damage");
    Tick(s,.42F,{1160,610});
    Check(s.AttackHits(body,body,source),"Slug strike missed nearby body");
    Check(!s.AttackHits(body,body,source),"Attack hit twice in one strike");

    s.Clear();auto bomb=s.Spawn(EnemyKind::Exploder,1300);
    s.Damage(bomb,99,DamageType::Physical,{1000,620});
    Check(Unit(s,bomb).state==EnemyState::Fuse&&!s.Cleared(),"Dead exploder skipped fuse");
    Tick(s,.6F,{1000,600});
    Check(Unit(s,bomb).state==EnemyState::Fuse,"Explosion fired before telegraph ended");
    Tick(s,.22F,{1000,600});
    const Rectangle blastBody{1240,590,40,54};
    Check(s.AttackHits(blastBody,blastBody,source),"Explosion did not damage in radius");
    Check(!s.AttackHits(blastBody,blastBody,source),"Explosion applied repeated damage");
    Tick(s,.4F,{1000,600});Check(s.Cleared(),"Explosion never cleared");

    s.Clear();auto archer=s.Spawn(EnemyKind::Crossbow,1600);
    s.Update(.01F,{1250,582});
    Check(Unit(s,archer).state==EnemyState::Windup,"Archer aim phase missing");
    Tick(s,.93F,{1250,440});
    Check(!s.Bolts().empty(),"Archer did not fire");
    Check(std::abs(s.Bolts().front().velocity.y)<.01F,"Arrow homed after telegraph lock");
    Check(s.DestroyBolt(s.Bolts().front().position,25),"Sword wave cannot cancel arrow");
    s.Update(.01F,{1250,440});Check(s.Bolts().empty(),"Cancelled bolt remained active");

    s.Clear();auto drone=s.Spawn(EnemyKind::Drone,1200);
    const float droneStartY=Unit(s,drone).feet.y;
    const Vector2 droneTarget{1150,GameConfig::kFloorY-40.0F};
    s.Update(.01F,droneTarget);
    Check(Unit(s,drone).state==EnemyState::Windup,"Drone lock-on phase missing");
    Tick(s,1.15F,droneTarget);
    Check(!s.Bolts().empty(),"Drone did not fire an energy bolt");
    Check(std::abs(Unit(s,drone).feet.y-droneStartY)<25.0F,"Drone lost its flight height");

    s.Clear();auto walker=s.Spawn(EnemyKind::Shield,1300);
    Tick(s,.6F,{2000,600});
    Check(Unit(s,walker).facing==1 && !Unit(s,walker).turning,"Shield did not finish turning right");
    for(float dt:{1.0F/60,1.0F/144,.017F,.033F}) {
        s.Update(dt,{2000,600});
        Check(Unit(s,walker).moving,"Substep residue erased shield walk animation");
    }

    s.Clear();auto far=s.Spawn(EnemyKind::Soldier,1400);auto near=s.Spawn(EnemyKind::Soldier,1200);
    Bullet bullet;bullet.position={1600,600};bullet.velocity={2000,0};bullet.radius=5;bullet.damage=2;bullet.lifetime=1;
    Check(s.ResolveBullet(bullet,{1000,600}),"Fast bullet tunnelled through enemies");
    Check(Unit(s,near).health==10&&Unit(s,far).health==12,"Bullet ignored nearest collision");
    Bullet slash;slash.position={1300,600};slash.kind=BulletKind::MeleeSlash;slash.radius=145;slash.damage=2;slash.lifetime=1;slash.facingDirection=1;
    Check(s.ResolveBullet(slash,slash.position),"Melee missed multiple targets");
    const auto health=Unit(s,near).health;
    Check(Unit(s,far).health==10,"Melee should reach second target");
    Check(!s.ResolveBullet(slash,slash.position)&&Unit(s,near).health==health,"Melee damaged same unit repeatedly");
    Check(s.Target({1250,600},1).position.x==1400,"Target selection ignored facing");
    s.ResetTrial();Check(s.Remaining()==6&&!s.Cleared(),"Trial formation not complete");
    std::cout<<"Enemy combat rules passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
