#include "story_boss.h"
#include <algorithm>
#include <cmath>

namespace {
bool SweptObject(Vector2 from,Vector2 to,Vector2 center,float radius,float bulletRadius) {
    const float r=radius+bulletRadius;
    float low=0,high=1;
    const auto axis=[&](float origin,float delta,float target) {
        if(std::abs(delta)<.00001F)return std::abs(origin-target)<=r;
        float a=(target-r-origin)/delta,b=(target+r-origin)/delta;
        if(a>b)std::swap(a,b);
        low=std::max(low,a);high=std::min(high,b);return low<=high;
    };
    return axis(from.x,to.x-from.x,center.x)&&axis(from.y,to.y-from.y,center.y);
}
}

void Boss::StartMirror(Vector2 player) {
    fanAim_=player;darts_.clear();echoes_.clear();shadowRecover_=true;
    mirrorHalf_=health_<=maxHealth_*.5F?2:1;
    const float center=std::clamp(player.x,arena_.x+280,arena_.x+arena_.width-280);
    for(float side:{-1.0F,1.0F})
        echoes_.push_back({{center+side*220,floorY_-180},1.1F,.2F,0,true});
    mirrorCooldown_=health_<=maxHealth_*.5F?13:17;
    state_=State::MirrorWindup;stateTimer_=1.1F;animationTime_=0;
    ++effectRevision_;effectCue_=EffectCue::Charge;
}
void Boss::LaunchMirror() {
    for(const auto& echo:echoes_) {
        const float angle=std::atan2(fanAim_.y-echo.position.y,fanAim_.x-echo.position.x);
        for(int i=-mirrorHalf_;i<=mirrorHalf_;++i) {
            const float a=angle+i*.19F;
            darts_.push_back({echo.position,{std::cos(a)*520,std::sin(a)*520},a});
        }
    }
}
void Boss::StartAmbush(Vector2 player) {
    darts_.clear();echoes_.clear();shadowRecover_=true;
    const float center=std::clamp(player.x,arena_.x+270,arena_.x+arena_.width-270);
    fanAim_={center,floorY_-kRadius};
    // Mark all positions once; the center attacks first, then the two flanks.
    for(int i=0;i<3;++i) {
        const float offset=i==0?0:i==1?-190.0F:190.0F;
        echoes_.push_back({{center+offset,floorY_-kRadius},1.2F+i*.32F});
    }
    ambushCooldown_=health_<=maxHealth_*.5F?16:21;
    state_=State::AmbushWindup;stateTimer_=1.2F;animationTime_=0;
    ++effectRevision_;effectCue_=EffectCue::Charge;
}

