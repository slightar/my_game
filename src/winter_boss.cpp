#include "story_boss.h"
#include <algorithm>
#include <cmath>
namespace {constexpr float kSealWarning=2.4F,kSealLifetime=18.0F,kSealFirstHealth=4,kSealRevivedHealth=8;}

const char* StoryBossName(StoryBossKind k) {
    switch(k){case StoryBossKind::Crownslayer:return "弑君者";case StoryBossKind::W:return "W";
    case StoryBossKind::FrostNova:return "霜星 · 冬痕";case StoryBossKind::Patriot:return "爱国者";default:return "--";}
}
const char* StoryBossId(StoryBossKind k) {
    switch(k){case StoryBossKind::Crownslayer:return "crownslayer";case StoryBossKind::W:return "w";
    case StoryBossKind::FrostNova:return "frostnova";case StoryBossKind::Patriot:return "patriot";default:return "";}
}
const char* StoryBossRegion(StoryBossKind k) {
    switch(k){case StoryBossKind::Crownslayer:return "bridge";case StoryBossKind::W:return "wtower";
    case StoryBossKind::FrostNova:return "ice";case StoryBossKind::Patriot:return "industry";default:return "";}
}
const char* StoryBoss::Briefing() const {
    switch(kind_) {
    case StoryBossKind::FrostNova:return "目标确认：霜星。变换站位避开攻击；留意落点，地板可攻击修复。";
    case StoryBossKind::Patriot:return "目标确认：爱国者。绕后或集中火力击破护盾；第二阶段避开地面标记。";
    case StoryBossKind::W:return "目标确认：W。爆炸范围之间留有空隙；跳跃避开低空攻击。半血后火力增强。";
    default:return "目标确认：弑君者。离开红线与红圈，落地后反击。";
    }
}
void StoryBoss::AddVisual(BossFx kind,Vector2 position,float duration,float scale) {
    if(visuals_.size()>=72)visuals_.erase(visuals_.begin());
    visuals_.push_back({kind,position,0,duration,scale,facing_});
}
void StoryBoss::BeginRevival() {
    feet_.y=GameConfig::kFloorY;
    missiles_.clear();visuals_.clear();stun_=0;invulnerability_=0;
    // Black ice persists between FrostNova's lives; transient attack zones do not.
    std::erase_if(zones_,[](const auto& z){return z.kind!=BossFx::BlackIce;});
    ritualTimer_=.9F;SetState(StoryBossState::Revive,RevivalDuration());
    AddVisual(BossFx::Revival,feet_,RevivalDuration(),1);
}
void StoryBoss::FinishRevival() {
    phaseTwo_=true;health_=MaxHealth();invulnerability_=kind_==StoryBossKind::FrostNova?3.0F:2.5F;
    if(kind_==StoryBossKind::FrostNova)for(auto& zone:zones_)
        if(zone.kind==BossFx::BlackIce&&zone.remaining>0) {
            // Harden surviving pits while preserving damage already dealt to them.
            zone.durability+=kSealRevivedHealth-zone.maxDurability;
            zone.maxDurability=kSealRevivedHealth;
        }
    ringCooldown_=1.2F;sealCooldown_=3;rainCooldown_=2.8F;waveCooldown_=1;
    skillCooldown_=2;guardCooldown_=4;guardIntegrity_=0;ritualTimer_=0;
    frostFanCooldown_=4;spearRainCooldown_=7;frostSkillCursor_=0;frostNeedsBasic_=false;
    mobilityCooldown_=3;
    expansionCooldown_={6,11};
    SetState(StoryBossState::Hunt,.7F);AddVisual(BossFx::Revival,feet_,1.8F,1.4F);
}
void StoryBoss::TickWinterHazards(float dt,Vector2 player) {
    for(auto& fx:visuals_)fx.age+=dt;
    std::erase_if(visuals_,[](const auto& fx){return fx.age>=fx.duration;});
    for(auto& zone:zones_) {
        zone.age+=dt;
        if(zone.delay>0)zone.delay=std::max(0.0F,zone.delay-dt);
        else zone.remaining-=dt;
        if(zone.remaining>0&&zone.kind==BossFx::SpearRain&&zone.delay<=0&&!zone.consumed) {
            const Vector2 start{zone.position.x,GameConfig::kFloorY-430};
            missiles_.push_back({BossFx::Spear,start,start,{0,850*difficulty_.projectile},1.0F});
            zone.consumed=true;
        }
    }
    std::erase_if(zones_,[](const auto& z){return z.remaining<=0;});
    TickExpansionHazards();
    for(auto& m:missiles_) {
        if(m.life<=0)continue;
        m.age+=dt;m.life-=dt;m.position.x+=m.velocity.x*dt;m.position.y+=m.velocity.y*dt;
        if((m.kind==BossFx::Spear||m.kind==BossFx::SpearLance) && (m.position.y>=GameConfig::kFloorY || m.position.x<left_ || m.position.x>right_)) {
            const Vector2 landing{std::clamp(m.position.x,left_,right_),std::min(m.position.y,GameConfig::kFloorY)};
            zones_.push_back({BossFx::Impact,landing,65,0,.4F});AddVisual(BossFx::Impact,landing,.8F);m.life=0;
        }
    }
    std::erase_if(missiles_,[&](const auto& m){return m.life<=0||m.position.x<left_-120||m.position.x>right_+120;});
    if(kind_==StoryBossKind::Patriot&&(phaseTwo_||state_==StoryBossState::Revive)&&
       state_!=StoryBossState::LeapWindup&&state_!=StoryBossState::Leap&&
       state_!=StoryBossState::QuakeWindup&&state_!=StoryBossState::Quake&&
       state_!=StoryBossState::LanceWindup&&state_!=StoryBossState::Lance) {
        ritualTimer_-=dt;
        if(ritualTimer_<=0) {
            ritualTimer_=.8F;
            zones_.push_back({BossFx::Ritual,feet_,205,.16F,.42F});
            AddVisual(BossFx::Ritual,feet_,.8F);
        }
    }
    if(kind_==StoryBossKind::FrostNova&&phaseTwo_&&state_!=StoryBossState::Revive) {
        waveCooldown_-=dt;
        if(waveCooldown_<=0) {
            waveCooldown_=8*difficulty_.cooldown;
            // The original phase-two ice crystals send low cold waves across the lane.
            const int pattern=attackSerial_++%3;
            for(int side=0;side<(pattern==2?2:1);++side) {
                const bool fromLeft=pattern==2?side==0:pattern==0;
                const Vector2 at{fromLeft?left_:right_,GameConfig::kFloorY-35};
                missiles_.push_back({BossFx::ColdWave,at,at,{fromLeft?210.0F:-210.0F,0},(right_-left_)/210+1});
            }
        }
    }
}
void StoryBoss::LaunchFrost(Vector2 player) {
    const Vector2 start{feet_.x+facing_*28,feet_.y-64};
    const Vector2 delta{player.x-start.x,player.y-start.y};
    const float length=std::max(1.0F,std::hypot(delta.x,delta.y));
    const float speed=(phaseTwo_?440.0F:360.0F)*difficulty_.projectile;
    missiles_.push_back({BossFx::FrostBolt,start,start,{delta.x/length*speed,delta.y/length*speed},3});
    AddVisual(BossFx::FrostBolt,start,.55F);
}
std::vector<Vector2> StoryBoss::FrostFanVelocities() const {
    const Vector2 start{feet_.x+facing_*28,feet_.y-64};
    const float angle=std::atan2(aim_.y-start.y,aim_.x-start.x);
    const float speed=(phaseTwo_?410.0F:330.0F)*difficulty_.projectile;
    const int half=phaseTwo_?3:2;
    std::vector<Vector2> velocities;
    for(int i=-half;i<=half;++i)velocities.push_back({std::cos(angle+i*.18F)*speed,std::sin(angle+i*.18F)*speed});
    return velocities;
}
void StoryBoss::LaunchFrostFan() {
    const Vector2 start{feet_.x+facing_*28,feet_.y-64};
    for(auto velocity:FrostFanVelocities())missiles_.push_back({BossFx::FrostBolt,start,start,velocity,3});
    AddVisual(BossFx::FrostBolt,start,.7F);
}
void StoryBoss::AddSpearRain(Vector2 player) {
    constexpr float spacing=180;
    const float center=std::clamp(player.x,left_+55+spacing,right_-55-spacing);
    aim_={center,GameConfig::kFloorY};
    for(int i=0;i<3;++i)zones_.push_back({BossFx::SpearRain,{center+(i-1)*spacing,GameConfig::kFloorY},38,
                                      1.15F+i*.24F,.65F});
}
void StoryBoss::AddBlackIce(Vector2 player) {
    const int count=phaseTwo_?3:2;
    constexpr float tile=120.0F;
    for(int i=0;i<count;++i) {
        const float offset=phaseTwo_?(i-1)*165.0F:(i==0?-160.0F:160.0F);
        const float raw=std::clamp(player.x+offset,left_+tile/2,right_-tile/2);
        const float x=std::clamp(left_+tile*(std::floor((raw-left_)/tile)+.5F),left_+tile/2,right_-tile/2);
        bool overlap=false;for(const auto& z:zones_)if(z.kind==BossFx::BlackIce&&std::abs(z.position.x-x)<tile)overlap=true;
        if(overlap)continue;
        if(std::count_if(zones_.begin(),zones_.end(),[](const auto& z){return z.kind==BossFx::BlackIce;})>=6) {
            auto oldest=std::find_if(zones_.begin(),zones_.end(),[](const auto& z){return z.kind==BossFx::BlackIce;});
            zones_.erase(oldest);
        }
        const float hp=phaseTwo_?kSealRevivedHealth:kSealFirstHealth;
        BossZone seal{BossFx::BlackIce,{x,GameConfig::kFloorY},tile/2,kSealWarning,kSealLifetime,0,0,false,hp};
        seal.maxDurability=hp;zones_.push_back(seal);
    }
}
void StoryBoss::AddIceRain(Vector2 player) {
    const int count=phaseTwo_?5:3;
    const float step=phaseTwo_?125.0F:150.0F;
    const float halfSpan=(count-1)*step*.5F;
    const float center=std::clamp(player.x,left_+55+halfSpan,right_-55-halfSpan);
    for(int i=0;i<count;++i) {
        const float x=center+(i-count/2)*step;
        if(std::any_of(zones_.begin(),zones_.end(),[&](const BossZone& z){
            return z.kind==BossFx::IceRain&&std::abs(z.position.x-x)<70&&z.remaining>0;
        }))continue;
        const int rank=rainPattern_==0?i:rainPattern_==1?count-1-i:
            i==count/2?count-1:i<count/2?i*2:(count-1-i)*2+1;
        zones_.push_back({BossFx::IceRain,{x,GameConfig::kFloorY},50,1.25F+rank*.22F,.36F});
    }
}
void StoryBoss::LaunchGroundWave(float impactScale) {
    AddVisual(BossFx::Impact,feet_,.7F,impactScale);
    for(int side=0;side<(phaseTwo_?2:1);++side) {
        const float direction=side==0?float(facing_):-float(facing_);
        const Vector2 at{feet_.x+direction*45,GameConfig::kFloorY-23};
        missiles_.push_back({BossFx::GroundWave,at,at,{direction*(phaseTwo_?540.0F:440.0F),0},
            (right_-left_)/440+1});
    }
}
void StoryBoss::LaunchSpear() {
    const Vector2 start{feet_.x+facing_*40,feet_.y-135};
    const Vector2 d{aim_.x-start.x,aim_.y-start.y};const float len=std::max(1.0F,std::hypot(d.x,d.y));
    const float speed=900*difficulty_.projectile;
    missiles_.push_back({BossFx::Spear,start,start,{d.x/len*speed,d.y/len*speed},2.5F});
    AddVisual(BossFx::PatriotSlash,start,.5F);
}
void StoryBoss::TickWinter(float dt,Vector2 player) {
    if(kind_==StoryBossKind::FrostNova) {
        const float target=Active()?(phaseTwo_?1.0F:.72F):0.0F;
        const float rate=target>frostWeather_?.24F:.17F;
        frostWeather_+=std::clamp(target-frostWeather_,-rate*dt,rate*dt);
        if(frostWeather_>0)weatherTime_+=dt;
    }
    if(!Active())return;
    TickWinterHazards(dt,player);
    invulnerability_=std::max(0.0F,invulnerability_-dt);
    if(kind_==StoryBossKind::FrostNova) {
        stillTime_=std::abs(player.x-lastPlayerX_)<8.0F?stillTime_+dt:0.0F;
        lastPlayerX_=player.x;
        if(stillTime_>2.5F){rainCooldown_=std::min(rainCooldown_,3.0F);stillTime_=0;}
    }
    if(state_==StoryBossState::Revive) {
        timer_-=dt;if(timer_<=0)FinishRevival();return;
    }
    if(stun_>0){stun_=std::max(0.0F,stun_-dt);return;}
    ringCooldown_=std::max(0.0F,ringCooldown_-dt);sealCooldown_=std::max(0.0F,sealCooldown_-dt);
    rainCooldown_=std::max(0.0F,rainCooldown_-dt);guardCooldown_=std::max(0.0F,guardCooldown_-dt);
    chargeCooldown_=std::max(0.0F,chargeCooldown_-dt);
    mobilityCooldown_=std::max(0.0F,mobilityCooldown_-dt);
    frostFanCooldown_=std::max(0.0F,frostFanCooldown_-dt);
    spearRainCooldown_=std::max(0.0F,spearRainCooldown_-dt);
    for(auto& cooldown:expansionCooldown_)cooldown=std::max(0.0F,cooldown-dt);
    skillCooldown_=std::max(0.0F,skillCooldown_-dt);timer_-=dt;
    const float dx=player.x-feet_.x,distance=std::abs(dx);
    switch(state_) {
    case StoryBossState::Arrival:if(timer_<=0)SetState(StoryBossState::Hunt,.5F);break;
    case StoryBossState::Hunt:
        facing_=dx>=0?1:-1;
        if(kind_==StoryBossKind::FrostNova) {
            if(timer_<=0&&(distance>650||distance<200)&&StartFrostStep(player)) {
                // Escape pressure or close a long gap before resuming the casting rotation.
            } else if(timer_<=0&&frostNeedsBasic_&&distance<850) {
                frostNeedsBasic_=false;aim_=player;
                SetState(StoryBossState::FrostCast,frostCastSerial_%3==2?.98F:.8F);
            } else if(timer_<=0&&StartFrostSkill(player)) {
                // Rotate through ready skills instead of starving lower-priority casts.
            } else if(timer_<=0 && distance<850) {
                aim_=player;SetState(StoryBossState::FrostCast,frostCastSerial_%3==2?.98F:.8F);
            } else {
                float direction=distance>500?float(facing_):distance<180?-float(facing_):0;
                const float next=std::clamp(feet_.x+direction*(phaseTwo_?195:150)*difficulty_.movement*dt,left_+35,right_-35);
                moving_=next!=feet_.x;feet_.x=next;
            }
        } else {
            if(timer_<=0&&mobilityCooldown_<=0&&distance>430) {
                StartPatriotLeap(player);
            } else if(phaseTwo_&&timer_<=0&&spearRainCooldown_<=0&&distance>180) {
                AddSpearRain(player);SetState(StoryBossState::SpearRainWindup,1.15F);
                spearRainCooldown_=14.0F*difficulty_.cooldown;
            } else if(timer_<=0&&StartPatriotExpansion(player)) {
                // New attacks share the same encounter and recovery rules.
            } else if(phaseTwo_&&timer_<=0&&skillCooldown_<=0&&distance>190) {
                aim_=player;SetState(StoryBossState::SpearWindup,2.0F);
                skillCooldown_=6.5F*difficulty_.cooldown;
            } else if(timer_<=0&&guardCooldown_<=0&&distance>80&&distance<620) {
                SetState(StoryBossState::GuardWindup,.9F);
                guardCooldown_=(phaseTwo_?11.0F:13.0F)*difficulty_.cooldown;
            } else if(timer_<=0&&chargeCooldown_<=0&&distance>260&&distance<760&&player.y>GameConfig::kFloorY-230) {
                aim_=player;
                chargeStart_=feet_.x;chargeTarget_=std::clamp(player.x+facing_*55,left_+45,right_-45);
                SetState(StoryBossState::ChargeWindup,phaseTwo_?.75F:1.0F);
                chargeCooldown_=(phaseTwo_?7.0F:9.0F)*difficulty_.cooldown;
            } else if(distance<(phaseTwo_?225:145)&&timer_<=0&&player.y>GameConfig::kFloorY-195) {
                comboCount_=phaseTwo_?1:4;comboFired_=0;
                SetState(StoryBossState::ComboWindup,phaseTwo_?.75F:.90F);
            } else if(distance>(phaseTwo_?185:112)) {
                const float next=std::clamp(feet_.x+facing_*(phaseTwo_?145:98)*difficulty_.movement*dt,left_+45,right_-45);
                moving_=next!=feet_.x;feet_.x=next;
            }
        }
        break;
    case StoryBossState::PrismWindup:
        if(timer_<=0)SetState(StoryBossState::Prism,1.6F);
        break;
    case StoryBossState::ThornWindup:
        if(timer_<=0)SetState(StoryBossState::Thorn,1.7F);
        break;
    case StoryBossState::QuakeWindup:
        if(timer_<=0)SetState(StoryBossState::Quake,1.8F);
        break;
    case StoryBossState::LanceWindup:
        if(timer_<=0) {
            const Vector2 start{feet_.x+facing_*48.0F,feet_.y-92};
            missiles_.push_back({BossFx::SpearLance,start,start,{facing_*1100.0F*difficulty_.projectile,0},.7F});
            AddVisual(BossFx::PatriotSlash,start,.6F,1.3F);
            SetState(StoryBossState::Lance,.55F);
        }
        break;
    case StoryBossState::Prism:
    case StoryBossState::Thorn:
    case StoryBossState::Quake:
    case StoryBossState::Lance:
        if(timer_<=0)SetState(StoryBossState::Recover,1.1F*difficulty_.recovery);
        break;
    case StoryBossState::FrostStepWindup:
        if(timer_<=0){mobilityTrail_=0;SetState(StoryBossState::FrostStep,.60F);}
        break;
    case StoryBossState::FrostStep: {
        const float t=std::clamp(1-timer_/.60F,0.0F,1.0F);
        feet_.x=mobilityStart_+(mobilityTarget_-mobilityStart_)*t*t*(3-2*t);moving_=true;
        mobilityTrail_-=dt;
        if(mobilityTrail_<=0){AddVisual(BossFx::FrostTrail,{feet_.x,feet_.y-25},.65F);mobilityTrail_=.08F;}
        if(timer_<=0) {
            feet_.x=mobilityTarget_;aim_=player;facing_=player.x>=feet_.x?1:-1;
            SetState(StoryBossState::FrostFanWindup,1.2F);
        }
        break;
    }
    case StoryBossState::LeapWindup:
        if(timer_<=0)SetState(StoryBossState::Leap,.80F);
        break;
    case StoryBossState::Leap: {
        const float t=std::clamp(1-timer_/.80F,0.0F,1.0F);
        feet_={mobilityStart_+(mobilityTarget_-mobilityStart_)*t,GameConfig::kFloorY-240*std::sin(t*3.14159265F)};
        moving_=true;
        if(timer_<=0) {
            feet_={mobilityTarget_,GameConfig::kFloorY};
            zones_.push_back({BossFx::Impact,feet_,145,0,.20F});
            LaunchGroundWave(1.6F);ritualTimer_=1.3F;
            SetState(StoryBossState::Recover,1.15F*difficulty_.recovery);
        }
        break;
    }
    case StoryBossState::FrostCast:
        if(timer_<=0){
            if(frostCastSerial_++%3==2)for(float offset:{-105.0F,0.0F,105.0F})
                LaunchFrost({aim_.x,aim_.y+offset});
            else LaunchFrost(aim_);
            SetState(StoryBossState::Recover,(phaseTwo_?1.25F:1.55F)*difficulty_.recovery);
        }break;
    case StoryBossState::FrostFanWindup:
        if(timer_<=0){LaunchFrostFan();frostNeedsBasic_=true;SetState(StoryBossState::Recover,1.1F*difficulty_.recovery);}
        break;
    case StoryBossState::SpearRainWindup:
        if(timer_<=0)SetState(StoryBossState::SpearRain,1.3F);
        break;
    case StoryBossState::SpearRain:
        if(timer_<=0)SetState(StoryBossState::Recover,1.0F*difficulty_.recovery);
        break;
    case StoryBossState::IceRingWindup:
        if(timer_<=0) {
            const float radius=phaseTwo_?390:250;
            zones_.push_back({BossFx::IceRing,feet_,radius,0,.65F});AddVisual(BossFx::IceRing,feet_,1.0F,radius/250);
            SetState(StoryBossState::IceRing,.65F);
        }break;
    case StoryBossState::IceRing:
        if(timer_<=0)SetState(StoryBossState::Recover,.8F*difficulty_.recovery);break;
    case StoryBossState::BlackIceWindup:
        if(timer_<=0){AddVisual(BossFx::BlackIce,feet_,.7F);SetState(StoryBossState::BlackIce,.7F);}break;
    case StoryBossState::BlackIce:if(timer_<=0)SetState(StoryBossState::Recover,.8F*difficulty_.recovery);break;
    case StoryBossState::IceRainWindup:
        if(timer_<=0){AddIceRain(aim_);SetState(StoryBossState::IceRain,1.9F);}break;
    case StoryBossState::IceRain:
        if(timer_<=0)SetState(StoryBossState::Recover,.7F*difficulty_.recovery);break;
    case StoryBossState::GuardWindup:
        if(timer_<=0){guardIntegrity_=phaseTwo_?14.0F:10.0F;guardPulse_=.4F;guardPressure_=0;
            SetState(StoryBossState::Guard,phaseTwo_?3.1F:3.5F);}break;
    case StoryBossState::Guard:
        guardPulse_-=dt;
        if(guardPulse_<=0) {
            zones_.push_back({BossFx::Ritual,feet_,phaseTwo_?185.0F:155.0F,.38F,.4F});
            AddVisual(BossFx::Ritual,feet_,.8F);guardPulse_=1.3F;
        }
        if(distance>105) {
            const float next=std::clamp(feet_.x+facing_*(phaseTwo_?125.0F:90.0F)*difficulty_.movement*dt,left_+45,right_-45);
            moving_=next!=feet_.x;feet_.x=next;
        }
        if(timer_<=0)SetState(StoryBossState::Recover,.8F*difficulty_.recovery);
        break;
    case StoryBossState::ChargeWindup:
        if(timer_<=0)SetState(StoryBossState::Charge,.85F);break;
    case StoryBossState::Charge: {
        const float speed=(phaseTwo_?690.0F:550.0F)*difficulty_.movement;
        const float delta=std::clamp(chargeTarget_-feet_.x,-speed*dt,speed*dt);
        feet_.x=std::clamp(feet_.x+delta,left_+45,right_-45);moving_=std::abs(delta)>.01F;
        if(timer_<=0||std::abs(feet_.x-chargeTarget_)<2) {
            LaunchGroundWave();
            SetState(StoryBossState::Recover,(phaseTwo_?.75F:1.05F)*difficulty_.recovery);
        }
        break;
    }
    case StoryBossState::ComboWindup:
        if(timer_<=0)SetState(StoryBossState::Combo,phaseTwo_?.5F:1.6F);break;
    case StoryBossState::Combo:
        while(comboFired_<comboCount_ && stateTime_>=comboFired_*.36F) {
            ++comboFired_;
            const Vector2 at{feet_.x+facing_*(phaseTwo_?125.0F:85.0F),feet_.y-55};
            zones_.push_back({BossFx::PatriotSlash,at,phaseTwo_?128.0F:80.0F,0,.23F});
            AddVisual(BossFx::PatriotSlash,at,.35F,phaseTwo_?1.4F:1);
        }
        if(timer_<=0)SetState(StoryBossState::Recover,(phaseTwo_?1.0F:1.25F)*difficulty_.recovery);break;
    case StoryBossState::SpearWindup:
        // Track only early in the windup; the final bright marker is a fixed target.
        if(stateTime_<1.05F)aim_=player;
        if(timer_<=0){LaunchSpear();SetState(StoryBossState::Spear,.5F);}break;
    case StoryBossState::Spear:if(timer_<=0)SetState(StoryBossState::Recover,.8F*difficulty_.recovery);break;
    case StoryBossState::Recover:if(timer_<=0)SetState(StoryBossState::Hunt,.1F);break;
    default:break;
    }
}
bool StoryBoss::WinterAttackHits(Rectangle body,Rectangle projectileBody,Vector2& source,BossImpact* impact) {
    const auto assign=[&](int damage,bool cold,bool shatter){if(impact)*impact={damage,cold,shatter};};
    for(auto& m:missiles_)if(m.life>0) {
        const bool spear=m.kind==BossFx::Spear||m.kind==BossFx::SpearLance;
        const float radius=m.kind==BossFx::ColdWave?48:m.kind==BossFx::GroundWave?29:spear?15:13;
        const float dx=m.position.x-m.previous.x,dy=m.position.y-m.previous.y;
        const int steps=std::clamp(int(std::hypot(dx,dy)/8)+1,1,256);
        bool hit=false;for(int i=0;i<=steps&&!hit;++i) {
            const float t=float(i)/steps;
            hit=CheckCollisionCircleRec({m.previous.x+dx*t,m.previous.y+dy*t},radius,projectileBody);
        }
        if(hit) {
            source=m.position;assign(spear?2:1,
                m.kind==BossFx::ColdWave||m.kind==BossFx::FrostBolt,false);
            AddVisual(spear||m.kind==BossFx::GroundWave?BossFx::Impact:BossFx::FrostBolt,m.position,.6F);m.life=0;return true;
        }
    }
    if(kind_==StoryBossKind::Patriot&&state_==StoryBossState::Charge&&!strikeConsumed_&&
        CheckCollisionRecs(body,{feet_.x-62,feet_.y-136,124,136})) {
        strikeConsumed_=true;source=feet_;assign(phaseTwo_?2:1,false,false);
        AddVisual(BossFx::PatriotSlash,feet_,.4F,1.5F);return true;
    }
    for(auto& z:zones_)if(z.delay<=0&&z.remaining>0&&!z.consumed&&z.kind!=BossFx::BlackIce&&z.kind!=BossFx::SpearRain&&z.kind!=BossFx::FrostPrism) {
        const bool ground=z.kind==BossFx::Ritual||z.kind==BossFx::IceRing||z.kind==BossFx::FrostThorn||z.kind==BossFx::PatriotQuake;
        if(ground&&body.y+body.height<GameConfig::kFloorY-(z.kind==BossFx::IceRing?130:85))continue;
        const Rectangle area{z.position.x-z.radius,z.position.y-(z.kind==BossFx::IceRain?310:ground?145:70),
            z.radius*2,z.kind==BossFx::IceRain?315.0F:ground?150.0F:140.0F};
        if(!CheckCollisionRecs(body,area))continue;
        z.consumed=true;z.pulse=1.5F;source=z.position;
        assign(z.kind==BossFx::PatriotQuake&&phaseTwo_?2:1,
            z.kind==BossFx::IceRing||z.kind==BossFx::IceRain||z.kind==BossFx::FrostThorn,z.kind==BossFx::IceRing);
        return true;
    }
    return false;
}
std::vector<Rectangle> StoryBoss::SealedFloor() const {
    std::vector<Rectangle> tiles;
    if(kind_!=StoryBossKind::FrostNova||!Active())return tiles;
    for(const auto& z:zones_)if(z.kind==BossFx::BlackIce&&z.delay<=0&&z.remaining>0)
        tiles.push_back({z.position.x-z.radius,GameConfig::kFloorY-2,z.radius*2,70});
    return tiles;
}
bool StoryBoss::SealedGroundAt(float x) const {
    for(const auto& t:SealedFloor())if(x>=t.x&&x<t.x+t.width)return true;
    return false;
}
float StoryBoss::SafeGroundX(float x) const {
    if(!SealedGroundAt(x))return std::clamp(x,left_+25,right_-25);
    for(float distance=30;distance<right_-left_;distance+=10)
        for(float candidate:{x-distance,x+distance})
            if(candidate>=left_+25&&candidate<=right_-25&&!SealedGroundAt(candidate))return candidate;
    return std::clamp(x,left_+25,right_-25);
}

