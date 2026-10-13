#include "story_boss.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
constexpr float Step=1.0F/120;
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
Rectangle Body(Vector2 p){return {p.x-19,p.y-39,38,78};}
void Tick(StoryBoss& boss,float time,Vector2 player){for(int i=0;i<int(time/Step)+1;++i)boss.Update(Step,player);}
void Reach(StoryBoss& boss,StoryBossState state,Vector2 player) {
    for(int i=0;i<14000&&boss.State()!=state;++i)boss.Update(Step,player);
    if(boss.State()!=state)throw std::runtime_error("New skill starved: kind="+std::to_string(int(boss.Kind()))+
        " expected="+std::to_string(int(state))+" actual="+std::to_string(int(boss.State()))+
        " distance="+std::to_string(std::abs(boss.Position().x-player.x)));
}
Vector2 Follow(const StoryBoss& boss) {
    const float direction=boss.Feet().x<(boss.Left()+boss.Right())*.5F?1.0F:-1.0F;
    return {boss.Feet().x+direction*400,605};
}
void ReachW(StoryBoss& boss,StoryBossState state,bool low=false) {
    for(int i=0;i<14000;++i) {
        if(boss.State()==state&&(!low||boss.LowArcVolley()))return;
        boss.Update(Step,Follow(boss));
    }
    throw std::runtime_error("W never used its new attack pattern");
}
}

