#pragma once
#include "game_types.h"
#include "boss.h"
#include "adaptive_difficulty.h"
#include <set>
#include <string>
#include <string_view>
#include <vector>

enum class StoryBossKind { None, Crownslayer, W, FrostNova, Patriot };
inline constexpr int kStoryBossCount=4;
const char* StoryBossName(StoryBossKind kind);
const char* StoryBossId(StoryBossKind kind);
const char* StoryBossRegion(StoryBossKind kind);
enum class StoryBossState { Dormant, Arrival, Hunt, MeleeWindup, Strike,
    Disappear, Appear, GrenadeWindup, VolleyWindup, Volley, MineWindup,
    EvadeWindup, Evade, C4, Recover, Defeated, Revive,
    FrostCast, IceRingWindup, IceRing, BlackIceWindup, BlackIce,
    ComboWindup, Combo, SpearWindup, Spear, ChargeWindup, Charge,
    IceRainWindup, IceRain, GuardWindup, Guard,
    RushWindup, Rush, AirSlashWindup, AirSlash, DartWindup, FanRetreat,
    ShadowWindup, ShadowSlash, ShadowPause, ShadowFinalWindup, ShadowFinal,
    BarrageWindup, Barrage, FrostFanWindup, SpearRainWindup, SpearRain,
    FrostStepWindup, FrostStep, LeapWindup, Leap,
    MirrorWindup, MirrorVolley, AmbushWindup, Ambush,
    AirburstWindup, Airburst, RemoteMineWindup, RemoteMine,
    PrismWindup, Prism, ThornWindup, Thorn,
    QuakeWindup, Quake, LanceWindup, Lance };
enum class BossFx { FrostBolt, IceRing, BlackIce, ColdWave, Revival, PatriotSlash, Spear, Ritual, Impact,
    IceRain, GroundWave, WExplosion, WC4Explosion, CrownSlash, SpearRain, FrostTrail, WMuzzle,
    FrostPrism, FrostThorn, PatriotQuake, SpearLance };
struct BossMissile {
    BossFx kind=BossFx::FrostBolt;
    Vector2 position{},previous{},velocity{};
    float life=3,age=0;
};
struct BossZone {
    BossFx kind=BossFx::IceRing;
    Vector2 position{};
    float radius=100,delay=0,remaining=.6F,age=0,pulse=0;
    bool consumed=false;
    float durability=0;
    Vector2 target{};
    bool visualStarted=false;
    float maxDurability=0;
};
struct BossVisual {
    BossFx kind=BossFx::Impact;
    Vector2 position{};
    float age=0,duration=1,scale=1;
    int facing=1;
};
struct BossImpact {int damage=1;bool cold=false,shatter=false;};
struct BossGrenade {
    Vector2 position{},previous{},velocity{},landing{};
    float gravity=680,lifetime=3,age=0;
};
struct BossBlast {
    Vector2 position{};
    float radius=85,timer=0,tracking=0,offset=0,remaining=.35F;
    bool c4=false,consumed=false;
    bool visualStarted=false;
    bool airburst=false;
};
struct BossMine {
    Vector2 position{};
    float age=0, fuse=-1;
    bool remote=false;
    float durability=2;
};

