#include "prts_narrator.h"
#include <iostream>
#include <stdexcept>
#include <tuple>
using N=PrtsNarrator;
using T=N::Topic;
using E=N::Event;
void Check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void Advance(N& n,float seconds,N::Observation o={}){for(int i=0;i<int(seconds/.05F);++i)n.Update(.05F,o);}
N Begin(unsigned seed=17){N n;n.Reset(seed);n.Enter({"test","请沿主路继续前进。"});return n;}
void Start(N& n){Advance(n,3);n.Notify(E::Wave);n.Update(.05F,{0,false,true,true});}
int main(){try {
    N::Observation fighting{};fighting.threat=true;fighting.combatArea=true;fighting.deltaX=1;
    {
        auto n=Begin();const auto initial=n.Revision();Advance(n,8,{1,true});
        Check(!n.Visible()&&n.Revision()==initial,"Navigation persists or adds irrelevant comments");
        Advance(n,2,{-1,true});Advance(n,5,{-1,true});
        Check(n.CurrentTopic()==T::Backtrack,"Exploration lacks deliberate backtracking reaction");
        auto revision=n.Revision();Advance(n,12,{-1,true});
        Check(n.Revision()==revision,"Continuous backtracking repeats its warning");
    }
    for(auto [event,topic,count]:{std::tuple{E::Ranged,T::Ranged,12},std::tuple{E::Melee,T::Melee,4},
          std::tuple{E::Skill,T::Skill,3},std::tuple{E::Dodge,T::Mobile,4},std::tuple{E::Switch,T::Switch,2}}) {
        auto n=Begin();Start(n);Check(n.CurrentTopic()==T::Wave,"Combat opening missing");
        const auto opening=n.Revision();Advance(n,8,fighting);
        for(int i=0;i<count;++i)n.Notify(event);
        Advance(n,12,fighting);
        Check(n.Revision()==opening,"Style triggers automatic mid-fight report");
        n.Notify(E::Wave);n.Update(.05F,fighting);
        Check(n.Revision()==opening,"Second reinforcement wave repeated briefing");
        n.Notify(E::Clear);n.Update(.05F,{});
        Check(n.CurrentTopic()==topic,"Whole-encounter style missing from result");
        Check(n.Text().find("清理完成")!=std::string::npos,"Style result lacks battle outcome");
        auto result=n.Revision();n.Notify(E::Clear);Advance(n,12,{1,true});
        Check(n.Revision()==result,"Clear repeats a report or schedules more style messages");
    }
    {
        auto n=Begin();Start(n);Advance(n,8,fighting);auto initial=n.Revision();
        auto danger=fighting;danger.lowHealth=true;danger.corroded=true;
        n.Notify(E::Damage);n.Notify(E::Corrosion);n.Notify(E::Reload);n.Notify(E::Switch);
        Advance(n,12,danger);
        Check(n.Revision()==initial,"Damage/low health/reload/switch interrupted combat");
        n.Notify(E::Clear);n.Update(.05F,{});
        Check(n.CurrentTopic()==T::Clear,"Damage record not reflected at battle end");
    }
    {
        auto n=Begin();Start(n);
        for(int i=0;i<12;++i)n.Notify(E::Ranged);
        Advance(n,25,fighting);n.Notify(E::Wave);Advance(n,2,fighting);
        n.Notify(E::Clear);n.Update(.05F,{});
        Check(n.CurrentTopic()==T::Ranged,"Early actions were forgotten after ten seconds/later wave");
    }
    {
        auto n=Begin();Start(n);Advance(n,8,fighting);n.Notify(E::Ranged);
        n.Notify(E::Clear);n.Update(.05F,{});
        Check(n.CurrentTopic()==T::CleanClear,"Insufficient style evidence incorrectly classified combat");
        n.Enter({"next","继续前进。"});Start(n);Advance(n,8,fighting);
        n.Notify(E::Clear);n.Update(.05F,{});
        Check(n.CurrentTopic()==T::CleanClear,"Prior encounter stats leaked into new scene");
    }
    {
        auto n=Begin();Start(n);Advance(n,5,fighting);auto dead=fighting;dead.dead=true;
        n.Update(.05F,dead);Check(n.CurrentTopic()==T::Defeat,"Failure boundary lacks immediate result");
        auto revision=n.Revision();Advance(n,60,dead);Check(n.Revision()==revision,"Failure/ambient report repeats after defeat");
    }
    {
        auto n=Begin();Advance(n,3);n.Notify(E::Investigate,"收费亭的灯一直没有熄灭。");n.Update(.05F,{});
        Check(n.Text()=="收费亭的灯一直没有熄灭。","Exploration clue text lost");
        auto revision=n.Revision();n.Notify(E::Investigate,"重复读取");Advance(n,1);
        Check(n.Revision()==revision,"Repeated investigation restarts bubble");
    }
    {
        auto n=Begin();Advance(n,14);n.Notify(E::Unplanned);n.Update(.05F,{});const auto first=n.Text();
        n.Enter({"hidden","",false,true});Check(n.CurrentTopic()==T::Hidden,"Hidden discovery missing");
        n.Enter({"next","继续前进。"});Advance(n,30,{1,true});n.Notify(E::Unplanned);n.Update(.05F,{1,true});
        Advance(n,14,{1,true});Check(n.Text()!=first,"Repeated deviations never change attitude");
        n.Enter({"hidden","",true,false});Check(n.CurrentTopic()==T::Revisit,"Revisit repeats discovery");
        n.Enter({"system","",false,true,true});Check(n.CurrentTopic()==T::System,"System entry lacks distinct reaction");
    }
    {
        auto n=Begin();auto previous=n.Revision();int asides=0;
        for(int i=0;i<6000;++i){n.Update(.05F,{1,true});if(n.Revision()!=previous){++asides;previous=n.Revision();}}
        Check(asides==2,"Exploration random aside budget is not two per scene");
        auto combat=Begin();Start(combat);previous=combat.Revision();asides=0;
        for(int i=0;i<6000;++i){combat.Notify(E::Ranged);combat.Update(.05F,fighting);
            if(combat.Revision()!=previous){++asides;previous=combat.Revision();Check(combat.CurrentTopic()==T::CombatAside,"Random battle line turned into style analysis");}}
        Check(asides==1,"Combat random aside budget is not one per scene");
    }
    {
        auto timing=[](unsigned seed){auto n=Begin(seed);auto initial=n.Revision();
            for(int i=0;i<1200;++i){n.Update(.05F,{1,true});if(n.Revision()!=initial)return i;}return -1;};
        Check(timing(17)==timing(17),"Seeded random timing cannot be reproduced");
        Check(timing(17)!=timing(12345),"Different seeds have identical random timing");
        auto ending=Begin();ending.Enter({"ending","任务路线已完成。",false,false,false,true});
        auto revision=ending.Revision();Advance(ending,300);Check(ending.Revision()==revision,"Ending received unsolicited ambient chatter");
    }
    std::cout<<"PRTS combat boundaries, whole-encounter summaries, quiet combat, seeded random timing/caps and exploration reactions passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}