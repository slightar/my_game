#include "boss.h"

#include "game_types.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

constexpr float kIdleDuration = 0.38F;
constexpr float kEnragedIdleDuration = 0.2F;
constexpr float kMeleeRange = 155.0F;
constexpr float kMeleeWindupDuration = 0.22F;
constexpr float kMeleeDuration = 0.18F;
constexpr float kMeleeReach = 64.0F;
constexpr float kRushWindupDuration = 0.34F;
constexpr float kRushTeleportOutDuration = 0.24F;
constexpr float kRushTeleportInDuration = 0.55F;
constexpr float kRushTeleportBehindDistance = 150.0F;
constexpr float kRushDuration = 0.82F;
constexpr float kRushSpeed = 1040.0F;
constexpr float kAirWindupDuration = 0.44F;
constexpr float kAirSlashDuration = 0.28F;
constexpr float kDartWindupDuration = 0.42F;
constexpr float kDartSpeed = 720.0F;
constexpr float kDartRadius = 10.0F;
constexpr float kRecoverDuration = 0.28F;
constexpr float kEnragedRecoverDuration = 0.16F;
constexpr std::array<float, 3> kDartAngles{-0.24F, 0.0F, 0.24F};
Vector2 Lerp(Vector2 start, Vector2 end, float amount) {
    return {start.x + (end.x - start.x) * amount,
            start.y + (end.y - start.y) * amount};
}

bool BladeHits(Vector2 from,Vector2 to,Rectangle body,float radius) {
    const Rectangle expanded{body.x-radius,body.y-radius,body.width+radius*2,body.height+radius*2};
    float enter=0,leave=1;
    const auto axis=[&](float start,float delta,float low,float high) {
        if(std::abs(delta)<.00001F)return start>=low&&start<=high;
        float a=(low-start)/delta,b=(high-start)/delta;
        if(a>b)std::swap(a,b);
        enter=std::max(enter,a);leave=std::min(leave,b);return enter<=leave;
    };
    return axis(from.x,to.x-from.x,expanded.x,expanded.x+expanded.width)&&
           axis(from.y,to.y-from.y,expanded.y,expanded.y+expanded.height);
}

}  // namespace

void Boss::Reset() {
    Reset(GameConfig::kBossArena,
          {GameConfig::kBossArena.x + GameConfig::kBossArena.width - 210.0F,
           GameConfig::kFloorY - kRadius});
}

void Boss::Reset(Rectangle arena, Vector2 position, float maxHealth) {
    arena_=arena;
    floorY_=arena.y+arena.height-28.0F;
    maxHealth_=maxHealth;
    position_=position;
    slashStart_ = position_;
    slashTarget_ = position_;
    state_ = State::Idle;
    stateTimer_ = kIdleDuration;
    facingDirection_ = -1;
    health_ = maxHealth_;
    nextRangedAttack_ = 0;
    meleeSerial_=0;
    fanAim_=retreatStart_=position_;
    retreatTargetX_=position_.x;
    animationTime_ = 0.0F;
    rushTeleportTargetX_ = position_.x;
    stunTimer_ = 0.0F;
    darts_.clear();
    shadowAim_=shadowLanding_=position_;shadowStrike_=0;shadowCooldown_=8;
    shadowImpactWindow_=0;shadowRecover_=false;shadowHit_.fill(false);
    shadowTraces_.clear();bladeSweeps_.clear();
    effectRevision_=0;effectCue_=EffectCue::Charge;
    echoes_.clear();mirrorCooldown_=12;ambushCooldown_=19;mirrorHalf_=1;
}

void Boss::Update(float deltaTime, Vector2 playerPosition,
                  int playerFacingDirection,bool continueFrame) {
    if(!continueFrame)BeginAttackFrame();
    float remaining=std::clamp(deltaTime,0.0F,.25F);
    while(remaining>.000001F) {
        const float step=std::min(remaining,1.0F/120);
        Tick(step,playerPosition,playerFacingDirection);remaining-=step;
    }
}

void Boss::BeginAttackFrame(){bladeSweeps_.clear();}

