#include "enemy_system.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
// A shield cannot be damaged from behind until it has spent this long in Approach, so
// enemies that spawn already facing away (or clustered) cannot be back-stabbed before
// they ever get to turn.
constexpr float kSpawnGrace = .4F;
// A cloaked unit drops its cloak once the operator is this close. Melee always connects
// regardless - only ranged projectiles are filtered by `cloaked`. The value lives on
// EnemySystem so the renderer and tests share one definition.
constexpr float kCloakRevealRange = EnemySystem::kCloakRevealRange;
constexpr bool IsCloakedKind(EnemyKind kind) { return kind==EnemyKind::StealthCrossbow; }
// Every kind that fires a bolt shares the same channel. The caster's shot is slower and
// does Arts (the counter to the guard's physical-only block); 酸液源石虫's is slower still
// and applies corrosion on contact.
bool IsRangedKind(EnemyKind kind) {
    return EnemyData(kind).attack!=EnemyAttack::Melee;
}
// Ground archers and casters keep their distance; the drone holds station overhead and
// always fires from where it is. Widening this to every ranged kind would silently stop
// the drone from ever committing to a shot.
bool KeepsDistance(EnemyKind kind) {
    return IsRangedKind(kind) && kind!=EnemyKind::Drone;
}
bool SegmentHit(Vector2 a, Vector2 b, Rectangle r, float radius, float& time) {
    float lo = 0, hi = 1;
    const float origins[]{a.x, a.y}, deltas[]{b.x-a.x,b.y-a.y};
    const float mins[]{r.x-radius,r.y-radius}, maxs[]{r.x+r.width+radius,r.y+r.height+radius};
    for (int axis=0;axis<2;++axis) {
        if (std::abs(deltas[axis]) < .0001F) {
            if (origins[axis] < mins[axis] || origins[axis] > maxs[axis]) return false;
        } else {
            float first=(mins[axis]-origins[axis])/deltas[axis], last=(maxs[axis]-origins[axis])/deltas[axis];
            if (first>last) std::swap(first,last);
            lo=std::max(lo,first);hi=std::min(hi,last);
            if (lo>hi) return false;
        }
    }
    time=lo;return true;
}
void SetState(EnemyUnit& u, EnemyState state, float timer) {
    u.state=state;u.timer=timer;u.animationTime=0;u.hitConsumed=false;
}
Rectangle AttackArea(const EnemyUnit& u) {
    const auto& d=EnemyData(u.kind);
    return {u.facing>0 ? u.feet.x : u.feet.x-d.reach, u.feet.y-d.height, d.reach, d.height};
}
}
Rectangle EnemyUnit::Hitbox() const { const auto& d=EnemyData(kind);return {feet.x-d.width/2,feet.y-d.height,d.width,d.height}; }
Vector2 EnemyUnit::Center() const { return {feet.x,feet.y-EnemyData(kind).height/2}; }
bool EnemyUnit::Targetable() const { return health>0 && state!=EnemyState::Dead && state!=EnemyState::Blast; }
bool EnemyUnit::Guarding() const { return EnemyData(kind).shield && stun<=0 && state!=EnemyState::Recover && state!=EnemyState::Dead; }
bool EnemyUnit::InterceptsProjectiles() const { return !cloaked; }
void EnemySystem::Clear() {
    units_.clear();bolts_.clear();nextId_=1;
    entryX_=kEntryX;leftBound_=GameConfig::kBossGateX+55;rightBound_=GameConfig::kRoom.x+GameConfig::kRoom.width-65;
    spawnQueue_.clear();nextSpawn_=0;spawnTimer_=0;entryPulse_=0;
}
unsigned EnemySystem::Spawn(EnemyKind kind, float x) {
    EnemyUnit u;u.id=nextId_++;u.kind=kind;
    u.feet={std::clamp(x,leftBound_,rightBound_), kind==EnemyKind::Drone ? GameConfig::kFloorY-260.0F : GameConfig::kFloorY};
    u.health=EnemyData(kind).health;
    units_.push_back(u);return u.id;
}
void EnemySystem::ResetTrial() {
    Clear();
    // Authored action-game timing, inspired by PRTS SPAWN actions. No units exist
    // until Game starts updating the encounter after crossing the arena trigger.
    spawnQueue_={EnemyKind::Slug,EnemyKind::Hound,EnemyKind::AcidSlug,
        EnemyKind::Soldier,EnemyKind::Caster,EnemyKind::SlugHigh,
        EnemyKind::StealthCrossbow,EnemyKind::Crossbow,EnemyKind::IrrSlug,
        EnemyKind::Shield,EnemyKind::Drone};
    for(unsigned i=static_cast<unsigned>(EnemyKind::SlugAlpha);i<kEnemyKindCount;++i)
        spawnQueue_.push_back(static_cast<EnemyKind>(i));
    spawnTimer_=kFirstSpawnDelay;
}
void EnemySystem::ResetEncounter(std::vector<EnemyKind> wave,float entryX,float left,float right) {
    Clear();leftBound_=left;rightBound_=std::max(left,right);
    entryX_=std::clamp(entryX,leftBound_,rightBound_);
    spawnQueue_=std::move(wave);spawnTimer_=kFirstSpawnDelay;
}
void EnemySystem::Update(float dt, Vector2 player) {
    // Bound catch-up time and substep attacks/projectiles on slow frames.
    for(auto& b:bolts_) b.previous=b.position;
    float remaining=std::clamp(dt,0.0F,.25F);
    // Float residue must not run a zero-displacement tick and erase the walk pose.
    while (remaining>0.000001F) { const float step=std::min(remaining,1.0F/120);Tick(step,player);remaining-=step; }
}
void EnemySystem::Tick(float dt, Vector2 player) {
    entryPulse_=std::max(0.0F,entryPulse_-dt);
    if(Pending()) {
        spawnTimer_-=dt;
        while(Pending() && spawnTimer_<=0.0F) {
            Spawn(spawnQueue_[nextSpawn_++],entryX_);
            auto& unit=units_.back();
            unit.facing=player.x>=unit.feet.x?1:-1;
            unit.cloaked=IsCloakedKind(unit.kind);
            spawnTimer_+=kSpawnInterval;
            entryPulse_=.65F;
        }
    }
    for(auto& b:bolts_) {
        b.velocity.y+=b.gravity*dt;
        b.position.x+=b.velocity.x*dt;b.position.y+=b.velocity.y*dt;b.lifetime-=dt;
        if(b.blastRadius>0 && b.position.y>=GameConfig::kFloorY-4) {
            b.position.y=GameConfig::kFloorY-4;b.velocity={};b.gravity=0;
            b.lifetime=std::min(b.lifetime,.30F);
        }
    }
    std::erase_if(bolts_,[this](const auto& b){return b.lifetime<=0 || b.position.x<leftBound_-100 || b.position.x>rightBound_+100 || b.position.y>GameConfig::kFloorY+10;});
    for (auto& u:units_) {
        const auto& d=EnemyData(u.kind);
        u.animationTime+=dt;u.flash=std::max(0.0F,u.flash-dt);u.moving=false;
        u.haste=std::max(0.0F,u.haste-dt);
        if (u.state==EnemyState::Dead) continue;
        u.facingTime+=dt;
        if (u.state==EnemyState::Fuse) {
            u.timer-=dt;
            if (u.timer<=0) { u.health=0;SetState(u,EnemyState::Blast,.35F); }
            continue;
        }
        if (u.state==EnemyState::Blast) { u.timer-=dt;if(u.timer<=0) SetState(u,EnemyState::Dead,0);continue; }
        if (u.stun>0) {u.stun=std::max(0.0F,u.stun-dt);continue;}
        u.timer-=dt;
        if (u.kind==EnemyKind::Drone) {
            u.feet.y=GameConfig::kFloorY-260.0F+std::sin(u.animationTime*1.7F)*22.0F;
        }
        const float dx=player.x-u.feet.x, distance=std::abs(dx);
        // Cloak is pure geometry: close in and it shows itself, back off and it vanishes
        // again. Windup and Strike force it visible so the shot is never a surprise.
        if (IsCloakedKind(u.kind)) {
            const bool shooting=u.state==EnemyState::Windup || u.state==EnemyState::Strike;
            u.cloaked=!shooting && distance>kCloakRevealRange;
        }
        if (u.state==EnemyState::Approach) {
            const int desired=dx>=0?1:-1;
            // Turn to face the player at once, like every other unit. A scripted
            // turn-around delay used to live here; it read as a canned animation, and
            // the player can already walk behind a guard on their own, so the window it
            // was granting is not needed. Facing is still only changed here and at the
            // end of Recover, never mid-attack.
            if (u.facingTime>=kSpawnGrace) u.facing=desired;
            if(distance>d.reach || (KeepsDistance(u.kind) && distance<200)) {
                float direction=static_cast<float>(u.facing);
                if(KeepsDistance(u.kind) && distance<200) direction=-direction;
                const float speed=d.speed*(u.haste>0?EnemySystem::kEnergizeSpeedScale:1.0F);
                const float next=std::clamp(u.feet.x+direction*speed*dt,leftBound_,rightBound_);
                u.moving=next!=u.feet.x;u.feet.x=next;
                // Cornered ranged units can still shoot rather than becoming stuck backing up.
                if(u.moving || !IsRangedKind(u.kind)) continue;
            }
            if (std::abs(player.y-u.Center().y)>135 && !IsRangedKind(u.kind)) continue;
            u.aim=player;
            SetState(u,EnemyState::Windup,d.windup);
        } else if (u.state==EnemyState::Windup && u.timer<=0) {
            if(IsRangedKind(u.kind)) {
                const Vector2 origin{u.feet.x+u.facing*24.0F,u.feet.y-62};
                const float ax=u.aim.x-origin.x,ay=u.aim.y-origin.y,len=std::max(1.0F,std::sqrt(ax*ax+ay*ay));
                // 术师 lobs a slow Arts bolt, 酸液源石虫 a slow corrosive spit (290 vs 410).
                const bool arts=d.attack==EnemyAttack::ArtsBolt||d.attack==EnemyAttack::ColdBolt;
                const bool corrosive=d.attack==EnemyAttack::AcidBolt;
                const float speed=(arts||corrosive)?290.0F:410.0F;
                EnemyBolt bolt{origin,origin,{ax/len*speed,ay/len*speed},3,arts,corrosive};
                ++u.attacks;
                bolt.cold=d.attack==EnemyAttack::ColdBolt && u.attacks%3==0;
                bolt.shatter=d.shatter;
                if(d.attack==EnemyAttack::Lob) {
                    const float flight=std::clamp(std::abs(ax)/430.0F,.65F,1.35F);
                    bolt.gravity=680;
                    const float floorDelta=GameConfig::kFloorY-4-origin.y;
                    bolt.velocity={ax/flight,(floorDelta-.5F*bolt.gravity*flight*flight)/flight};
                    bolt.blastRadius=u.kind==EnemyKind::GuerrillaMortar?80:55;
                    bolt.landing={u.aim.x,GameConfig::kFloorY-4};
                }
                bolts_.push_back(bolt);
            }
            SetState(u,EnemyState::Strike,.22F);
        } else if (u.state==EnemyState::Strike && u.timer<=0) {
            SetState(u,EnemyState::Recover,d.recovery);
        } else if (u.state==EnemyState::Recover && u.timer<=0) {
            u.facing=dx>=0?1:-1;SetState(u,EnemyState::Approach,0);
        }
    }
}
bool EnemySystem::Damage(unsigned id, float damage, DamageType type, Vector2 source, float stun) {
    for(auto& u:units_) if(u.id==id && u.Targetable() && damage>0) {
        const bool front=(source.x-u.feet.x)*u.facing>=0;
        // Behind the guard and it is not mid-bash: the shield simply is not in the way.
        // This is the "walk around it" reward, and it costs the guard nothing scripted -
        // no turn animation, no artificial window. The trade is that the 80% physical
        // reduction only applies to its front arc, so a guard that has just committed to
        // a bash is also open from the front while it recovers.
        const auto& d=EnemyData(u.kind);
        const bool exposed = !front && d.shield && u.facingTime>=kSpawnGrace;
        u.blocked=u.Guarding() && !exposed && type==DamageType::Physical && source.y>=u.feet.y-EnemyData(u.kind).height-15;
        const float reduction=type==DamageType::Physical && (u.blocked||!d.shield)?d.armor:0;
        u.health=std::max(0.0F,u.health-damage*(1-reduction));u.flash=.16F;
        if(u.health<=0) {
            // Both slug lines detonate on death. 高能源石虫 holds a long physical fuse;
            // 辐能源石虫 leaks raw energy as it goes, handing every other live enemy a
            // burst of speed (the original's "使场上敌人获得1点能量").
            if(u.kind==EnemyKind::SlugHigh) { u.stun=0;if(u.state!=EnemyState::Fuse) SetState(u,EnemyState::Fuse,1.00F); }
            else if(u.kind==EnemyKind::IrrSlug) {
                u.stun=0;
                if(u.state!=EnemyState::Fuse) SetState(u,EnemyState::Fuse,1.10F);
                for(auto& other:units_)
                    if(other.id!=u.id && other.state!=EnemyState::Dead && other.health>0)
                        other.haste=EnemySystem::kEnergizeDuration;
            }
            else if(d.coldBlast) {u.stun=0;SetState(u,EnemyState::Fuse,.85F);}
            else SetState(u,EnemyState::Dead,0);
        } else if(stun>0 && u.state!=EnemyState::Fuse) {
            u.stun=std::max(u.stun,stun);SetState(u,EnemyState::Recover,EnemyData(u.kind).recovery);
        }
        return true;
    }
    return false;
}
bool EnemySystem::ResolveBullet(Bullet& bullet, Vector2 previous) {
    if(bullet.activationDelay>0 || bullet.lifetime<=0) return false;
    // MeleeSlash swings a short arc and collides as a circle around its own position
    // rather than sweeping a segment, so it can catch several targets in one pass and is
    // never filtered by stealth - only flying projectiles are.
    const bool melee=bullet.kind==BulletKind::MeleeSlash;
    std::vector<std::pair<float,unsigned>> hits;
    for(const auto& u:units_) if(u.Targetable() && std::find(bullet.hitEnemyIds.begin(),bullet.hitEnemyIds.end(),u.id)==bullet.hitEnemyIds.end()) {
        // A cloaked unit simply is not there as far as projectiles are concerned - the
        // bullet flies on to whatever is behind it and keeps its lifetime. Melee is
        // exempt by design, so walking up with the blade is the counter.
        if(!melee && !u.InterceptsProjectiles()) continue;
        float t=0;
        if(melee ? CheckCollisionCircleRec(bullet.position,bullet.radius,u.Hitbox()) : SegmentHit(previous,bullet.position,u.Hitbox(),bullet.radius,t)) hits.emplace_back(t,u.id);
    }
    std::sort(hits.begin(),hits.end());
    bool damaged=false;
    for(const auto& [time,id]:hits) {
        Vector2 source=previous;
        if(melee) source.x=bullet.position.x-(bullet.facingDirection==0?1:bullet.facingDirection)*bullet.radius*1.5F;
        if(Damage(id,bullet.damage,bullet.damageType,source,bullet.stunDuration)) {
            damaged=true;bullet.hitEnemyIds.push_back(id);
            if(!melee) {bullet.lifetime=0;bullet.hasHit=true;break;}
        }
    }
    return damaged;
}
bool EnemySystem::DestroyBolt(Vector2 position,float radius) {
    for(auto& b:bolts_) if(b.lifetime>0 && CheckCollisionCircles(position,radius,b.position,7)) {b.lifetime=0;return true;}
    return false;
}
bool EnemySystem::AttackHits(Rectangle body,Rectangle projectileBody,Vector2& source,bool* corrosive,EnemyImpact* impact) {
    if(corrosive) *corrosive=false;
    if(impact) *impact={};
    for(auto& u:units_) if(!u.hitConsumed && u.stun<=0) {
        const bool blast=u.state==EnemyState::Blast && CheckCollisionCircleRec(u.Center(),kBlastRadius,body);
        // Melee reach is never filtered by stealth - only ranged projectiles are.
        const bool strike=u.state==EnemyState::Strike && !IsRangedKind(u.kind) && CheckCollisionRecs(AttackArea(u),body);
        if(blast||strike) {
            u.hitConsumed=true;source=u.Center();
            if(impact) {impact->cold=blast&&EnemyData(u.kind).coldBlast;impact->shatter=EnemyData(u.kind).shatter;impact->frozenDamage=u.kind==EnemyKind::Icebreaker?3:2;}
            return true;
        }
    }
    for(auto& b:bolts_) {
        float t=0;
        const bool landed=b.blastRadius>0 && b.gravity==0;
        if(b.lifetime>0 && (landed?CheckCollisionCircleRec(b.position,b.blastRadius,body):SegmentHit(b.previous,b.position,projectileBody,5,t))) {
            b.lifetime=0;source=b.previous;
            if(corrosive) *corrosive=b.corrosive;
            if(impact) {impact->cold=b.cold;impact->shatter=b.shatter;}
            return true;
        }
    }
    return false;
}
EnemyTarget EnemySystem::Target(Vector2 player,int facing) const {
    const EnemyUnit* best=nullptr;float bestScore=std::numeric_limits<float>::max();
    // Cloaked units stay selectable: the assist aim is the only thing pointing them out,
    // and the bullet it fires is filtered in ResolveBullet anyway.
    for(const auto& u:units_) if(u.Targetable()) {
        const float dx=u.feet.x-player.x;
        const float score=std::abs(dx)+(dx*facing<0?4000.0F:0.0F);
        if(score<bestScore) {bestScore=score;best=&u;}
    }
    return best?EnemyTarget{best->Center(),EnemyData(best->kind).width/2}:EnemyTarget{{player.x+facing*500,player.y},0};
}
int EnemySystem::Remaining() const {
    return Pending()+static_cast<int>(std::count_if(units_.begin(),units_.end(),[](const auto& u){return u.state!=EnemyState::Dead;}));
}
bool EnemySystem::Cleared() const {
    return !units_.empty() && Remaining()==0 && std::none_of(bolts_.begin(),bolts_.end(),[this](const auto& b){return b.lifetime>0;});
}
