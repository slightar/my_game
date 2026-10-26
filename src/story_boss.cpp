#include "story_boss.h"
#include <algorithm>
#include <cmath>

namespace {
bool Sweep(Vector2 a,Vector2 b,Rectangle r,float radius,float* hitTime=nullptr) {
    float lo=0,hi=1;
    const float origin[]{a.x,a.y},delta[]{b.x-a.x,b.y-a.y};
    const float minimum[]{r.x-radius,r.y-radius},maximum[]{r.x+r.width+radius,r.y+r.height+radius};
    for(int axis=0;axis<2;++axis) {
        if(std::abs(delta[axis])<.0001F) {if(origin[axis]<minimum[axis]||origin[axis]>maximum[axis])return false;}
        else {
            float first=(minimum[axis]-origin[axis])/delta[axis],last=(maximum[axis]-origin[axis])/delta[axis];
            if(first>last)std::swap(first,last);
            lo=std::max(lo,first);hi=std::min(hi,last);if(lo>hi)return false;
        }
    }
    if(hitTime)*hitTime=lo;
    return true;
}
}
StoryBossKind StoryBoss::ForRegion(std::string_view region) {
    if(region=="bridge")return StoryBossKind::Crownslayer;
    if(region=="wtower")return StoryBossKind::W;
    if(region=="ice")return StoryBossKind::FrostNova;
    if(region=="industry")return StoryBossKind::Patriot;
    return StoryBossKind::None;
}
void StoryBoss::Reset(){cleared_.clear();Enter("",0);}
void StoryBoss::Enter(std::string_view region,float width,CombatDifficulty difficulty) {
    difficulty_=difficulty;
    region_=region;kind_=ForRegion(region);state_=cleared_.contains(region_)?StoryBossState::Defeated:StoryBossState::Dormant;
    trigger_=width*.36F;left_=80;right_=std::max(left_+80,width-80);
    feet_={std::clamp(trigger_+470,left_,std::max(left_,right_)),GameConfig::kFloorY};
    if(kind_==StoryBossKind::Crownslayer) {ResetCrown();if(Defeated())crown_.TakeDamage(MaxHealth());}
    health_=Defeated()?0:MaxHealth();facing_=-1;phaseTwo_=false;moving_=false;
    timer_=stateTime_=animation_=stun_=flash_=0;strikeConsumed_=false;
    stateDuration_=0;
    recoveryFrom_=StoryBossState::Hunt;
    if(Defeated())animation_=stateTime_=4;
    skillCooldown_=kind_==StoryBossKind::W?6.0F:1.2F;
    evadeCooldown_=1.2F;mineCooldown_=firePressure_=0;evadeStart_=evadeTarget_=feet_.x;
    fallVelocity_=0;
    attackSerial_=volleyFired_=0;strafeDirection_=1;volleyCount_=3;mineCount_=2;
    lowArcVolley_=false;barrageSerial_=rainSerial_=rainPattern_=0;
    frostNeedsBasic_=false;frostSkillCursor_=0;
    frostFanCooldown_=6;spearRainCooldown_=15;
    mobilityCooldown_=4;mobilityStart_=mobilityTarget_=feet_.x;mobilityTrail_=0;
    frostWeather_=weatherTime_=0;
    grenades_.clear();blasts_.clear();mines_.clear();
    missiles_.clear();zones_.clear();visuals_.clear();
    invulnerability_=0;ringCooldown_=3;sealCooldown_=6;waveCooldown_=3;ritualTimer_=0;
    chargeCooldown_=kind_==StoryBossKind::Patriot?1.0F:3.0F;chargeStart_=chargeTarget_=feet_.x;
    rainCooldown_=5.5F;guardCooldown_=5.0F;guardIntegrity_=guardPulse_=guardPressure_=0;
    stillTime_=0;lastPlayerX_=0;
    comboFired_=0;comboCount_=4;frostCastSerial_=0;actionRevision_=0;elapsed_=0;
    expansionCooldown_={10,18};
    remoteMineCount_=3;
}
void StoryBoss::SetState(StoryBossState state,float duration) {
    ++actionRevision_;
    if(state==StoryBossState::VolleyWindup)volleyCount_=phaseTwo_?5:3;
    if(state==StoryBossState::MineWindup)mineCount_=phaseTwo_?3:2;
    if(state==StoryBossState::Recover)recoveryFrom_=state_;
    state_=state;timer_=stateDuration_=duration;stateTime_=animation_=0;strikeConsumed_=false;moving_=false;
}
void StoryBoss::StartSandbox(StoryBossKind kind,float width,float x,CombatDifficulty difficulty) {
    Reset();Enter(StoryBossRegion(kind),width,difficulty);
    if(!Present())return;
    left_=80;right_=std::max(left_,width-80);trigger_=left_;
    feet_.x=std::clamp(x,left_+35,right_-35);
    if(kind_==StoryBossKind::Crownslayer)ResetCrown();
    SetState(StoryBossState::Hunt,.5F);
}
bool StoryBoss::TryStart(float x) {
    if(!Present()||state_!=StoryBossState::Dormant||x<trigger_)return false;
    SetState(StoryBossState::Arrival,TwoLives()?1.6F:.75F);return true;
}
bool StoryBoss::Targetable() const {
    if(kind_==StoryBossKind::Crownslayer)return Active()&&state_!=StoryBossState::Arrival;
    return Active()&&state_!=StoryBossState::Arrival&&state_!=StoryBossState::Revive&&invulnerability_<=0&&state_!=StoryBossState::Disappear&&
        !(state_==StoryBossState::Appear&&stateTime_<.20F);
}
Rectangle StoryBoss::Hitbox() const {
    if(kind_==StoryBossKind::Crownslayer){const auto p=crown_.Position();const float r=crown_.Radius();return {p.x-r,p.y-r,r*2,r*2};}
    return kind_==StoryBossKind::Patriot?Rectangle{feet_.x-42,feet_.y-158,84,158}:Rectangle{feet_.x-26,feet_.y-108,52,108};}