int main(){try {
    StoryBoss boss;Vector2 source{};BossImpact impact{};
    boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    Reach(boss,StoryBossState::FanRetreat,{1300,605});
    const auto before=boss.Position();
    Check(!boss.AttackHits(Body(before),Body(before),source),"Retreat warning dealt contact damage");
    Tick(boss,.46F,{1300,605});
    Check(std::abs(boss.Position().x-before.x)>150&&std::abs(boss.Feet().y-GameConfig::kFloorY)<.01F,
          "Knife fan did not retreat and land inside the arena");
    Check(boss.CustomCrownslayer().DartCount()==0,"Knife fan fired before its warning ended");
    Tick(boss,.3F,{1300,605});
    Check(boss.CustomCrownslayer().DartCount()==5,"Knife fan did not fire five aimed knives");
    Check(boss.DestroyProjectile(boss.Position(),100),"New knives could not be intercepted");
    boss.Damage(10000);
    Check(boss.Defeated()&&boss.CustomCrownslayer().DartCount()==0,"Knife fan survived defeat");

    // A half-health blink also leaves a visible, fixed-direction reaction window.
    boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    boss.Damage(31);
    Reach(boss,StoryBossState::Appear,{1000,605});
    const auto blink=boss.Position();
    const int facing=boss.CustomCrownslayer().Facing();
    Tick(boss,.4F,{blink.x-facing*200,605});
    Check(boss.State()==StoryBossState::Appear&&boss.CustomCrownslayer().Facing()==facing,
          "Blink rush attacked too early or changed its warned direction");
    Check(!boss.CustomCrownslayer().CanDealContactDamage()&&
          !boss.AttackHits(Body(blink),Body(blink),source),"Blink preparation dealt damage");
    Tick(boss,.2F,{blink.x-facing*200,605});
    Check(boss.State()==StoryBossState::Rush,"Blink rush never followed its preparation");

    // Signature combo: warning is harmless, routes lock, and each blade hits once.
    for(float frame:{1.0F/120,1.0F/60,1.0F/30,.1F}) {
        boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
        Reach(boss,StoryBossState::ShadowWindup,{1300,605});
        const auto locked=boss.CustomCrownslayer().ShadowAim();
        Check(!boss.AttackHits(Body(boss.Position()),Body(boss.Position()),source),"Shadow charge dealt contact damage");
        boss.Update(frame,{locked.x+600,605});
        Check(boss.CustomCrownslayer().ShadowAim().x==locked.x,"Shadow routes followed player after warning");
        int hits=0,slashes=0;bool finish=false;
        auto previous=boss.State();
        for(int i=0;i<800&&!finish;++i) {
            boss.Update(frame,{locked.x+600,605});
            const auto state=boss.State();
            if(state==StoryBossState::ShadowSlash&&previous!=state)++slashes;
            const bool finisher=state==StoryBossState::ShadowFinal||state==StoryBossState::Recover;
            const Rectangle center=Body({finisher?boss.CustomCrownslayer().ShadowLanding().x:locked.x,605});
            if(boss.AttackHits(center,center,source)) {
                ++hits;
                Check(!boss.AttackHits(center,center,source),"One shadow strike dealt repeated damage");
            }
            Check(!boss.AttackHits(Body({locked.x+350,605}),Body({locked.x+350,605}),source),"Shadow combo hit outside the warned routes and landing");
            finish=state==StoryBossState::Recover&&previous==StoryBossState::ShadowFinal;
            previous=state;
        }
        Check(finish&&slashes==3&&hits==4,"Signature combo lost one of its three slashes or aerial finisher");
        Check(!boss.CustomCrownslayer().CanDealContactDamage(),"Finisher recovery had unavoidable contact damage");
        Tick(boss,.7F,{locked.x+600,605});
        Check(boss.State()==StoryBossState::Recover,"Signature combo did not leave a counterattack window");
        Check(!boss.AttackHits(Body({locked.x,605}),Body({locked.x,605}),source),"Visual afterglow dealt stale damage");
        Tick(boss,.5F,{locked.x+600,605});
        Check(!boss.CustomCrownslayer().ShadowActive(),"Signature immediately restarted without its cooldown");
    }
    {
    // Every teleport must precede a stationary, harmless preparation, including later strikes.
    for(float frame:{1.0F/120,1.0F/60,1.0F/30,.1F}) {
        boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
        Reach(boss,StoryBossState::ShadowWindup,{1300,605});
        for(int strike=0;strike<3;++strike) {
            const auto warning=boss.State();
            const auto start=boss.Position();
            const auto center=boss.CustomCrownslayer().ShadowAim();
            Check(std::abs(start.x-(center.x+(strike==1?240:-240)))<.01F,
                  "Shadow teleport still occurred after its warning");
            float time=0;
            while(boss.State()==warning) {
                Check(!boss.CustomCrownslayer().CanDealContactDamage(),"Shadow preparation dealt contact damage");
                Check(!boss.AttackHits(Body(start),Body(start),source),"Shadow preparation dealt blade damage");
                Check(std::abs(boss.Position().x-start.x)<.01F&&std::abs(boss.Position().y-start.y)<.01F,
                      "Shadow moved before its warning finished");
                boss.Update(frame,{1300,605});time+=frame;
            }
            Check(time>=(strike==0?1.15F:.70F),"Shadow teleport did not leave enough reaction time");
            Check(boss.State()==StoryBossState::ShadowSlash,"Preparation did not lead to its slash");
            while(boss.State()==StoryBossState::ShadowSlash)boss.Update(frame,{1300,605});
        }
    }

    // At normal movement speed, reacting 0.25 s late to the displayed direction must be enough.
    for(float frame:{1.0F/120,1.0F/60,1.0F/30}) {
        boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
        Reach(boss,StoryBossState::ShadowWindup,{1300,605});
        for(int strike=0;strike<4;++strike) {
            const auto warning=boss.State();
            Vector2 player{strike<3?boss.CustomCrownslayer().ShadowAim().x:
                                   boss.CustomCrownslayer().ShadowLanding().x,605};
            const float direction=strike==2?-1.0F:1.0F;
            float time=0;
            while(boss.State()==warning) {
                if(time>=.25F)player.x+=direction*320*frame;
                boss.Update(frame,player);time+=frame;
                Check(!boss.AttackHits(Body(player),Body(player),source),"Warning damaged a reacting player");
            }
            const auto attack=boss.State();
            while(boss.State()==attack) {
                player.x+=direction*320*frame;
                boss.Update(frame,player);
                Check(!boss.AttackHits(Body(player),Body(player),source),"Displayed dodge direction failed at normal walking speed");
            }
            if(strike==3) {
                Tick(boss,.1F,player);
                Check(!boss.AttackHits(Body(player),Body(player),source),"Avoided finisher still hit through its landing burst");
            }
        }
    }

    // The first two dashes leave time to read the client animation; the third stays brisk.
    boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    Reach(boss,StoryBossState::ShadowSlash,{1300,605});
    for(int strike=0;strike<3;++strike) {
        Check(boss.CustomCrownslayer().ShadowStrike()==strike,"Shadow strike order changed");
        Tick(boss,strike<2?.22F:.10F,{1900,605});
        Check(boss.State()==StoryBossState::ShadowSlash,"Shadow dash ended before its readable duration");
        Tick(boss,.1F,{1900,605});
        Check(boss.State()!=StoryBossState::ShadowSlash,"Shadow dash did not finish on time");
        if(strike<2)Reach(boss,StoryBossState::ShadowSlash,{1900,605});
    }

    // The finisher retargets a moving/jumping player once, including outside the original routes.
    for(Vector2 target:{Vector2{1900,605},Vector2{230,400},Vector2{2670,420}}) {
        boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
        Reach(boss,StoryBossState::ShadowSlash,{1300,605});
        Reach(boss,StoryBossState::ShadowFinalWindup,target);
        const auto warned=boss.CustomCrownslayer().ShadowLanding();
        Check(std::abs(warned.x-target.x)<.01F&&std::abs(boss.Position().x-target.x)<.01F,
              "Finisher did not teleport above the player's current position");
        Check(boss.Position().y<target.y-150,"Finisher did not appear above a jumping player");
        Check(!boss.AttackHits(Body(boss.Position()),Body(boss.Position()),source),"Finisher teleport dealt contact damage");
        Reach(boss,StoryBossState::ShadowFinal,{1300,605});
        Check(boss.CustomCrownslayer().ShadowLanding().x==warned.x,"Finisher followed the player after its landing warning");
        while(boss.State()==StoryBossState::ShadowFinal)boss.Update(Step,{1300,605});
        Check(std::abs(boss.Position().x-warned.x)<.01F,"Finisher missed its warned landing");
        Check(!boss.AttackHits(Body({1300,605}),Body({1300,605}),source),"Finisher hit the old route center");
        Check(boss.AttackHits(Body({warned.x,605}),Body({warned.x,605}),source),"Retargeted landing did not deal damage");
    }

    boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    Reach(boss,StoryBossState::ShadowSlash,{1300,605});
    const auto locked=boss.CustomCrownslayer().ShadowAim();
    boss.Update(.1F,{locked.x+600,605});
    const Rectangle crossed{locked.x-170,600,2,10};
    Check(std::abs(boss.Position().x-(crossed.x+1))>60&&boss.AttackHits(crossed,crossed,source),
          "Fast shadow slash tunneled through an earlier portion of the outer frame");
    boss.Damage(1,.4F);
    boss.Update(.05F,{locked.x,605});
    Check(!boss.AttackHits(Body(boss.Position()),Body(boss.Position()),source),"Stunned shadow slash dealt damage");
    boss.Damage(10000);
    Check(!boss.AttackHits(Body({locked.x,605}),Body({locked.x,605}),source),"Defeated shadow combo retained damage");

    // The landing burst has its own short, one-hit window beyond the plunge blade.
    boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    Reach(boss,StoryBossState::ShadowFinal,{1300,605});
    const auto landing=boss.CustomCrownslayer().ShadowLanding();
    while(boss.State()==StoryBossState::ShadowFinal)boss.Update(Step,{1900,605});
    const Rectangle edge{landing.x+85,600,2,10};
    Check(boss.AttackHits(edge,edge,source)&&!boss.AttackHits(edge,edge,source),"Finisher landing burst missing or repeated");
    boss.StartSandbox(StoryBossKind::Crownslayer,2900,1900);
    Reach(boss,StoryBossState::ShadowFinal,{1300,605});
    const auto expired=boss.CustomCrownslayer().ShadowLanding();
    while(boss.State()==StoryBossState::ShadowFinal)boss.Update(Step,{1900,605});
    Tick(boss,.2F,{1900,605});
    Check(!boss.AttackHits({expired.x+85,600,2,10},{expired.x+85,600,2,10},source),"Finisher burst outlived its damage window");
    }

    for(bool phaseTwo:{false,true}) {
        boss.StartSandbox(StoryBossKind::W,2900,1900);
        if(phaseTwo)boss.Damage(56);
        ReachW(boss,StoryBossState::BarrageWindup);
        std::vector<BossBlast> plan;
        for(const auto& blast:boss.Blasts())if(!blast.c4&&blast.timer>1)plan.push_back(blast);
        const auto gap=boss.Aim();
        Check(plan.size()==(phaseTwo?6:4),"Barrage did not scale from four to six marked blasts");
        for(const auto& blast:plan)Check(blast.timer>1&&blast.position.x>=boss.Left()&&blast.position.x<=boss.Right(),
                                       "Barrage warning was instant or outside the arena");
        Check(!boss.AttackHits(Body(gap),Body(gap),source),"Barrage warning dealt damage");
        boss.Update(Step,{gap.x+400,605});
        Check(boss.Aim().x==gap.x&&std::any_of(boss.Blasts().begin(),boss.Blasts().end(),[&](const auto& b){return b.position.x==plan.front().position.x;}),
              "Barrage followed the player after marking the lanes");
        bool explosionHit=false;
        for(int i=0;i<360;++i) {
            boss.Update(Step,{gap.x,605});
            Check(!boss.AttackHits(Body(gap),Body(gap),source),"Marked safe gap did not protect the player");
            for(const auto& blast:boss.Blasts())if(blast.timer<=0&&!blast.consumed) {
                const Rectangle body{blast.position.x-19,GameConfig::kFloorY-78,38,78};
                explosionHit|=boss.AttackHits(body,body,source);
                break;
            }
        }
        Check(explosionHit,"Barrage effects did not cause real damage");
        ReachW(boss,StoryBossState::VolleyWindup,true);
        while(boss.State()==StoryBossState::VolleyWindup)boss.Update(Step,Follow(boss));
        boss.Update(Step,Follow(boss));
        Check(std::any_of(boss.Grenades().begin(),boss.Grenades().end(),[](const auto& g){return g.gravity==260;}),
              "Low volley still used the ordinary high arc");
        boss.Damage(10000);
        Check(boss.Blasts().empty()&&boss.Grenades().empty(),"New W attacks survived defeat");
    }

    boss.StartSandbox(StoryBossKind::FrostNova,2900,1900);
    const Vector2 frostTarget{1500,605};
    Reach(boss,StoryBossState::FrostFanWindup,frostTarget);
    const auto fan=boss.FrostFanVelocities();
    Check(fan.size()==5&&fan.front().y!=fan.back().y,"First-life frost fan had no angular spread");
    const auto locked=boss.Aim();boss.Update(Step,{1100,400});
    Check(boss.Aim().x==locked.x&&boss.Aim().y==locked.y,"Frost fan warning tracked the player until firing");
    while(boss.State()==StoryBossState::FrostFanWindup)boss.Update(Step,frostTarget);
    Check(std::count_if(boss.Missiles().begin(),boss.Missiles().end(),[](const auto& m){return m.kind==BossFx::FrostBolt&&m.age<.01F;})==5,
          "First-life frost fan did not launch five blades");
    const auto blade=*std::find_if(boss.Missiles().begin(),boss.Missiles().end(),[](const auto& m){return m.kind==BossFx::FrostBolt&&m.age<.01F;});
    Check(boss.AttackHits(Body(blade.position),Body(blade.position),source,&impact)&&impact.cold,
          "New frost blades lost cold damage");
    Reach(boss,StoryBossState::FrostCast,frostTarget);
    boss.Damage(10000);Tick(boss,7.2F,frostTarget);
    Reach(boss,StoryBossState::FrostFanWindup,frostTarget);
    Check(boss.FrostFanVelocities().size()==7,"Second-life frost fan did not strengthen");

    boss.StartSandbox(StoryBossKind::FrostNova,2900,1900);
    float firstOrder[3]{};
    for(int cast=0;cast<3;++cast) {
        Reach(boss,StoryBossState::IceRainWindup,frostTarget);
        while(boss.State()==StoryBossState::IceRainWindup)boss.Update(Step,frostTarget);
        int index=0;
        for(const auto& z:boss.Zones())if(z.kind==BossFx::IceRain) {
            if(cast==0)firstOrder[index]=z.delay;
            if(cast==1)Check((index==0&&z.delay>firstOrder[0])||(index==1)||(index==2&&z.delay<firstOrder[2]),
                            "Second ice rain did not reverse the sweep");
            if(cast==2&&index==1)Check(z.delay>1.6F,"Outside-in ice rain did not leave the middle lane until last");
            ++index;
        }
        Check(index==3,"Rain variation changed the telegraphed lane count");
        Tick(boss,3.0F,frostTarget);
    }

    boss.StartSandbox(StoryBossKind::Patriot,2900,1900);
    boss.Damage(10000);Tick(boss,8.6F,{1400,605});
    for(int i=0;i<14000&&boss.State()!=StoryBossState::SpearRainWindup;++i)boss.Update(Step,Follow(boss));
    Check(boss.State()==StoryBossState::SpearRainWindup,"Patriot never used falling spears against a distant player");
    Check(std::count_if(boss.Zones().begin(),boss.Zones().end(),[](const auto& z){return z.kind==BossFx::SpearRain&&z.delay>1.1F;})==3,
          "Falling spear formation did not mark three delayed lanes");
    const float lane=boss.Aim().x+(boss.Aim().x>boss.Feet().x?180.0F:-180.0F);
    const Rectangle target{lane-12,GameConfig::kFloorY-78,24,78};
    Check(!boss.AttackHits(target,target,source),"Falling spear warning hurt the player");
    const float lockedX=boss.Aim().x;
    Tick(boss,.5F,{1100,605});Check(boss.Aim().x==lockedX,"Falling spear lanes followed player movement");
    bool fallingHit=false;
    for(int i=0;i<240&&!fallingHit;++i) {
        boss.Update(Step,{1100,605});
        fallingHit=boss.AttackHits(target,target,source,&impact)&&impact.damage==2;
    }
    Check(fallingHit,"Falling spears had visuals but no actual projectile damage");
    boss.Damage(10000);
    Check(boss.Defeated()&&boss.Zones().empty()&&boss.Missiles().empty(),"Falling spear formation survived final defeat");
    std::cout<<"Four Bosses: retreat knives, safe-gap barrage, low volley, frost fan/rain order, falling spears and cleanup passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