bool StoryBoss::StartFrostSkill(Vector2 player) {
    const float distance=std::abs(player.x-feet_.x);
    for(int offset=0;offset<7;++offset) {
        const int skill=(frostSkillCursor_+offset)%7;
        if(skill==0&&ringCooldown_<=0) {
            SetState(StoryBossState::IceRingWindup,1.15F);
            ringCooldown_=(phaseTwo_?5.2F:6.8F)*difficulty_.cooldown;
        } else if(skill==1&&sealCooldown_<=0) {
            aim_=player;AddBlackIce(aim_);SetState(StoryBossState::BlackIceWindup,kSealWarning);
            sealCooldown_=(phaseTwo_?8:11)*difficulty_.cooldown;
        } else if(skill==2&&rainCooldown_<=0&&distance<920) {
            aim_=player;SetState(StoryBossState::IceRainWindup,1.05F);
            rainPattern_=rainSerial_++%3;
            rainCooldown_=(phaseTwo_?8.8F:11.5F)*difficulty_.cooldown;
        } else if(skill==3&&frostFanCooldown_<=0&&distance<850) {
            aim_=player;SetState(StoryBossState::FrostFanWindup,1.2F);
            frostFanCooldown_=(phaseTwo_?9.0F:12.0F)*difficulty_.cooldown;
        } else if(skill==4&&StartFrostStep(player)) {
            // A movement skill participates in the normal rotation as well as reacting to range.
        } else if(skill==5&&expansionCooldown_[0]<=0&&distance<850) {
            StartPrism(player);
        } else if(skill==6&&expansionCooldown_[1]<=0&&distance<850) {
            StartThorns(player);
        } else continue;
        frostSkillCursor_=(skill+1)%7;
        return true;
    }
    return false;
}