void StoryBoss::Update(float dt,Vector2 player,int playerFacing) {
    playerFacing_=playerFacing;
    crown_.BeginAttackFrame();
    for(auto& grenade:grenades_)grenade.previous=grenade.position;
    for(auto& missile:missiles_)missile.previous=missile.position;
    float remaining=std::clamp(dt,0.0F,.25F);
    while(remaining>.000001F){const float step=std::min(remaining,1.0F/120);Tick(step,player);remaining-=step;}
}
void StoryBoss::Tick(float dt,Vector2 player) {
    animation_+=dt;stateTime_+=dt;elapsed_+=dt;moving_=false;flash_=std::max(0.0F,flash_-dt);
    if(TwoLives()){TickWinter(dt,player);return;}
    if(kind_==StoryBossKind::Crownslayer) {
        if(state_==StoryBossState::Dormant)return;
        if(state_==StoryBossState::Arrival) {
            timer_-=dt;
            if(timer_<=0)SetState(StoryBossState::Hunt,0);
            return;
        }
        crown_.Update(dt,player,playerFacing_,true);
        SyncCrown();
        return;
    }
    if(!Active())return;
    for(auto& visual:visuals_)visual.age+=dt;
    std::erase_if(visuals_,[](const auto& visual){return visual.age>=visual.duration;});
    for(auto& mine:mines_) {
        if(mine.age>=12)continue;
        mine.age+=dt;
        if(mine.fuse<0&&mine.age>=.8F&&std::abs(player.x-mine.position.x)<92&&player.y>GameConfig::kFloorY-145)
            mine.fuse=.8F;
        else if(mine.fuse>=0) {
            mine.fuse-=dt;
            if(mine.fuse<=0){blasts_.push_back({mine.position,84});mine.age=13;}
        }
    }
    std::erase_if(mines_,[](const auto& m){return m.age>=12;});
    for(auto& blast:blasts_) {
        if(blast.tracking>0) {
            blast.position.x=std::clamp(player.x+blast.offset,left_+40,right_-40);
            blast.tracking=std::max(0.0F,blast.tracking-dt);
        }
        if(blast.timer>0)blast.timer=std::max(0.0F,blast.timer-dt);
        else blast.remaining-=dt;
        if(blast.timer<=0&&!blast.visualStarted) {
            AddVisual(blast.c4?BossFx::WC4Explosion:BossFx::WExplosion,blast.position,blast.c4?4.0F:1.5F,blast.radius/85);
            blast.visualStarted=true;
        }
    }
    std::erase_if(blasts_,[](const auto& b){return b.remaining<=0;});
    for(auto& grenade:grenades_) {
        if(grenade.lifetime<=0)continue;
        grenade.age+=dt;
        grenade.velocity.y+=grenade.gravity*dt;
        grenade.position.x+=grenade.velocity.x*dt;grenade.position.y+=grenade.velocity.y*dt;grenade.lifetime-=dt;
        if(grenade.position.y>=GameConfig::kFloorY-4) {
            blasts_.push_back({{grenade.position.x,GameConfig::kFloorY-4},72});grenade.lifetime=0;
        }
    }
    std::erase_if(grenades_,[&](const auto& g){return g.lifetime<=0||g.position.x<left_-100||g.position.x>right_+100;});
    if(state_!=StoryBossState::Evade&&feet_.y<GameConfig::kFloorY) {
        fallVelocity_+=1100*dt;feet_.y=std::min(GameConfig::kFloorY,feet_.y+fallVelocity_*dt);
    }
    if(stun_>0){stun_=std::max(0.0F,stun_-dt);return;}
    evadeCooldown_=std::max(0.0F,evadeCooldown_-dt);mineCooldown_=std::max(0.0F,mineCooldown_-dt);
    firePressure_=std::max(0.0F,firePressure_-2*dt);
    skillCooldown_=std::max(0.0F,skillCooldown_-dt);timer_-=dt;
    for(auto& cooldown:expansionCooldown_)cooldown=std::max(0.0F,cooldown-dt);
    const float dx=player.x-feet_.x,distance=std::abs(dx);
    switch(state_) {
    case StoryBossState::Arrival:
        if(timer_<=0)SetState(StoryBossState::Hunt,.5F);
        break;
    case StoryBossState::Hunt:
        facing_=dx>=0?1:-1;
        if(timer_<=0&&expansionCooldown_[0]<=0&&distance<800)StartAirburst(player);
        else if(timer_<=0&&expansionCooldown_[1]<=0&&distance<800)StartRemoteMines(player);
        else if(distance<720&&skillCooldown_<=0&&timer_<=0)StartC4(player);
        else if(WantsEvade(player))StartEvade(player);
        else {
            MoveW(dt,player);
            if(distance<650&&timer_<=0) {
                aim_={std::clamp(player.x,left_+40,right_-40),GameConfig::kFloorY-4};
                const int pattern=attackSerial_++%5;
                lowArcVolley_=pattern==4;
                if(pattern==1||pattern==4)SetState(StoryBossState::VolleyWindup,.85F);
                else if(pattern==2&&mineCooldown_<=0)SetState(StoryBossState::MineWindup,.70F);
                else if(pattern==3)StartBarrage(player);
                else SetState(StoryBossState::GrenadeWindup,.85F);
            }
        }
        break;
    case StoryBossState::AirburstWindup:
        if(timer_<=0)SetState(StoryBossState::Airburst,2.0F);
        break;
    case StoryBossState::Airburst:
        if(timer_<=0)SetState(StoryBossState::Recover,1.1F*difficulty_.recovery);
        break;
    case StoryBossState::RemoteMineWindup:
        if(timer_<=0){DeployRemoteMines();SetState(StoryBossState::RemoteMine,2.9F);}
        break;
    case StoryBossState::RemoteMine:
        if(timer_<=0)SetState(StoryBossState::Recover,1.1F*difficulty_.recovery);
        break;
    case StoryBossState::GrenadeWindup:
        if(timer_<=0){LaunchGrenade(aim_);SetState(StoryBossState::Recover,1.20F*difficulty_.recovery);}
        break;
    case StoryBossState::VolleyWindup:
        if(timer_<=0){volleyFired_=0;SetState(StoryBossState::Volley,VolleyCount()*.24F);}
        break;
    case StoryBossState::Volley: {
        MoveW(dt,player);
        const auto targets=GrenadeTargets();
        while(volleyFired_<int(targets.size())&&stateTime_>=volleyFired_*.24F)
            LaunchGrenade({targets[volleyFired_++],GameConfig::kFloorY-4});
        if(timer_<=0)SetState(StoryBossState::Recover,.85F*difficulty_.recovery);
        break;
    }
    case StoryBossState::MineWindup:
        if(timer_<=0) {
            for(float target:MineTargets())AddMine(target);
            mineCooldown_=8*difficulty_.cooldown;SetState(StoryBossState::Recover,.85F*difficulty_.recovery);
        }
        break;
    case StoryBossState::BarrageWindup:
        if(timer_<=0)SetState(StoryBossState::Barrage,1.8F);
        break;
    case StoryBossState::Barrage:
        if(timer_<=0)SetState(StoryBossState::Recover,1.0F*difficulty_.recovery);
        break;
    case StoryBossState::EvadeWindup:
        if(timer_<=0) {
            AddMine(feet_.x);evadeStart_=feet_.x;SetState(StoryBossState::Evade,.72F);
        }
        break;
    case StoryBossState::Evade: {
        const float t=std::clamp(stateTime_/.72F,0.0F,1.0F),ease=t*t*(3-2*t);
        feet_={evadeStart_+(evadeTarget_-evadeStart_)*ease,GameConfig::kFloorY-std::sin(t*3.14159265F)*125};
        moving_=true;
        if(timer_<=0) {
            feet_={evadeTarget_,GameConfig::kFloorY};facing_=player.x>=feet_.x?1:-1;
            if(skillCooldown_<=0&&std::abs(player.x-feet_.x)<720){StartC4(player);break;}
            aim_={std::clamp(player.x,left_+40,right_-40),GameConfig::kFloorY-4};
            const int counterPattern=attackSerial_++%5;
            if(counterPattern==3){StartBarrage(player);break;}
            lowArcVolley_=counterPattern==4;
            SetState(StoryBossState::VolleyWindup,.85F);
        }
        break;
    }
    case StoryBossState::C4:
        if(timer_<=0)SetState(StoryBossState::Recover,.90F*difficulty_.recovery);
        break;
    case StoryBossState::Recover:
        if(kind_==StoryBossKind::W) {
            if(WantsEvade(player)&&skillCooldown_>0){StartEvade(player);break;}
            MoveW(dt,player);
        }
        if(timer_<=0)SetState(StoryBossState::Hunt,.15F);
        break;
    default:break;
    }
}
void StoryBoss::MoveW(float dt,Vector2 player) {
    const float dx=player.x-feet_.x,distance=std::abs(dx);
    facing_=dx>=0?1:-1;
    if(distance<340)strafeDirection_=1;
    else if(distance>500)strafeDirection_=-1;
    const float direction=-facing_*strafeDirection_;
    const float next=std::clamp(feet_.x+direction*185*difficulty_.movement*dt,left_+35,right_-35);
    moving_=next!=feet_.x;feet_.x=next;
}
bool StoryBoss::WantsEvade(Vector2 player) const {
    const bool cornered=std::abs(player.x-feet_.x)<335&&
        ((feet_.x>=right_-45&&player.x<feet_.x)||(feet_.x<=left_+45&&player.x>feet_.x));
    return evadeCooldown_<=0&&(std::abs(player.x-feet_.x)<215||firePressure_>=6||
        cornered||(phaseTwo_&&attackSerial_>=3&&std::abs(player.x-feet_.x)<720));
}
void StoryBoss::StartEvade(Vector2 player) {
    const float away=player.x>=feet_.x?-1.0F:1.0F;
    evadeTarget_=std::clamp(feet_.x+away*(phaseTwo_?420.0F:340.0F),left_+40,right_-40);
    // Keep the destination within the player's camera/combat reach. A distant
    // shooter provokes a vault across them rather than an off-screen retreat.
    const float maxRange=phaseTwo_?520.0F:500.0F;
    if(std::abs(evadeTarget_-player.x)>maxRange)
        evadeTarget_=std::clamp(player.x+away*maxRange,left_+40,right_-40);
    // A cornered W vaults to the open side instead of repeatedly backing into a wall.
    if(std::abs(evadeTarget_-feet_.x)<220)
        evadeTarget_=std::clamp(player.x-away*340.0F,left_+40,right_-40);
    facing_=evadeTarget_>feet_.x?1:-1;
    firePressure_=0;evadeCooldown_=(phaseTwo_?4.3F:5.5F)*difficulty_.cooldown;
    SetState(StoryBossState::EvadeWindup,.25F);
}
void StoryBoss::AddMine(float x) {
    const Vector2 position{std::clamp(x,left_+40,right_-40),GameConfig::kFloorY-4};
    // Do not stack nearly overlapping traps, including clamped edge positions.
    for(const auto& mine:mines_)if(std::abs(mine.position.x-position.x)<100)return;
    if(mines_.size()>=4)mines_.erase(mines_.begin());
    mines_.push_back({position});
}
std::vector<float> StoryBoss::GrenadeTargets() const {
    std::vector<float> targets;
    for(int i=0;i<VolleyCount();++i) {
        const float x=std::clamp(aim_.x+(i-VolleyCount()/2)*125.0F,left_+40,right_-40);
        if(targets.empty()||std::abs(targets.back()-x)>10)targets.push_back(x);
    }
    return targets;
}
std::vector<float> StoryBoss::MineTargets() const {
    if(mineCount_==3)return {std::clamp(aim_.x-180,left_+40,right_-40),aim_.x,std::clamp(aim_.x+180,left_+40,right_-40)};
    return {std::clamp(aim_.x-150,left_+40,right_-40),std::clamp(aim_.x+150,left_+40,right_-40)};
}
void StoryBoss::StartC4(Vector2 player) {
    SetState(StoryBossState::C4,3.2F);skillCooldown_=11*difficulty_.cooldown;
    for(int index=0;index<(phaseTwo_?3:1);++index) {
        const float offset=phaseTwo_?(index-1)*190.0F:0;
        blasts_.push_back({{std::clamp(player.x+offset,left_+40,right_-40),GameConfig::kFloorY-4},90,3.2F,1.7F,offset,.35F,true});
    }
}
void StoryBoss::StartBarrage(Vector2 player) {
    const int pairs=phaseTwo_?3:2;
    const float spacing=std::min(150.0F,(right_-left_-110)/(2*(pairs+1)));
    const float span=pairs*spacing;
    aim_={std::clamp(player.x,left_+55+span,right_-55-span),GameConfig::kFloorY-4};
    const bool reverse=barrageSerial_++%2!=0;
    for(int i=0;i<pairs*2;++i) {
        const int index=reverse?pairs*2-1-i:i;
        const int lane=index<pairs?index-pairs:index-pairs+1;
        blasts_.push_back({{aim_.x+lane*spacing,aim_.y},std::min(54.0F,spacing*.32F),
                           1.1F+i*.24F,0,0,.35F});
    }
    SetState(StoryBossState::BarrageWindup,1.1F);
}
void StoryBoss::LaunchGrenade(Vector2 target) {
    const Vector2 origin{feet_.x+facing_*26.0F,feet_.y-68};
    const float flight=std::clamp(std::abs(target.x-origin.x)/440.0F,.65F,1.30F)/difficulty_.projectile;
    BossGrenade grenade;grenade.position=grenade.previous=origin;grenade.landing=target;
    if(lowArcVolley_)grenade.gravity=260;
    grenade.velocity={(target.x-origin.x)/flight,(target.y-origin.y-.5F*grenade.gravity*flight*flight)/flight};
    grenades_.push_back(grenade);
    AddVisual(BossFx::WMuzzle,origin,1);
}
bool StoryBoss::Damage(float amount,float stun,DamageType type) {
    if(!Targetable()||amount<=0)return false;
    if(kind_==StoryBossKind::Crownslayer) {
        crown_.TakeDamage(amount);crown_.Stun(stun);flash_=.14F;SyncCrown();
        if(crown_.IsDefeated())cleared_.insert(region_);
        return true;
    }
    if(kind_==StoryBossKind::Patriot)amount*=phaseTwo_?(type==DamageType::Arts?.80F:.70F):(type==DamageType::Arts?.65F:.35F);
    health_=std::max(0.0F,health_-amount);flash_=.14F;
    if(health_<=0) {
        if(TwoLives()&&!phaseTwo_){BeginRevival();return true;}
        feet_.y=GameConfig::kFloorY;
        SetState(StoryBossState::Defeated,0);cleared_.insert(region_);grenades_.clear();blasts_.clear();mines_.clear();visuals_.clear();
        missiles_.clear();zones_.clear();visuals_.clear();stun_=0;return true;
    }
    if(kind_==StoryBossKind::W&&!phaseTwo_&&health_<=MaxHealth()*.5F){phaseTwo_=true;skillCooldown_=0;}
    if(kind_==StoryBossKind::W)firePressure_=std::min(12.0F,firePressure_+std::min(amount,4.0F));
    if(kind_==StoryBossKind::Patriot&&state_!=StoryBossState::Guard&&state_!=StoryBossState::GuardWindup) {
        guardPressure_+=std::min(amount,2.5F);
        if(guardPressure_>=6.0F){guardPressure_=0;guardCooldown_=std::min(guardCooldown_,.1F);}
    }
    if(stun>0&&kind_!=StoryBossKind::FrostNova&&!(kind_==StoryBossKind::Patriot&&phaseTwo_)) {
        // Original W's interrupted C4 detonates immediately; markers remain readable.
        if(state_==StoryBossState::C4)for(auto& blast:blasts_)if(blast.c4){blast.timer=blast.tracking=0;}
        if(state_==StoryBossState::Evade){fallVelocity_=0;evadeTarget_=feet_.x;}
        if(state_==StoryBossState::Leap)feet_.y=GameConfig::kFloorY;
        stun_=std::max(stun_,std::min(stun,.8F));SetState(StoryBossState::Recover,.65F);
    }
    return true;
}
bool StoryBoss::ResolveBullet(Bullet& bullet,Vector2 previous) {
    if(HitExpansionObject(bullet,previous))return true;
    if(!Targetable()||bullet.hasHit||bullet.lifetime<=0||bullet.activationDelay>0)return false;
    const bool melee=bullet.kind==BulletKind::MeleeSlash;
    if(!(melee?CheckCollisionCircleRec(bullet.position,bullet.radius,Hitbox()):Sweep(previous,bullet.position,Hitbox(),bullet.radius)))return false;
    float damage=bullet.damage;
    if(kind_==StoryBossKind::Patriot&&state_==StoryBossState::Guard&&
       bullet.velocity.x*facing_<-1.0F) {
        guardIntegrity_-=std::max(.5F,bullet.damage);
        damage*=.25F;
        if(guardIntegrity_<=0){SetState(StoryBossState::Recover,1.0F);AddVisual(BossFx::Impact,feet_,.75F);}
    }
    if(!Damage(damage,bullet.stunDuration,bullet.damageType))return false;
    bullet.impactPoint=Position();bullet.hasImpact=true;
    bullet.impactEvents.push_back({Position(),Defeated()});
    bullet.hasHit=true;if(!melee)bullet.lifetime=0;return true;
}
bool StoryBoss::BreakSeal(Bullet& bullet,Vector2 previous) {
    if(kind_!=StoryBossKind::FrostNova||!Active()||bullet.hasHit||bullet.lifetime<=0||bullet.activationDelay>0||
       !std::isfinite(bullet.damage)||bullet.damage<=0)return false;
    for(auto& zone:zones_)if(zone.kind==BossFx::BlackIce&&zone.delay<=0&&zone.remaining>0) {
        const Rectangle target{zone.position.x-zone.radius,GameConfig::kFloorY-92,zone.radius*2,104};
        if(!Sweep(previous,bullet.position,target,bullet.radius))continue;
        zone.durability=std::max(0.0F,zone.durability-bullet.damage);
        bullet.hasHit=true;
        if(bullet.kind!=BulletKind::MeleeSlash)bullet.lifetime=0;
        if(zone.durability<=0) {
            zone.remaining=0;
            AddVisual(BossFx::IceRing,zone.position,.55F,.35F);
        }
        return true;
    }
    return false;
}
bool StoryBoss::DestroyProjectile(Vector2 p,float radius) {
    if(kind_==StoryBossKind::Crownslayer)return crown_.DestroyProjectileAt(p,radius);
    for(auto& mine:mines_)if(mine.remote&&mine.age<12&&CheckCollisionCircles(p,radius,{mine.position.x,mine.position.y-18},22)) {
        mine.age=13;mine.fuse=-1;AddVisual(BossFx::WMuzzle,mine.position,.5F);return true;
    }
    for(auto& z:zones_)if(z.kind==BossFx::FrostPrism&&z.delay>0&&z.remaining>0&&
        CheckCollisionCircles(p,radius,z.position,26)) {
        z.remaining=0;AddVisual(BossFx::FrostTrail,z.position,.7F);return true;
    }
    for(auto& missile:missiles_)if(missile.kind==BossFx::FrostBolt&&missile.life>0&&CheckCollisionCircles(p,radius,missile.position,12)){missile.life=0;return true;}
    for(auto& g:grenades_)if(g.lifetime>0&&CheckCollisionCircles(p,radius,g.position,7)){g.lifetime=0;return true;}
    return false;
}
bool StoryBoss::AttackHits(Rectangle body,Rectangle projectileBody,Vector2& source,BossImpact* impact) {
    if(impact)*impact={};
    if(!Active())return false;
    if(kind_==StoryBossKind::Crownslayer) {
        if(state_==StoryBossState::Arrival)return false;
        source=crown_.Position();
        return (crown_.CanDealContactDamage()&&CheckCollisionCircleRec(source,crown_.Radius(),body))||crown_.AttackHits(body,projectileBody);
    }
    if(TwoLives())return WinterAttackHits(body,projectileBody,source,impact);
    for(auto& grenade:grenades_) {
        float hitTime=0;
        if(grenade.lifetime<=0||!Sweep(grenade.previous,grenade.position,projectileBody,7,&hitTime))continue;
        // Check the entire flight segment so fast grenades cannot skip the player.
        source={grenade.previous.x+(grenade.position.x-grenade.previous.x)*hitTime,
                grenade.previous.y+(grenade.position.y-grenade.previous.y)*hitTime};
        grenade.lifetime=0;
        AddVisual(BossFx::WExplosion,source,1.5F,72.0F/85);
        return true;
    }
    for(auto& blast:blasts_)if(blast.timer<=0&&blast.remaining>0&&!blast.consumed&&CheckCollisionCircleRec(blast.position,blast.radius,body)) {
        blast.consumed=true;source=blast.position;return true;
    }
    return false;
}
std::string StoryBoss::Glyphs() {
    return "赤影处刑三重瞬斩终结下斩离开红圈线锁定后移动闪避落地反击后撤扇形飞刀冰刃蓄力连环爆破寻找安全空隙低空榴弹落矛阵顺序会变化冲刺突进空中斩击飞刀投掷半血强化领袖遭遇弑君者闪现绕后近身斩击保持机动预警榴弹锁定落点炸药包追踪固定爆破半血强化击败解除封锁重试肃清战斗中无法进入通道素材缺失目标确认她会穿过你的防线留意身后三枚数量增加前方检测到后继续前进连射换位后跳感应地雷布设待激活警戒跃过玩家反击与火力增强双阶段召唤叠加仍会将释放与蓄力护盾范围内重新冰晶横版适配霜星冬痕冰环寒冷冻结黑冰封印重生寒潮爱国者行军毁灭姿态四连击投枪生命献祭护甲免疫眩晕阶段倒地回满无敌即将结束避开范围危险区域真正结束远离等待蓄力光环原作专场钢铁残阳瞄准正在第二条生命尚未解除封锁生命解放寒潮与三重黑冰第一阶段寒冷与冰环第二阶段毁灭投枪与献祭高防御四连击地板攻击修复冲锋跳跃闪避齐射冰雨离开落点盾阵绕后背击破盾地面冲击波";
}

