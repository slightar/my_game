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
    // Size proportions come from the baked rigs, not from taste. This is the regression
    // guard for the bug where every spriteSize was hand-picked: 猎狗 ended up drawn at
    // roughly half the on-screen size of 术师 even though the rigs are nearly equal.
    {
        const auto h=[](EnemyKind k){return EnemyData(k).height;};
        const auto s=[](EnemyKind k){return EnemyData(k).spriteSize;};
        Check(h(EnemyKind::Slug)<70.0F,"源石虫 should stay a small creature");
        Check(h(EnemyKind::Slug)<h(EnemyKind::Hound)&&h(EnemyKind::Hound)<h(EnemyKind::Soldier),
              "Size ordering slug < hound < soldier is broken");
        Check(h(EnemyKind::Soldier)>100.0F&&h(EnemyKind::Soldier)<130.0F,
              "士兵 should be roughly operator-sized (the operator sprite is 112px)");
        Check(std::abs(h(EnemyKind::Caster)-h(EnemyKind::Soldier))<12.0F,
              "Humanoids drifted apart in scale");
        Check(h(EnemyKind::Shield)>=h(EnemyKind::Soldier),"重装防御者 should not be smaller than 士兵");
        // spriteSize is the on-screen cell, so it must track height as well.
        Check(s(EnemyKind::Slug)<s(EnemyKind::Soldier),"源石虫 cell should be smaller than 士兵's");
    }
    EnemySystem s;
    s.ResetTrial();
    const int roster=static_cast<int>(EnemyDefinitions().size());
    Check(s.Units().empty()&&s.Pending()==roster&&s.Remaining()==roster,
          "Trial must start with queued enemies and an empty map");
    Check(!s.Cleared(),"An empty map with pending enemies must not clear");
    Tick(s,EnemySystem::kFirstSpawnDelay-.1F,{1100,600});
    Check(s.Units().empty(),"Enemy appeared before the entry delay");
    Tick(s,.12F,{1100,600});
    Check(s.Units().size()==1&&s.Pending()==roster-1,"First entry did not release one enemy");
    Check(std::abs(s.Units().front().feet.x-EnemySystem::kEntryX)<5,
          "Enemy did not emerge at the red gate");
    s.Damage(s.Units().front().id,1000,DamageType::Arts,{1100,600});
    Check(!s.Cleared()&&s.Remaining()==roster-1,"Cleared before reinforcements arrived");
    Tick(s,EnemySystem::kSpawnInterval*roster,{1100,600});
    Check(s.Pending()==0&&s.Units().size()==static_cast<unsigned>(roster),
          "Entry skipped or duplicated queued enemies");
    std::set<EnemyKind> spawnedKinds;
    for(const auto& u:s.Units())spawnedKinds.insert(u.kind);
    Check(spawnedKinds.size()==static_cast<unsigned>(roster),"Entry lost an enemy kind");
    for(const auto& u:s.Units())s.Damage(u.id,10000,DamageType::Arts,{1100,600});
    Tick(s,8,{1100,600});
    Check(s.Cleared(),"Completed entry and combat failed to clear");
    s.ResetTrial();
    Check(s.Units().empty()&&s.Pending()==roster,"Retry did not reset the spawn queue");
    s.Clear();
    Tick(s,5,{1100,600});
    Check(s.Units().empty()&&s.Pending()==0&&!s.HasEntry(),"Clear retained trial reinforcements");
    auto shield=s.Spawn(EnemyKind::Shield,1500);
    // The spawn grace keeps a guard from being back-stabbed before it has faced anything.
    // 60 ticks = 1s of .01 substeps, well past kSpawnGrace (.4s).
    Tick(s,1.0F,{1300,600});
    s.Damage(shield,10,DamageType::Physical,{1300,600});
    Check(std::abs(Unit(s,shield).health-34)<.01F,"Frontal physical shield reduction");
    // Guarding() is false while stunned or recovering, so re-establish it before the
    // rear-arc assertion; the grace has long since elapsed.
    Tick(s,1.0F,{1300,600});
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

    // --- 辐能源石虫: Arts detonation that also feeds the field ------------------------
    // Same death blast as 高能源石虫, but its parting gift is energy: every other live
    // enemy gets a burst of speed, which is the action-game reading of the original's
    // "死亡后…使场上敌人获得1点能量".
    s.Clear();auto irr=s.Spawn(EnemyKind::IrrSlug,1300);auto buddy=s.Spawn(EnemyKind::Soldier,1700);
    Check(Unit(s,buddy).haste<=0,"Enemy started hastened");
    s.Damage(irr,99,DamageType::Physical,{1000,620});
    Check(Unit(s,irr).state==EnemyState::Fuse&&!s.Cleared(),"Dead 辐能源石虫 skipped its fuse");
    Check(Unit(s,buddy).haste>0,"辐能源石虫 death did not energize the field");
    Tick(s,.6F,{1000,600});
    Check(Unit(s,irr).state==EnemyState::Fuse,"辐能源石虫 exploded before its fuse ended");
    Tick(s,.55F,{1000,600});
    const Rectangle blastBody{1240,590,40,54};
    Check(s.AttackHits(blastBody,blastBody,source),"辐能源石虫 explosion did not damage in radius");
    Check(!s.AttackHits(blastBody,blastBody,source),"Explosion applied repeated damage");
    s.Damage(buddy,99,DamageType::Physical,{1700,600});
    Tick(s,.6F,{1000,600});Check(s.Cleared(),"Explosion never cleared");

    s.Clear();auto archer=s.Spawn(EnemyKind::Crossbow,1600);
    s.Update(.01F,{1250,582});
    Check(Unit(s,archer).state==EnemyState::Windup,"Archer aim phase missing");
    Tick(s,.93F,{1250,440});
    Check(!s.Bolts().empty(),"Archer did not fire");
    Check(std::abs(s.Bolts().front().velocity.y)<.01F,"Arrow homed after telegraph lock");
    Check(!s.Bolts().front().corrosive,"A plain arrow should not be corrosive");
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
    // The shield turns the moment it commits, like every other unit - no scripted
    // turn-around window. Facing must still be right without waiting on an animation.
    Check(Unit(s,walker).facing==1,"Shield did not turn to face the player on its own");
    for(float dt:{1.0F/60,1.0F/144,.017F,.033F}) {
        s.Update(dt,{2000,600});
        Check(Unit(s,walker).moving,"Substep residue erased shield walk animation");
    }

    // Walking around a guard must pay off immediately once it is engaged, and its rear
    // must stay soft while it bashes - that is the whole reward for repositioning.
    s.Clear();auto flank=s.Spawn(EnemyKind::Shield,1500);
    Tick(s,1.0F,{1500-60,600});
    const float beforeRear=Unit(s,flank).health;
    s.Damage(flank,10,DamageType::Physical,{1700,600});
    Check(std::abs(Unit(s,flank).health-(beforeRear-10))<.01F,"Rear hit was reduced after engaging");
    const float beforeFront=Unit(s,flank).health;
    s.Damage(flank,10,DamageType::Physical,{1300,600});
    Check(std::abs(Unit(s,flank).health-(beforeFront-2))<.01F,"Frontal hit lost its 80% reduction");

    s.Clear();auto grace=s.Spawn(EnemyKind::Shield,1500);
    s.Update(.01F,{1500,600});
    const float spawnHealth=Unit(s,grace).health;
    s.Damage(grace,10,DamageType::Physical,{1700,600});
    Check(std::abs(Unit(s,grace).health-(spawnHealth-2))<.01F,"Spawn grace did not cover the guard's back");
    Tick(s,1.0F,{1500,600});
    const float engaged=Unit(s,grace).health;
    s.Damage(grace,10,DamageType::Physical,{1700,600});
    Check(std::abs(Unit(s,grace).health-(engaged-10))<.01F,"Spawn grace never expired");

    s.Clear();auto far=s.Spawn(EnemyKind::Soldier,1400);auto near=s.Spawn(EnemyKind::Soldier,1200);    Bullet bullet;bullet.position={1600,600};bullet.velocity={2000,0};bullet.radius=5;bullet.damage=2;bullet.lifetime=1;
    Check(s.ResolveBullet(bullet,{1000,600}),"Fast bullet tunnelled through enemies");
    Check(Unit(s,near).health==10&&Unit(s,far).health==12,"Bullet ignored nearest collision");
    Bullet slash;slash.position={1300,600};slash.kind=BulletKind::MeleeSlash;slash.radius=145;slash.damage=2;slash.lifetime=1;slash.facingDirection=1;
    Check(s.ResolveBullet(slash,slash.position),"Melee missed multiple targets");
    const auto health=Unit(s,near).health;
    Check(Unit(s,far).health==10,"Melee should reach second target");
    Check(!s.ResolveBullet(slash,slash.position)&&Unit(s,near).health==health,"Melee damaged same unit repeatedly");
    Check(s.Target({1250,600},1).position.x==1400,"Target selection ignored facing");

    // --- stealth archer -------------------------------------------------------------
    // Cloaked by default whenever the operator is further away than the reveal range,
    // and while it is just holding station out of reach.
    s.Clear();auto sneak=s.Spawn(EnemyKind::StealthCrossbow,1900);
    Tick(s,.05F,{1100,600});
    Check(Unit(s,sneak).cloaked,"Stealth archer was not cloaked at range");
    // A ranged projectile passes straight through and stays alive to hit targets beyond.
    const float sneakyHealth=Unit(s,sneak).health;
    Bullet shot;shot.position={1800,600};shot.velocity={1200,0};shot.radius=5;shot.damage=3;shot.lifetime=1;
    Check(!s.ResolveBullet(shot,{1700,600}),"Cloaked archer was hit by a ranged projectile");
    Check(Unit(s,sneak).health==sneakyHealth,"Cloaked archer still took ranged damage");
    Check(shot.lifetime>0&&!shot.hasHit,"Projectile was consumed by a cloaked unit instead of passing through");
    // Melee is never filtered by the cloak - that is the whole counter.
    Check(Unit(s,sneak).cloaked,"Cloak dropped while still at range");
    {
        Bullet blade;blade.position={1900,600};blade.kind=BulletKind::MeleeSlash;blade.radius=40;blade.damage=3;blade.lifetime=1;blade.facingDirection=1;
        Check(s.ResolveBullet(blade,blade.position),"Melee failed to connect with a cloaked unit");
    }
    Check(Unit(s,sneak).health==sneakyHealth-3,"Melee did not damage the cloaked archer");
    // Walking inside the reveal range drops the cloak, and it comes back when backing off.
    Tick(s,.05F,{1900-EnemySystem::kCloakRevealRange+20,600});
    Check(!Unit(s,sneak).cloaked,"Stealth archer stayed hidden after the operator closed in");
    Tick(s,.05F,{1100,600});
    Check(Unit(s,sneak).cloaked,"Stealth archer never re-cloaked after the operator backed off");
    // Inside reach it commits to a shot, and the shot itself is always visible - the
    // cloak only survives while it is holding station, never through the windup.
    Tick(s,.05F,{1500,600});
    Check(Unit(s,sneak).state==EnemyState::Windup,"Stealth archer never began aiming");
    Check(!Unit(s,sneak).cloaked,"Stealth archer aimed while still cloaked");
    Tick(s,.90F,{1500,600});
    Check(!s.Bolts().empty(),"Stealth archer never fired");

    // --- caster ---------------------------------------------------------------------
    // The Arts bolt is the answer to the guard's physical-only reduction.
    s.Clear();auto mage=s.Spawn(EnemyKind::Caster,1700);
    s.Update(.01F,{1500,582});
    Check(Unit(s,mage).state==EnemyState::Windup,"Caster aim phase missing");
    Tick(s,1.10F,{1500,440});
    Check(!s.Bolts().empty(),"Caster did not fire");
    Check(s.Bolts().front().arts,"Caster bolt was not tagged as Arts");
    // Slower than an arrow, so it is dodgeable.
    Check(std::abs(s.Bolts().front().velocity.x)<400.0F,"Caster bolt is as fast as an arrow");
    Check(s.DestroyBolt(s.Bolts().front().position,25),"Sword wave cannot cancel a caster bolt");

    // --- acid slug ------------------------------------------------------------------
    // Ranged physical, and its spit carries corrosion. The flag is reported through
    // AttackHits so the operator side can shorten its post-hit invincibility.
    s.Clear();auto acid=s.Spawn(EnemyKind::AcidSlug,1700);
    s.Update(.01F,{1500,582});
    Check(Unit(s,acid).state==EnemyState::Windup,"Acid slug aim phase missing");
    Tick(s,1.00F,{1500,440});
    Check(!s.Bolts().empty(),"Acid slug did not fire");
    Check(s.Bolts().front().corrosive,"Acid bolt was not tagged as corrosive");
    Check(!s.Bolts().front().arts,"Acid bolt should be physical, not Arts");
    Check(std::abs(s.Bolts().front().velocity.x)<400.0F,"Acid bolt should lob, not race the arrow");
    {
        // Sweep the bolt's flight path; a hit must report corrosion.
        const Rectangle path{1500,440,224,142};
        Vector2 acidSource{};bool corroded=false;
        Check(s.AttackHits({0,0,0,0},path,acidSource,&corroded),"Acid bolt missed the operator box");
        Check(corroded,"Corrosive bolt did not report corrosion");
        // And the flag must not linger: a later clean hit reports false.
        corroded=true;
        Check(!s.AttackHits({0,0,0,0},path,acidSource,&corroded),"Spent acid bolt hit twice");
        Check(!corroded,"Corrosion flag leaked past the bolt that caused it");
    }

    // --- 高能源石虫 ------------------------------------------------------------------
    // Dying detonates on a long fuse, so clearing it from a distance stays the safe play.
    s.Clear();auto heavy=s.Spawn(EnemyKind::SlugHigh,1300);
    s.Damage(heavy,99,DamageType::Physical,{1000,620});
    Check(Unit(s,heavy).state==EnemyState::Fuse&&!s.Cleared(),"Dead high-energy slug skipped its fuse");
    Tick(s,.6F,{1000,600});
    Check(Unit(s,heavy).state==EnemyState::Fuse,"High-energy slug exploded before its longer fuse ended");
    Tick(s,.45F,{1000,600});
    const Rectangle heavyBlast{1240,590,40,54};
    Check(s.AttackHits(heavyBlast,heavyBlast,source),"High-energy slug explosion did no damage");
    Tick(s,.4F,{1000,600});
    Check(s.Cleared(),"High-energy slug explosion never cleared");

    // --- hound ----------------------------------------------------------------------
    // Fast and short-winded: with the operator far away it closes ground far sooner than
    // a soldier does, which is what makes it the unit that punishes standing still.
    s.Clear();const auto runner=s.Spawn(EnemyKind::Hound,1600);const auto footman=s.Spawn(EnemyKind::Soldier,1600);
    Tick(s,.5F,{900,600});
    const float runnerX=Unit(s,runner).feet.x, footmanX=Unit(s,footman).feet.x;
    Check(runnerX<footmanX,"Hound did not close distance faster than a soldier");

    s.ResetTrial();Check(s.Remaining()==11&&!s.Cleared(),"Trial formation not complete");
    std::cout<<"Enemy combat rules passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
