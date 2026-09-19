#include "enemy_system.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
constexpr float leftBound = GameConfig::kBossGateX + 55;
constexpr float rightBound = GameConfig::kRoom.x + GameConfig::kRoom.width - 65;
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
bool EnemyUnit::Guarding() const { return kind==EnemyKind::Shield && stun<=0 && state!=EnemyState::Recover && state!=EnemyState::Dead; }
void EnemySystem::Clear() { units_.clear();bolts_.clear();nextId_=1; }
unsigned EnemySystem::Spawn(EnemyKind kind, float x) {
    EnemyUnit u;u.id=nextId_++;u.kind=kind;
    u.feet={std::clamp(x,leftBound,rightBound), kind==EnemyKind::Drone ? GameConfig::kFloorY-260.0F : GameConfig::kFloorY};
    u.health=EnemyData(kind).health;
    units_.push_back(u);return u.id;
}
void EnemySystem::ResetTrial() {
    Clear();
    // TODO: Replace this authored demonstration formation with chapter encounter data.
    Spawn(EnemyKind::Slug,1250);Spawn(EnemyKind::Soldier,1500);
    Spawn(EnemyKind::Exploder,1720);Spawn(EnemyKind::Shield,1960);Spawn(EnemyKind::Crossbow,2240);
    Spawn(EnemyKind::Drone,2450);
}
void EnemySystem::Update(float dt, Vector2 player) {
    // Bound catch-up time and substep attacks/projectiles on slow frames.
    for(auto& b:bolts_) b.previous=b.position;
    float remaining=std::clamp(dt,0.0F,.25F);
    // Float residue must not run a zero-displacement tick and erase the walk pose.
    while (remaining>0.000001F) { const float step=std::min(remaining,1.0F/120);Tick(step,player);remaining-=step; }
}
void EnemySystem::Tick(float dt, Vector2 player) {
    for(auto& b:bolts_) { b.position.x+=b.velocity.x*dt;b.position.y+=b.velocity.y*dt;b.lifetime-=dt; }
    std::erase_if(bolts_,[](const auto& b){return b.lifetime<=0 || b.position.x<leftBound-100 || b.position.x>rightBound+100 || b.position.y>GameConfig::kFloorY+10;});
    for (auto& u:units_) {
        const auto& d=EnemyData(u.kind);
        u.animationTime+=dt;u.flash=std::max(0.0F,u.flash-dt);u.moving=false;
        if (u.state==EnemyState::Dead) continue;
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
        if (u.state==EnemyState::Approach) {
            const int desired=dx>=0?1:-1;
            // Shield takes time to turn: the player gets a real opportunity to reach its back.
            if (desired!=u.facing) {
                if(u.kind==EnemyKind::Shield) {u.turning=true;SetState(u,EnemyState::Recover,.5F);u.aim=player;continue;}
                u.facing=desired;
            }
            if(distance>d.reach || (u.kind==EnemyKind::Crossbow && distance<200)) {
                float direction=static_cast<float>(u.facing);
                if(u.kind==EnemyKind::Crossbow && distance<200) direction=-direction;
                const float next=std::clamp(u.feet.x+direction*d.speed*dt,leftBound,rightBound);
                u.moving=next!=u.feet.x;u.feet.x=next;
                // Cornered ranged units can still shoot rather than becoming stuck backing up.
                if(u.moving || (u.kind!=EnemyKind::Crossbow && u.kind!=EnemyKind::Drone)) continue;
            }
            if (std::abs(player.y-u.Center().y)>135 && u.kind!=EnemyKind::Crossbow && u.kind!=EnemyKind::Drone) continue;
            u.aim=player;
            SetState(u,u.kind==EnemyKind::Exploder?EnemyState::Fuse:EnemyState::Windup,d.windup);
        } else if (u.state==EnemyState::Windup && u.timer<=0) {
            if(u.kind==EnemyKind::Crossbow || u.kind==EnemyKind::Drone) {
                const Vector2 origin{u.feet.x+u.facing*24.0F,u.feet.y-62};
                const float ax=u.aim.x-origin.x,ay=u.aim.y-origin.y,len=std::max(1.0F,std::sqrt(ax*ax+ay*ay));
                bolts_.push_back({origin,origin,{ax/len*410,ay/len*410},3});
            }
            SetState(u,EnemyState::Strike,.22F);
        } else if (u.state==EnemyState::Strike && u.timer<=0) {
            SetState(u,EnemyState::Recover,d.recovery);
        } else if (u.state==EnemyState::Recover && u.timer<=0) {
            u.turning=false;u.facing=dx>=0?1:-1;SetState(u,EnemyState::Approach,0);
        }
    }
}
bool EnemySystem::Damage(unsigned id, float damage, DamageType type, Vector2 source, float stun) {
    for(auto& u:units_) if(u.id==id && u.Targetable() && damage>0) {
        const bool front=(source.x-u.feet.x)*u.facing>=0;
        u.blocked=u.Guarding() && front && type==DamageType::Physical && source.y>=u.feet.y-EnemyData(u.kind).height-15;
        u.health=std::max(0.0F,u.health-damage*(u.blocked?.2F:1.0F));u.flash=.16F;
        if(u.health<=0) {
            if(u.kind==EnemyKind::Exploder) { u.stun=0;if(u.state!=EnemyState::Fuse) SetState(u,EnemyState::Fuse,.80F); }
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
    const bool melee=bullet.kind==BulletKind::MeleeSlash;
    std::vector<std::pair<float,unsigned>> hits;
    for(const auto& u:units_) if(u.Targetable() && std::find(bullet.hitEnemyIds.begin(),bullet.hitEnemyIds.end(),u.id)==bullet.hitEnemyIds.end()) {
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
bool EnemySystem::AttackHits(Rectangle body,Rectangle projectileBody,Vector2& source) {
    for(auto& u:units_) if(!u.hitConsumed && u.stun<=0) {
        const bool blast=u.state==EnemyState::Blast && CheckCollisionCircleRec(u.Center(),kBlastRadius,body);
        const bool strike=u.state==EnemyState::Strike && u.kind!=EnemyKind::Crossbow && u.kind!=EnemyKind::Drone && CheckCollisionRecs(AttackArea(u),body);
        if(blast||strike) {u.hitConsumed=true;source=u.Center();return true;}
    }
    for(auto& b:bolts_) {
        float t=0;
        if(b.lifetime>0 && SegmentHit(b.previous,b.position,projectileBody,5,t)) {b.lifetime=0;source=b.previous;return true;}
    }
    return false;
}
EnemyTarget EnemySystem::Target(Vector2 player,int facing) const {
    const EnemyUnit* best=nullptr;float bestScore=std::numeric_limits<float>::max();
    for(const auto& u:units_) if(u.Targetable()) {
        const float dx=u.feet.x-player.x;
        const float score=std::abs(dx)+(dx*facing<0?4000.0F:0.0F);
        if(score<bestScore) {bestScore=score;best=&u;}
    }
    return best?EnemyTarget{best->Center(),EnemyData(best->kind).width/2}:EnemyTarget{{player.x+facing*500,player.y},0};
}
int EnemySystem::Remaining() const {
    return static_cast<int>(std::count_if(units_.begin(),units_.end(),[](const auto& u){return u.state!=EnemyState::Dead;}));
}
bool EnemySystem::Cleared() const {
    return !units_.empty() && Remaining()==0 && std::none_of(bolts_.begin(),bolts_.end(),[](const auto& b){return b.lifetime>0;});
}