void StoryBoss::ResetCrown() {
    const Rectangle arena{left_,GameConfig::kBossArena.y,right_-left_,GameConfig::kBossArena.height};
    crown_.Reset(arena,{feet_.x,feet_.y-34.0F},MaxHealth());
}
void StoryBoss::SyncCrown() {
    using S=Boss::State;
    StoryBossState next=StoryBossState::Hunt;
    switch(crown_.CurrentState()) {
    case S::Idle:break;
    case S::MeleeWindup:next=StoryBossState::MeleeWindup;break;
    case S::Melee:next=StoryBossState::Strike;break;
    case S::RushTeleportOut:next=StoryBossState::Disappear;break;
    case S::RushTeleportIn:next=StoryBossState::Appear;break;
    case S::RushWindup:next=StoryBossState::RushWindup;break;
    case S::Rush:next=StoryBossState::Rush;break;
    case S::AirSlashWindup:next=StoryBossState::AirSlashWindup;break;
    case S::AirSlash:next=StoryBossState::AirSlash;break;
    case S::DartWindup:next=StoryBossState::DartWindup;break;
    case S::FanRetreat:next=StoryBossState::FanRetreat;break;
    case S::ShadowWindup:next=StoryBossState::ShadowWindup;break;
    case S::ShadowSlash:next=StoryBossState::ShadowSlash;break;
    case S::ShadowPause:next=StoryBossState::ShadowPause;break;
    case S::ShadowFinalWindup:next=StoryBossState::ShadowFinalWindup;break;
    case S::ShadowFinal:next=StoryBossState::ShadowFinal;break;
    case S::MirrorWindup:next=StoryBossState::MirrorWindup;break;
    case S::MirrorVolley:next=StoryBossState::MirrorVolley;break;
    case S::AmbushWindup:next=StoryBossState::AmbushWindup;break;
    case S::Ambush:next=StoryBossState::Ambush;break;
    case S::Recover:next=StoryBossState::Recover;break;
    case S::Defeated:next=StoryBossState::Defeated;break;
    }
    if(state_!=next)SetState(next,0);
    const auto p=crown_.Position();feet_={p.x,p.y+crown_.Radius()};
    health_=crown_.Health();facing_=crown_.Facing();animation_=crown_.AnimationTime();
    stun_=crown_.StunTime();blinkX_=crown_.BlinkTarget();phaseTwo_=health_<=MaxHealth()*.5F;
    moving_=next==StoryBossState::Rush||next==StoryBossState::AirSlash||next==StoryBossState::FanRetreat;
    if(next==StoryBossState::MirrorWindup||next==StoryBossState::MirrorVolley||
       next==StoryBossState::AmbushWindup||next==StoryBossState::Ambush)aim_=crown_.EchoAim();
}
