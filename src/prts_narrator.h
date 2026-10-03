#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Pure narrative policy: no input polling, renderer, audio or save dependencies.
// The game reports completed actions; holding a key is not an action.
class PrtsNarrator {
public:
    enum class Event { Ranged, Melee, Skill, Dodge, Jump, Reload, Switch,
        Damage, Corrosion, Wave, Clear, Investigate, Unplanned, Locked, Defeat };
    enum class Topic { Navigation, Backtrack, Disobey, Idle, Investigate, Hidden,
        Revisit, Locked, Wave, Ranged, Melee, Mobile, Skill, Switch, Reload,
        Hurt, Corrosion, LowHealth, CleanClear, Clear, FastClear, Defeat,
        EmptyFire, Vertical, System, ExploreAside, CombatAside, Count };
    struct Scene {
        std::string id, guidance;
        bool revisit=false, firstHidden=false, system=false, ending=false;
    };
    struct Observation {
        float deltaX=0;
        bool hasForward=false, threat=false, combatArea=false, elevated=false;
        bool lowHealth=false, corroded=false, dead=false;
    };
    void Reset(std::uint32_t randomSeed=0);
    void Enter(Scene scene);
    void Notify(Event event,const std::string& detail={});
    void Update(float deltaTime,const Observation& observation);
    bool Visible() const { return age_ < duration_ && !text_.empty(); }
    const std::string& Text() const { return text_; }
    Topic CurrentTopic() const { return topic_; }
    float Age() const { return age_; }
    float Opacity() const;
    bool Caution() const;
    const char* Channel() const;
    unsigned Revision() const { return revision_; }
    static std::string GlyphText();
private:
    struct Pending { Topic topic; std::string detail; int priority; float expires; };
    bool Queue(Topic topic,int priority,const std::string& detail={});
    void BeginBattle();
    void FinishBattle();
    void RandomAside(const Observation& observation,bool combat);
    std::uint32_t Random();
    float RandomDelay(float minimum,float maximum);
    void Show(Topic topic,const std::string& detail,int priority);
    bool Relevant(Topic topic,const Observation& observation) const;
    Scene scene_;
    std::string text_;
    Topic topic_=Topic::Navigation;
    std::vector<Pending> pending_;
    std::array<float,static_cast<int>(Topic::Count)> last_{};
    std::array<unsigned,static_cast<int>(Topic::Count)> variants_{};
    float time_=0,age_=100,duration_=7,idle_=0,backtrack_=0,forward_=0;
    struct BattleRecord { int ranged=0,melee=0,skills=0,dodges=0,damage=0,switches=0; float seconds=0; } battle_;
    float lastAction_=0,nextExploration_=0,nextCombat_=0;
    int priority_=0,defiance_=0,exploreShots_=0,exploreJumps_=0;
    int explorationAsides_=0,combatAsides_=0;
    bool backtrackLatched_=false,defeatLatched_=false,battleStarted_=false,battleFinished_=false,battleClearRequested_=false;
    bool lastLowHealth_=false;
    std::uint32_t randomState_=0x51AC0DEU;
    unsigned revision_=0;
};