void StoryBoss::StartAirburst(Vector2 player) {
    aim_={std::clamp(player.x,left_+240,right_-240),GameConfig::kFloorY};
    const int pairs=phaseTwo_?3:2;
    for(int i=0;i<pairs;++i) {
        const float x=aim_.x+(i-(pairs-1)*.5F)*170;
        // High flashes are safe on the ground; the next low flash asks for a jump or movement.
        for(int row=0;row<2;++row) {
            BossBlast blast;
            blast.position={x,GameConfig::kFloorY-(row==0?220.0F:18.0F)};
            blast.radius=58;blast.timer=1.5F+i*.5F+row*.28F;blast.airburst=true;
            blasts_.push_back(blast);
        }
    }
    expansionCooldown_[0]=(phaseTwo_?13.0F:17.0F)*difficulty_.cooldown;
    SetState(StoryBossState::AirburstWindup,1.5F);
}
void StoryBoss::StartRemoteMines(Vector2 player) {
    aim_={std::clamp(player.x,left_+350,right_-350),GameConfig::kFloorY-4};
    remoteMineCount_=phaseTwo_?4:3;
    expansionCooldown_[1]=(phaseTwo_?17.0F:21.0F)*difficulty_.cooldown;
    SetState(StoryBossState::RemoteMineWindup,1.0F);
}
void StoryBoss::DeployRemoteMines() {
    const int count=remoteMineCount_;
    // Replace old traps so the marked escape lanes cannot contain a hidden mine.
    mines_.clear();
    for(int i=0;i<count;++i) {
        BossMine mine;mine.position={aim_.x+(i-(count-1)*.5F)*145,aim_.y};
        mine.fuse=1.65F+i*.24F;mine.remote=true;mines_.push_back(mine);
    }
}
void StoryBoss::StartPrism(Vector2 player) {
    aim_=player;
    const float center=std::clamp(player.x,left_+280,right_-280);
    const int count=phaseTwo_?3:2;
    for(int i=0;i<count;++i) {
        BossZone prism;
        prism.kind=BossFx::FrostPrism;
        prism.position={center+(i-(count-1)*.5F)*240,GameConfig::kFloorY-225-(i%2)*45};
        prism.radius=26;prism.delay=1.65F+i*.38F;prism.remaining=.1F;
        prism.target=player;prism.durability=2;zones_.push_back(prism);
    }
    expansionCooldown_[0]=(phaseTwo_?12.0F:16.0F)*difficulty_.cooldown;
    SetState(StoryBossState::PrismWindup,1.25F);
}
void StoryBoss::StartThorns(Vector2 player) {
    aim_={player.x,GameConfig::kFloorY};
    const int count=phaseTwo_?7:5;
    const float direction=player.x>=feet_.x?1.0F:-1.0F;
    for(int i=0;i<count;++i) {
        const float x=feet_.x+direction*(100+i*105.0F);
        if(x<left_+40||x>right_-40)break;
        zones_.push_back({BossFx::FrostThorn,{x,GameConfig::kFloorY},38,1.3F+i*.23F,.22F});
    }
    expansionCooldown_[1]=(phaseTwo_?13.0F:18.0F)*difficulty_.cooldown;
    SetState(StoryBossState::ThornWindup,1.3F);
}
bool StoryBoss::StartPatriotExpansion(Vector2 player) {
    const float distance=std::abs(player.x-feet_.x);
    if(expansionCooldown_[0]<=0&&distance<570) {
        aim_=feet_;
        for(int i=0;i<3;++i)
            zones_.push_back({BossFx::PatriotQuake,feet_,150+i*125.0F,1.3F+i*.65F,.20F});
        expansionCooldown_[0]=(phaseTwo_?14.0F:19.0F)*difficulty_.cooldown;
        // Existing sacrificial pulses yield to the separately marked quake pulses.
        std::erase_if(zones_,[](const auto& z){return z.kind==BossFx::Ritual;});
        SetState(StoryBossState::QuakeWindup,1.3F);return true;
    }
    if(expansionCooldown_[1]<=0&&distance>155&&distance<780) {
        aim_={std::clamp(feet_.x+facing_*770.0F,left_,right_),GameConfig::kFloorY-92};
        expansionCooldown_[1]=(phaseTwo_?12.0F:17.0F)*difficulty_.cooldown;
        std::erase_if(zones_,[](const auto& z){return z.kind==BossFx::Ritual;});
        SetState(StoryBossState::LanceWindup,1.3F);return true;
    }
    return false;
}
void StoryBoss::TickExpansionHazards() {
    for(auto& zone:zones_)if(zone.remaining>0&&zone.delay<=0) {
        if(zone.kind==BossFx::FrostPrism&&!zone.consumed) {
            const float angle=std::atan2(zone.target.y-zone.position.y,zone.target.x-zone.position.x);
            for(int i=-1;i<=1;++i) {
                const float a=angle+i*.14F;
                const float speed=(phaseTwo_?410.0F:340.0F)*difficulty_.projectile;
                missiles_.push_back({BossFx::FrostBolt,zone.position,zone.position,{std::cos(a)*speed,std::sin(a)*speed},2.6F});
            }
            zone.consumed=true;AddVisual(BossFx::FrostPrism,zone.position,.8F);
        }
        if(!zone.visualStarted&&(zone.kind==BossFx::FrostThorn||zone.kind==BossFx::PatriotQuake)) {
            AddVisual(zone.kind,zone.position,.85F,zone.radius/(zone.kind==BossFx::FrostThorn?38:250));
            zone.visualStarted=true;
        }
    }
}
bool StoryBoss::HitExpansionObject(Bullet& bullet,Vector2 previous) {
    if(!Active()||bullet.hasHit||bullet.lifetime<=0||bullet.activationDelay>0)return false;
    const auto consume=[&](){bullet.hasHit=true;if(bullet.kind!=BulletKind::MeleeSlash)bullet.lifetime=0;};
    for(auto& mine:mines_)if(mine.remote&&mine.age<12&&
        SweptObject(previous,bullet.position,{mine.position.x,mine.position.y-18},20,bullet.radius)) {
        mine.durability-=std::max(.5F,bullet.damage);consume();
        if(mine.durability<=0){mine.age=13;mine.fuse=-1;AddVisual(BossFx::WMuzzle,mine.position,.5F);}
        return true;
    }
    for(auto& zone:zones_)if(zone.kind==BossFx::FrostPrism&&zone.delay>0&&zone.remaining>0&&
        SweptObject(previous,bullet.position,zone.position,zone.radius,bullet.radius)) {
        zone.durability-=std::max(.5F,bullet.damage);consume();
        if(zone.durability<=0){zone.remaining=0;AddVisual(BossFx::FrostTrail,zone.position,.7F);}
        return true;
    }
    return false;
}