void Boss::Tick(float deltaTime,Vector2 playerPosition,int playerFacingDirection) {
    animationTime_ += deltaTime;
    if (state_ == State::Defeated) {
        return;
    }

    UpdateDarts(deltaTime);
    for(auto& echo:echoes_) {
        echo.age+=deltaTime;
        if(echo.delay>0)echo.delay=std::max(0.0F,echo.delay-deltaTime);
        else echo.remaining-=deltaTime;
    }
    std::erase_if(echoes_,[](const auto& echo){return echo.remaining<-.65F;});
    for(auto& trace:shadowTraces_)trace.age+=deltaTime;
    std::erase_if(shadowTraces_,[](const auto& trace){return trace.age>(trace.final?1.6F:.7F);});
    shadowImpactWindow_=std::max(0.0F,shadowImpactWindow_-deltaTime);
    if (stunTimer_ > 0.0F) {
        stunTimer_ = std::max(0.0F, stunTimer_ - deltaTime);
        return;
    }
    stateTimer_ -= deltaTime;
    shadowCooldown_=std::max(0.0F,shadowCooldown_-deltaTime);
    mirrorCooldown_=std::max(0.0F,mirrorCooldown_-deltaTime);
    ambushCooldown_=std::max(0.0F,ambushCooldown_-deltaTime);
    switch (state_) {
        case State::Idle:
            facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
            if (stateTimer_ <= 0.0F) {
                if(shadowCooldown_<=0)StartShadow(playerPosition);
                else if(mirrorCooldown_<=0)StartMirror(playerPosition);
                else if(ambushCooldown_<=0)StartAmbush(playerPosition);
                else if (std::abs(playerPosition.x - position_.x) <= kMeleeRange) {
                    if(meleeSerial_++%3==2)StartFanRetreat(playerPosition);
                    else StartMelee(playerPosition);
                } else {
                    switch (nextRangedAttack_) {
                        case 0:
                            StartRush(playerPosition, playerFacingDirection);
                            break;
                        case 1:
                            StartAirSlash(playerPosition);
                            break;
                        case 2:
                            StartDartVolley(playerPosition);
                            break;
                        default:
                            StartFanRetreat(playerPosition);
                            break;
                    }
                    nextRangedAttack_ = (nextRangedAttack_ + 1) % 4;
                }
            }
            break;
        case State::MeleeWindup:
            facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
            if (stateTimer_ <= 0.0F) {
                state_ = State::Melee;
                stateTimer_ = kMeleeDuration;
            }
            break;
        case State::Melee:
            position_.x += static_cast<float>(facingDirection_) * 170.0F * deltaTime;
            if (stateTimer_ <= 0.0F) {
                state_ = State::Recover;
                animationTime_ = 0.0F;
                stateTimer_ = health_ <= maxHealth_ / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        case State::RushTeleportOut:
            if (stateTimer_ <= 0.0F) {
                position_.x = rushTeleportTargetX_;
                position_.y = floorY_ - kRadius;
                facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
                state_ = State::RushTeleportIn;
                animationTime_ = 0.0F;
                stateTimer_ = kRushTeleportInDuration;
            }
            break;
        case State::RushTeleportIn:
            if (stateTimer_ <= 0.0F) {
                state_ = State::Rush;
                animationTime_ = 0.0F;
                stateTimer_ = kRushDuration;
            }
            break;
        case State::RushWindup:
            if (stateTimer_ <= 0.0F) {
                state_ = State::Rush;
                animationTime_ = 0.0F;
                stateTimer_ = kRushDuration;
            }
            break;
        case State::Rush: {
            position_.x += static_cast<float>(facingDirection_) * kRushSpeed * deltaTime;
            const float left = arena_.x + 28.0F + kRadius;
            const float right = arena_.x +
                                arena_.width - 28.0F - kRadius;
            const float clampedX = std::clamp(position_.x, left, right);
            const bool hitWall = clampedX != position_.x;
            position_.x = clampedX;
            if (stateTimer_ <= 0.0F || hitWall) {
                state_ = State::Recover;
                animationTime_ = 0.0F;
                stateTimer_ = health_ <= maxHealth_ / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        }
        case State::AirSlashWindup:
            if (stateTimer_ <= 0.0F) {
                state_ = State::AirSlash;
                animationTime_ = 0.0F;
                stateTimer_ = kAirSlashDuration;
            }
            break;
        case State::AirSlash: {
            const float progress = std::clamp(1.0F - stateTimer_ / kAirSlashDuration,
                                              0.0F, 1.0F);
            position_ = Lerp(slashStart_, slashTarget_, progress * progress);
            if (stateTimer_ <= 0.0F) {
                position_ = slashTarget_;
                state_ = State::Recover;
                animationTime_ = 0.0F;
                stateTimer_ = health_ <= maxHealth_ / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        }
        case State::DartWindup:
            facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
            if (stateTimer_ <= 0.0F) {
                LaunchDarts();
                state_ = State::Recover;
                animationTime_ = 0.0F;
                stateTimer_ = health_ <= maxHealth_ / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        case State::FanRetreat: {
            const float t=std::clamp(animationTime_/.45F,0.0F,1.0F);
            const float ease=t*t*(3-2*t);
            position_={retreatStart_.x+(retreatTargetX_-retreatStart_.x)*ease,
                       floorY_-kRadius-std::sin(t*3.14159265F)*85};
            if(stateTimer_<=0) {
                position_={retreatTargetX_,floorY_-kRadius};
                LaunchFan();
                state_=State::Recover;animationTime_=0;
                stateTimer_=.48F; // A clear counterattack window after the spread.
            }
            break;
        }
        case State::MirrorWindup:
            if(stateTimer_<=0) {
                LaunchMirror();state_=State::MirrorVolley;stateTimer_=.35F;animationTime_=0;
                ++effectRevision_;effectCue_=EffectCue::Slash;
            }
            break;
        case State::AmbushWindup:
            if(stateTimer_<=0){state_=State::Ambush;stateTimer_=1.0F;animationTime_=0;
                ++effectRevision_;effectCue_=EffectCue::Slash;}
            break;
        case State::MirrorVolley:
        case State::Ambush:
            if(stateTimer_<=0){state_=State::Recover;stateTimer_=1.0F;animationTime_=0;}
            break;
        case State::ShadowWindup:
        case State::ShadowPause:
            if(stateTimer_<=0)StartShadowSlash();
            break;
        case State::ShadowSlash: {
            const Vector2 before=position_;
            const float t=std::clamp(1-stateTimer_/ShadowSlashDuration(shadowStrike_),0.0F,1.0F);
            position_=Lerp(shadowStarts_[shadowStrike_],shadowEnds_[shadowStrike_],t);
            bladeSweeps_.push_back({before,position_,shadowStrike_});
            if(stateTimer_<=0) {
                position_=shadowEnds_[shadowStrike_];
                shadowTraces_.push_back({shadowStarts_[shadowStrike_],position_,0,facingDirection_,false});
                ++shadowStrike_;animationTime_=0;
                if(shadowStrike_<3)PrepareShadowSlash(false);
                else {
                    // Retarget once above the player's current position, then keep the warning fixed.
                    shadowLanding_={std::clamp(playerPosition.x,arena_.x+28+kRadius,
                                              arena_.x+arena_.width-28-kRadius),floorY_-kRadius};
                    position_={shadowLanding_.x,std::max(arena_.y+kRadius,
                                      std::min(shadowLanding_.y-260,playerPosition.y-220))};
                    state_=State::ShadowFinalWindup;stateTimer_=kShadowFinalWindup;
                    slashStart_=position_;slashTarget_=shadowLanding_;
                }
            }
            break;
        }
        case State::ShadowFinalWindup:
            if(stateTimer_<=0){state_=State::ShadowFinal;stateTimer_=.24F;animationTime_=0;}
            break;
        case State::ShadowFinal: {
            const Vector2 before=position_;
            const float t=std::clamp(1-stateTimer_/.24F,0.0F,1.0F);
            position_=Lerp(slashStart_,slashTarget_,t*t);
            bladeSweeps_.push_back({before,position_,3});
            if(stateTimer_<=0) {
                position_=slashTarget_;shadowImpactWindow_=.16F;
                ++effectRevision_;effectCue_=EffectCue::Impact;
                shadowTraces_.push_back({slashStart_,position_,0,facingDirection_,true});
                state_=State::Recover;stateTimer_=1.1F;animationTime_=0;
            }
            break;
        }
        case State::Recover:
            if (stateTimer_ <= 0.0F) {
                state_ = State::Idle;
                shadowRecover_=false;
                animationTime_ = 0.0F;
                stateTimer_ = health_ <= maxHealth_ / 2
                                  ? kEnragedIdleDuration
                                  : kIdleDuration;
            }
            break;
        case State::Defeated:
            break;
    }
}

bool Boss::ShadowActive() const {
    return state_==State::ShadowWindup||state_==State::ShadowSlash||state_==State::ShadowPause||
           state_==State::ShadowFinalWindup||state_==State::ShadowFinal;
}

void Boss::StartShadow(Vector2 player) {
    const float ground=floorY_-kRadius;
    shadowAim_={std::clamp(player.x,arena_.x+280,arena_.x+arena_.width-280),ground};
    shadowLanding_=shadowAim_;
    // All three routes lock before the first slash; moving away beats the combo.
    shadowStarts_={Vector2{shadowAim_.x-240,ground},Vector2{shadowAim_.x+240,ground-110},
                   Vector2{shadowAim_.x-240,ground-110}};
    shadowEnds_={Vector2{shadowAim_.x+240,ground},Vector2{shadowAim_.x-240,ground},
                 Vector2{shadowAim_.x+240,ground}};
    shadowHit_.fill(false);shadowStrike_=0;shadowRecover_=true;
    shadowImpactWindow_=0;shadowTraces_.clear();darts_.clear();
    PrepareShadowSlash(true);
    shadowCooldown_=health_<=maxHealth_*.5F?12.0F:16.0F;
}

void Boss::PrepareShadowSlash(bool first) {
    // Teleport BEFORE the harmless windup, never on the damaging strike's first frame.
    position_=shadowStarts_[shadowStrike_];
    facingDirection_=shadowEnds_[shadowStrike_].x>=position_.x?1:-1;
    state_=first?State::ShadowWindup:State::ShadowPause;
    stateTimer_=first?kShadowStartWindup:kShadowFollowWindup;animationTime_=0;
    ++effectRevision_;effectCue_=EffectCue::Charge;
}

void Boss::StartShadowSlash() {
    state_=State::ShadowSlash;stateTimer_=ShadowSlashDuration(shadowStrike_);animationTime_=0;
    ++effectRevision_;effectCue_=EffectCue::Slash;
}

void Boss::StartMelee(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    position_.y = floorY_ - kRadius;
    state_ = State::MeleeWindup;
    animationTime_ = 0.0F;
    stateTimer_ = kMeleeWindupDuration;
}

void Boss::StartRush(Vector2 playerPosition, int playerFacingDirection) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    position_.y = floorY_ - kRadius;
    if (health_ <= maxHealth_ / 2) {
        const float left = arena_.x + 28.0F + kRadius;
        const float right = arena_.x + arena_.width -
                            28.0F - kRadius;
        rushTeleportTargetX_ = std::clamp(
            playerPosition.x - static_cast<float>(playerFacingDirection) *
                                   kRushTeleportBehindDistance,
            left, right);
        state_ = State::RushTeleportOut;
        stateTimer_ = kRushTeleportOutDuration;
    } else {
        state_ = State::RushWindup;
        stateTimer_ = kRushWindupDuration;
    }
    animationTime_ = 0.0F;
}

void Boss::StartAirSlash(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    const float left = arena_.x + 80.0F;
    const float right = arena_.x +
                        arena_.width - 80.0F;
    slashStart_ = {
        std::clamp(playerPosition.x - static_cast<float>(facingDirection_) * 260.0F,
                   left, right),
        arena_.y + 120.0F};
    slashTarget_ = {std::clamp(playerPosition.x, left, right),
                    floorY_ - kRadius};
    position_ = slashStart_;
    state_ = State::AirSlashWindup;
    animationTime_ = 0.0F;
    stateTimer_ = kAirWindupDuration;
}

void Boss::StartDartVolley(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    position_.y = floorY_ - kRadius;
    state_ = State::DartWindup;
    animationTime_ = 0.0F;
    stateTimer_ = kDartWindupDuration;
}

void Boss::LaunchDarts() {
    const Vector2 origin{
        position_.x + static_cast<float>(facingDirection_) * 34.0F,
        position_.y - 24.0F};
    for (const float angle : kDartAngles) {
        darts_.push_back({
            origin,
            {static_cast<float>(facingDirection_) * std::cos(angle) * kDartSpeed,
             std::sin(angle) * kDartSpeed},
            angle});
    }
}

void Boss::StartFanRetreat(Vector2 playerPosition) {
    facingDirection_=playerPosition.x>=position_.x?1:-1;
    fanAim_=playerPosition;retreatStart_=position_;
    retreatTargetX_=std::clamp(position_.x-facingDirection_*220.0F,
                             arena_.x+28+kRadius,arena_.x+arena_.width-28-kRadius);
    state_=State::FanRetreat;animationTime_=0;stateTimer_=.75F;
}

void Boss::LaunchFan() {
    const Vector2 origin{position_.x+facingDirection_*34.0F,position_.y-24};
    const float aim=std::atan2(fanAim_.y-origin.y,fanAim_.x-origin.x);
    for(float offset:{-.36F,-.18F,0.0F,.18F,.36F}) {
        const float angle=aim+offset;
        darts_.push_back({origin,{std::cos(angle)*620,std::sin(angle)*620},angle});
    }
}

void Boss::UpdateDarts(float deltaTime) {
    for (Dart& dart : darts_) {
        dart.position.x += dart.velocity.x * deltaTime;
        dart.position.y += dart.velocity.y * deltaTime;
        dart.rotation += 12.0F * deltaTime;
    }

    std::erase_if(darts_, [this](const Dart& dart) {
        const Rectangle room = arena_;
        return dart.position.x < room.x ||
               dart.position.x > room.x + room.width ||
               dart.position.y < room.y ||
               dart.position.y > room.y + room.height;
    });
}

void Boss::TakeDamage(float damage) {
    if (state_ == State::Defeated) {
        return;
    }
    health_ = std::max(0.0F, health_ - damage);
    if (health_ <= 0.0F) {
        state_ = State::Defeated;
        animationTime_ = 0.0F;
        darts_.clear();
        shadowTraces_.clear();bladeSweeps_.clear();shadowImpactWindow_=0;shadowRecover_=false;
        echoes_.clear();
    }
}

void Boss::Stun(float duration) {
    if (state_ != State::Defeated) {
        stunTimer_ = std::max(stunTimer_, duration);
    }
}

bool Boss::DestroyProjectileAt(Vector2 position, float radius) {
    const auto hit = std::find_if(darts_.begin(), darts_.end(),
                                  [position, radius](const Dart& dart) {
        return CheckCollisionCircles(position, radius, dart.position,
                                     kDartRadius);
    });
    if (hit == darts_.end()) {
        return false;
    }
    darts_.erase(hit);
    return true;
}

Vector2 Boss::Position() const { return position_; }
float Boss::Radius() const { return kRadius; }
bool Boss::AttackHits(Rectangle playerHitbox, Rectangle projectileHitbox) {
    const auto dartHit = std::find_if(
        darts_.begin(), darts_.end(),
        [projectileHitbox](const Dart& dart) {
            return CheckCollisionCircleRec(dart.position, kDartRadius,
                                           projectileHitbox);
        });
    if (dartHit != darts_.end()) {
        darts_.erase(dartHit);
        return true;
    }

    if (stunTimer_ > 0.0F) {
        return false;
    }

    for(auto& echo:echoes_)if(state_!=State::MirrorWindup&&state_!=State::MirrorVolley&&
        echo.delay<=0&&echo.remaining>0&&!echo.consumed&&
        CheckCollisionRecs(playerHitbox,{echo.position.x-72,echo.position.y-62,144,96})) {
        echo.consumed=true;return true;
    }

    for(const auto& sweep:bladeSweeps_)if(!shadowHit_[sweep.strike]&&BladeHits(sweep.from,sweep.to,playerHitbox,22)) {
        shadowHit_[sweep.strike]=true;return true;
    }
    if(shadowImpactWindow_>0&&!shadowHit_[3]&&CheckCollisionCircleRec(slashTarget_,110,playerHitbox)) {
        shadowHit_[3]=true;return true;
    }

    if (state_ == State::Melee) {
        const Vector2 hitboxCenter{
            position_.x + static_cast<float>(facingDirection_) * kMeleeReach,
            position_.y - 2.0F};
        return CheckCollisionCircleRec(hitboxCenter, 48.0F, playerHitbox);
    }

    return false;
}
bool Boss::CanDealContactDamage() const {
    return stunTimer_ <= 0.0F &&
           !ShadowActive() && !shadowRecover_ &&
           state_ != State::RushTeleportOut &&
           state_ != State::RushTeleportIn &&
           state_ != State::MeleeWindup &&
           state_ != State::RushWindup &&
           state_ != State::AirSlashWindup &&
           state_ != State::DartWindup &&
           state_ != State::FanRetreat &&
           state_ != State::MirrorWindup && state_ != State::MirrorVolley &&
           state_ != State::AmbushWindup && state_ != State::Ambush &&
           state_ != State::Defeated;
}
bool Boss::IsDefeated() const { return state_ == State::Defeated; }
