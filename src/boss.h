#pragma once

#include "raylib.h"

#include <cstddef>
#include <array>
#include <vector>
#include <memory>

class UiFont;
class BossClientFx;

class Boss {
public:
    void Reset();
    void Reset(Rectangle arena, Vector2 position, float maxHealth=60);
    void Update(float deltaTime, Vector2 playerPosition,
                int playerFacingDirection, bool continueFrame=false);
    // Story encounters subdivide an outer frame; retain every blade sweep until collision.
    void BeginAttackFrame();
    void Draw(const UiFont& font, Texture2D sprite,const BossClientFx* effects=nullptr) const;
    void DrawHud(const UiFont& font) const;

    void TakeDamage(float damage);
    void Stun(float duration);
    bool DestroyProjectileAt(Vector2 position, float radius);
    [[nodiscard]] Vector2 Position() const;
    [[nodiscard]] float Radius() const;
    [[nodiscard]] bool AttackHits(Rectangle playerHitbox,
                                  Rectangle projectileHitbox);
    [[nodiscard]] bool CanDealContactDamage() const;
    [[nodiscard]] bool IsDefeated() const;

    enum class State {
        Idle,
        MeleeWindup,
        Melee,
        RushTeleportOut,
        RushTeleportIn,
        RushWindup,
        Rush,
        AirSlashWindup,
        AirSlash,
        DartWindup,
        FanRetreat,
        ShadowWindup,
        ShadowSlash,
        ShadowPause,
        ShadowFinalWindup,
        ShadowFinal,
        MirrorWindup, MirrorVolley, AmbushWindup, Ambush,
        Recover,
        Defeated
    };

    State CurrentState() const {return state_;}
    float Health() const {return health_;}
    int Facing() const {return facingDirection_;}
    float AnimationTime() const {return animationTime_;}
    float StunTime() const {return stunTimer_;}
    float BlinkTarget() const {return rushTeleportTargetX_;}
    std::size_t DartCount() const {return darts_.size();}
    Vector2 ShadowAim() const {return shadowAim_;}
    Vector2 ShadowLanding() const {return shadowLanding_;}
    struct Echo {Vector2 position{};float delay=0,remaining=.20F,age=0;bool consumed=false;};
    const std::vector<Echo>& Echoes() const {return echoes_;}
    Vector2 EchoAim() const {return fanAim_;}
    static constexpr float ShadowSlashDuration(int strike) {return strike<2?.30F:.18F;}
    int ShadowStrike() const {return shadowStrike_;}
    bool ShadowActive() const;
    enum class EffectCue {Charge,Slash,Impact};
    unsigned EffectRevision() const {return effectRevision_;}
    EffectCue LastEffectCue() const {return effectCue_;}

private:
    friend class BossClientFx;
    struct Dart {
        Vector2 position{};
        Vector2 velocity{};
        float rotation = 0.0F;
    };

    void StartMelee(Vector2 playerPosition);
    void StartRush(Vector2 playerPosition, int playerFacingDirection);
    void StartAirSlash(Vector2 playerPosition);
    void StartDartVolley(Vector2 playerPosition);
    void StartFanRetreat(Vector2 playerPosition);
    void Tick(float dt,Vector2 player,int playerFacing);
    void StartShadow(Vector2 player);
    void PrepareShadowSlash(bool first);
    void StartShadowSlash();
    struct ShadowTrace {Vector2 from{},to{};float age=0;int facing=1;bool final=false;};
    struct BladeSweep {Vector2 from{},to{};int strike=0;};
    void LaunchFan();
    void StartMirror(Vector2 player);
    void StartAmbush(Vector2 player);
    void LaunchMirror();
    void LaunchDarts();
    void UpdateDarts(float deltaTime);
    void DrawBattleSprite(Texture2D battleSprite_) const;

    static constexpr float kRadius = 34.0F;
    static constexpr float kMaxHealth = 60.0F;
    static constexpr float kShadowStartWindup = 1.20F;
    static constexpr float kShadowFollowWindup = .75F;
    static constexpr float kShadowFinalWindup = .90F;

    Vector2 position_{};
    Vector2 slashStart_{};
    Vector2 slashTarget_{};
    Vector2 fanAim_{},retreatStart_{};
    float retreatTargetX_=0;
    int meleeSerial_=0;
    State state_ = State::Idle;
    float stateTimer_ = 0.0F;
    int facingDirection_ = -1;
    float health_ = kMaxHealth;
    int nextRangedAttack_ = 0;
    float animationTime_ = 0.0F;
    float rushTeleportTargetX_ = 0.0F;
    float stunTimer_ = 0.0F;
    Rectangle arena_{};
    float floorY_=0, maxHealth_=kMaxHealth;
    std::vector<Dart> darts_;
    Vector2 shadowAim_{},shadowLanding_{};
    std::array<Vector2,3> shadowStarts_{},shadowEnds_{};
    std::array<bool,4> shadowHit_{};
    int shadowStrike_=0;
    float shadowCooldown_=8,shadowImpactWindow_=0;
    bool shadowRecover_=false;
    std::vector<ShadowTrace> shadowTraces_;
    std::vector<BladeSweep> bladeSweeps_;
    unsigned effectRevision_=0;
    EffectCue effectCue_=EffectCue::Charge;
    std::vector<Echo> echoes_;
    float mirrorCooldown_=12,ambushCooldown_=19;
    int mirrorHalf_=1;
};

// Texture ownership stays outside the reusable, headless combat simulation.
class BossRenderer {
public:
    BossRenderer();
    ~BossRenderer();
    BossRenderer(const BossRenderer&)=delete;
    BossRenderer& operator=(const BossRenderer&)=delete;
    void Draw(const Boss& boss,const UiFont& font) const;
    bool EffectsReady() const;
private:
    Texture2D sprite_{};
    std::unique_ptr<BossClientFx> effects_;
};