bool StoryBoss::StartFrostStep(Vector2 player) {
    if(mobilityCooldown_>0)return false;
    const float direction=player.x>=feet_.x?1.0F:-1.0F;
    const float distance=std::abs(player.x-feet_.x);
    float target=distance>600?player.x-direction*300:feet_.x-direction*420;
    target=std::clamp(target,left_+45,right_-45);
    if(std::abs(target-feet_.x)<180)target=std::clamp(feet_.x+direction*480,left_+45,right_-45);
    target=feet_.x+std::clamp(target-feet_.x,-640.0F,640.0F);
    if(std::abs(target-feet_.x)<100)return false;
    mobilityStart_=feet_.x;mobilityTarget_=target;
    facing_=target>=feet_.x?1:-1;
    mobilityCooldown_=(phaseTwo_?9.0F:11.0F)*difficulty_.cooldown;
    SetState(StoryBossState::FrostStepWindup,.85F);return true;
}

void StoryBoss::StartPatriotLeap(Vector2 player) {
    mobilityStart_=feet_.x;
    mobilityTarget_=std::clamp(feet_.x+std::clamp(player.x-feet_.x,-900.0F,900.0F),left_+55,right_-55);
    aim_={mobilityTarget_,GameConfig::kFloorY};facing_=mobilityTarget_>=feet_.x?1:-1;
    mobilityCooldown_=(phaseTwo_?9.0F:12.0F)*difficulty_.cooldown;
    SetState(StoryBossState::LeapWindup,1.10F);
}