// Main-story encounter simulation and excursion progress. No textures, audio or input.
class StoryBoss {
public:
    void Reset();
    void Enter(std::string_view region,float width,CombatDifficulty difficulty={});
    void StartSandbox(StoryBossKind kind,float width,float x,CombatDifficulty difficulty={});
    const CombatDifficulty& Difficulty() const {return difficulty_;}
    bool TryStart(float playerX);
    void Update(float dt,Vector2 player,int playerFacing=1);
    const Boss& CustomCrownslayer() const {return crown_;}
    bool ResolveBullet(Bullet& bullet,Vector2 previous);
    bool BreakSeal(Bullet& bullet,Vector2 previous);
    bool DestroyProjectile(Vector2 position,float radius);
    bool Damage(float amount,float stun=0,DamageType type=DamageType::Physical);
    bool AttackHits(Rectangle body,Rectangle projectileBody,Vector2& source,BossImpact* impact=nullptr);
    bool Present() const {return kind_!=StoryBossKind::None;}
    bool Active() const {return Present()&&state_!=StoryBossState::Dormant&&state_!=StoryBossState::Defeated;}
    bool Defeated() const {return Present()&&state_==StoryBossState::Defeated;}
    bool Targetable() const;
    Rectangle Hitbox() const;
    Vector2 Position() const {if(kind_==StoryBossKind::Crownslayer)return crown_.Position();return {feet_.x,feet_.y-(kind_==StoryBossKind::Patriot?85.0F:54.0F)};}
    Vector2 Feet() const {return feet_;}
    float Health() const {return health_;}
    float MaxHealth() const {return (kind_==StoryBossKind::Patriot?180.0F:kind_==StoryBossKind::FrostNova?140.0F:kind_==StoryBossKind::W?110.0F:60.0F)*difficulty_.health;}
    bool TwoLives() const {return kind_==StoryBossKind::FrostNova||kind_==StoryBossKind::Patriot;}
    float Invulnerability() const {return invulnerability_;}
    float RevivalDuration() const {return kind_==StoryBossKind::FrostNova?4.0F:6.0F;}
    const std::vector<BossMissile>& Missiles() const {return missiles_;}
    const std::vector<BossZone>& Zones() const {return zones_;}
    std::vector<Rectangle> SealedFloor() const;
    bool SealedGroundAt(float x) const;
    float SafeGroundX(float x) const;
    const std::vector<BossVisual>& Visuals() const {return visuals_;}
    int ComboCount() const {return comboCount_;}
    unsigned ActionRevision() const {return actionRevision_;}
    float Elapsed() const {return elapsed_;}
    float FrostWeather() const {return frostWeather_;}
    float WeatherTime() const {return weatherTime_;}
    const char* Briefing() const;
    float TriggerX() const {return trigger_;}
    float Left() const {return left_;}
    float Right() const {return right_;}
    int Facing() const {return facing_;}
    bool Moving() const {return moving_;}
    bool PhaseTwo() const {return phaseTwo_;}
    float AnimationTime() const {return animation_;}
    float StateTime() const {return stateTime_;}
    float StateDuration() const {return stateDuration_;}
    float Flash() const {return flash_;}
    float StunTime() const {return stun_;}
    float BlinkTarget() const {return blinkX_;}
    Vector2 Aim() const {return aim_;}
    float MobilityTarget() const {return mobilityTarget_;}
    float MobilityStart() const {return mobilityStart_;}
    float EvadeTarget() const {return evadeTarget_;}
    float GuardIntegrity() const {return guardIntegrity_;}
    int VolleyCount() const {return volleyCount_;}
    int RemoteMineCount() const {return remoteMineCount_;}
    bool LowArcVolley() const {return lowArcVolley_;}
    std::vector<Vector2> FrostFanVelocities() const;
    StoryBossKind Kind() const {return kind_;}
    StoryBossState State() const {return state_;}
    StoryBossState RecoveryFrom() const {return recoveryFrom_;}
    const std::vector<BossGrenade>& Grenades() const {return grenades_;}
    const std::vector<BossBlast>& Blasts() const {return blasts_;}
    const std::vector<BossMine>& Mines() const {return mines_;}
    std::vector<float> GrenadeTargets() const;
    std::vector<float> MineTargets() const;
    static StoryBossKind ForRegion(std::string_view region);
    static std::string Glyphs();
private:
    void Tick(float dt,Vector2 player);
    void ResetCrown();
    void SyncCrown();
    Boss crown_;
    int playerFacing_=1;
    void SetState(StoryBossState state,float duration);
    void StartC4(Vector2 player);
    void StartBarrage(Vector2 player);
    void LaunchGrenade(Vector2 target);
    void MoveW(float dt,Vector2 player);
    bool WantsEvade(Vector2 player) const;
    void StartEvade(Vector2 player);
    void AddMine(float x);
    void TickWinter(float dt,Vector2 player);
    void TickWinterHazards(float dt,Vector2 player);
    bool WinterAttackHits(Rectangle body,Rectangle projectileBody,Vector2& source,BossImpact* impact);
    void BeginRevival();
    void FinishRevival();
    void AddBlackIce(Vector2 player);
    void AddIceRain(Vector2 player);
    void LaunchFrost(Vector2 player);
    void LaunchFrostFan();
    bool StartFrostSkill(Vector2 player);
    bool StartFrostStep(Vector2 player);
    void StartPatriotLeap(Vector2 player);
    void AddSpearRain(Vector2 player);
    void LaunchSpear();
    void LaunchGroundWave(float impactScale=1);
    void AddVisual(BossFx kind,Vector2 position,float duration,float scale=1);
    void StartAirburst(Vector2 player);
    void StartRemoteMines(Vector2 player);
    void DeployRemoteMines();
    void StartPrism(Vector2 player);
    void StartThorns(Vector2 player);
    bool StartPatriotExpansion(Vector2 player);
    void TickExpansionHazards();
    bool HitExpansionObject(Bullet& bullet,Vector2 previous);
    StoryBossKind kind_=StoryBossKind::None;
    StoryBossState state_=StoryBossState::Dormant;
    StoryBossState recoveryFrom_=StoryBossState::Hunt;
    std::string region_;
    std::set<std::string> cleared_;
    Vector2 feet_{},aim_{};
    float health_=0,trigger_=0,left_=0,right_=0;
    float timer_=0,stateTime_=0,stateDuration_=0,animation_=0,skillCooldown_=0,blinkX_=0;
    float stun_=0,flash_=0;
    float evadeCooldown_=0,mineCooldown_=0,firePressure_=0,evadeStart_=0,evadeTarget_=0,fallVelocity_=0;
    int attackSerial_=0,volleyFired_=0,strafeDirection_=1,volleyCount_=3,mineCount_=2;
    bool moving_=false,phaseTwo_=false,strikeConsumed_=false;
    bool lowArcVolley_=false;
    bool frostNeedsBasic_=false;
    int barrageSerial_=0,rainSerial_=0,rainPattern_=0,frostSkillCursor_=0;
    float frostFanCooldown_=6,spearRainCooldown_=15;
    float mobilityCooldown_=4,mobilityStart_=0,mobilityTarget_=0,mobilityTrail_=0;
    float frostWeather_=0,weatherTime_=0;
    int facing_=-1;
    std::vector<BossGrenade> grenades_;
    std::vector<BossBlast> blasts_;
    std::vector<BossMine> mines_;
    CombatDifficulty difficulty_{};
    float invulnerability_=0,ringCooldown_=0,sealCooldown_=0,waveCooldown_=0,ritualTimer_=0,elapsed_=0;
    float chargeCooldown_=0,chargeStart_=0,chargeTarget_=0;
    float rainCooldown_=0,guardCooldown_=0,guardIntegrity_=0,guardPulse_=0,guardPressure_=0;
    float stillTime_=0,lastPlayerX_=0;
    int comboFired_=0,comboCount_=4,frostCastSerial_=0;
    unsigned actionRevision_=0;
    std::vector<BossMissile> missiles_;
    std::vector<BossZone> zones_;
    std::vector<BossVisual> visuals_;
    std::array<float,2> expansionCooldown_{10,18};
    int remoteMineCount_=3;
};
