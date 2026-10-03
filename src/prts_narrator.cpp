#include "prts_narrator.h"
#include <algorithm>
#include <cmath>
#include <string_view>
#include <random>

namespace {
using Topic=PrtsNarrator::Topic;
struct Lines { Topic topic; std::array<const char*,3> text; };
constexpr std::array kLines{
    Lines{Topic::Backtrack,{"你正在背离导航方向。请确认这次折返是必要的。","路线没有变更。你似乎另有打算。","我已记录你的折返。继续前进会更有效率。"}},
    Lines{Topic::Disobey,{"你正在检查未经登记的设施。建议返回既定路线。","你再次忽略了导航建议。我会保留这次偏离记录。","你并不需要我的许可，对吗？偏离行为已记录。"}},
    Lines{Topic::Idle,{"行动暂时停止。需要重新确认方向吗？","你停留得比预计更久。导航仍然有效。","我会等待。请在准备好后继续行动。"}},
    Lines{Topic::Investigate,{"记录已读取。它不属于本次行动简报。","你选择了核查原始记录。数据已保留。","这条记录不会改变我的建议。你的判断，另行保存。"}},
    Lines{Topic::Hidden,{"当前位置不在规划路线内。重新评估通行条件。","你进入了未登记区域。我无法确认这次选择的必要性。","又一次偏离。你寻找的东西，似乎不在我的路线中。"}},
    Lines{Topic::Revisit,{"已返回先前区域。行动日志保留，继续由你选择路线。","你又回到了这里。看来还有未完成的判断。","重复访问已记录。导航无需重新初始化。"}},
    Lines{Topic::Locked,{"通行条件尚未满足。重复尝试不会改变结果。","这条通路目前不可用。请评估其他行动。","条件没有变化。你仍然坚持尝试。"}},
    Lines{Topic::Wave,{"检测到敌方增援。请根据距离选择交战方式。","新的巡逻队进入区域。注意攻击前的动作。","敌方活动恢复。上一轮的打法仍需重新评估。"}},
    Lines{Topic::Ranged,{"你主要采用远程压制，下一段也请为换弹留出空间。","你以远程攻击为主，距离控制仍值得保留。","远程攻击是这场战斗的主要选择。行动记录已保存。"}},
    Lines{Topic::Melee,{"这场战斗以近身攻击为主，请为下一段保留撤离余地。","你更多选择近距离交战。敌方的起手动作仍值得留意。","近战是你的主要选择，我会保留这次风险评估。"}},
    Lines{Topic::Mobile,{"你多次利用闪避调整战局，这次机动记录已保留。","这场战斗中你频繁闪避。下一段请留意恢复时间。","你更依赖机动寻找机会，位移记录已保存。"}},
    Lines{Topic::Skill,{"这场战斗多次使用了技能，下一段请考虑冷却空档。","你多次启动技能改变战局，请保留常规应对。","技能使用较为频繁，这场战斗的启动记录已保存。"}},
    Lines{Topic::Switch,{"你在这场战斗中多次交接位置，协同行动已记录。","你多次调整出战干员，这次交战的衔接记录已保留。","干员交接是这场战斗的特点，下一段请重新确认距离。"}},
    Lines{Topic::Reload,{"正在换弹。保持移动，避免停在敌人的攻击范围内。","弹药补充中。现在需要为火力空档留出距离。","换弹窗口已开始。当前威胁不会因此停止。"}},
    Lines{Topic::Hurt,{"此前已记录到有效伤害。建议留意敌人的攻击预兆。","本次交战已有受伤记录，下一次接触请更加谨慎。","你在这场战斗中承受过伤害。请重新评估交战距离。"}},
    Lines{Topic::Corrosion,{"检测到腐蚀。受击后的保护时间缩短，请拉开距离。","腐蚀仍可能造成连续受击风险。注意酸液攻击。","防护状态受到腐蚀影响。请避免连续接触。"}},
    Lines{Topic::LowHealth,{"当前干员仅剩一次受击余量。建议撤离或交接。","生命状态进入危险范围。进攻需要更加谨慎。","下一次有效伤害可能使当前干员失去行动能力。"}},
    Lines{Topic::CleanClear,{"区域清理完成。本次交战未受到有效伤害。","威胁解除。你完整保留了本区域的生命状态。","清理结束，未记录到受伤。当前打法可以保留。"}},
    Lines{Topic::Clear,{"区域清理完成。请在继续前进前评估生命状态。","巡逻队已清除。此前的受击记录将用于风险评估。","当前威胁解除。你可以重新选择前进的节奏。"}},
    Lines{Topic::FastClear,{"威胁已快速清除。推进效率高于本次观察基线。","这次交战结束得很快。请为下一段保留技能余量。","快速清理已记录。现在可以恢复导航路线。"}},
    Lines{Topic::Defeat,{"所有干员失去行动能力。建议重试时调整交战策略。","行动中断。之前的站位与技能时机会纳入复盘。","战术执行失败。重复路线不必重复同一种打法。"}},
    Lines{Topic::EmptyFire,{"当前没有交战目标。建议停止无效火力消耗。","你正在向空处射击。弹药记录与目标记录不一致。","这段火力没有对应的敌方活动。你是在试探吗？"}},
    Lines{Topic::Vertical,{"你正在主动检查高处。导航仍优先考虑主路。","垂直移动增加。请先确认落点，再决定下一次跳跃。","你似乎更关心主路之外的高度。观察已记录。"}},
    Lines{Topic::System,{"导航数据存在缺口。我暂时无法验证当前位置。","位置校验未通过。继续维持……观察。","你的行动仍在记录中。路线确认暂不可用。"}},
    Lines{Topic::ExploreAside,{"导航仍然有效。但你似乎更愿意亲自确认。","这里保留了太多旧记录。并非每一条都与任务有关。","我会继续记录。至于停在哪里，由你决定。"}},
    Lines{Topic::CombatAside,{"我正在记录这场交战。请按你的判断继续。","战斗仍在进行。我会等待结果。","行动数据正在保存。保持你自己的节奏。"}}
};
float Cooldown(Topic topic) {
    if(topic==Topic::Wave||topic==Topic::Defeat)return 1;
    if(topic==Topic::LowHealth)return 10;
    if(topic==Topic::Investigate||topic==Topic::Clear||topic==Topic::CleanClear||topic==Topic::FastClear)return 4;
    return 24;
}
}

void PrtsNarrator::Reset(std::uint32_t seed) {
    *this=PrtsNarrator{};last_.fill(-1000);
    randomState_=seed?seed:std::random_device{}();
    if(!randomState_)randomState_=0x51AC0DEU;
}
std::uint32_t PrtsNarrator::Random() {
    randomState_^=randomState_<<13;randomState_^=randomState_>>17;randomState_^=randomState_<<5;
    return randomState_;
}
float PrtsNarrator::RandomDelay(float minimum,float maximum) {
    return minimum+(maximum-minimum)*static_cast<float>(Random()%10001)/10000;
}

void PrtsNarrator::Enter(Scene scene) {
    if(revision_==0)last_.fill(-1000);
    scene_=std::move(scene);pending_.clear();battle_={};
    idle_=backtrack_=forward_=0;exploreShots_=exploreJumps_=0;
    explorationAsides_=combatAsides_=0;
    backtrackLatched_=defeatLatched_=battleStarted_=battleFinished_=battleClearRequested_=false;
    lastLowHealth_=false;lastAction_=time_;
    nextExploration_=time_+RandomDelay(30,50);
    if(scene_.firstHidden)++defiance_;
    if(scene_.system)Show(Topic::System,{},50);
    else if(scene_.firstHidden)Show(Topic::Hidden,{},55);
    else if(scene_.revisit)Show(Topic::Revisit,{},30);
    else Show(Topic::Navigation,scene_.guidance,20);
}

bool PrtsNarrator::Queue(Topic topic,int priority,const std::string& detail) {
    if(time_-last_[static_cast<int>(topic)]<Cooldown(topic))return false;
    for(const auto& item:pending_)if(item.topic==topic)return false;
    if(pending_.size()>=8) {
        auto lowest=std::min_element(pending_.begin(),pending_.end(),[](auto& a,auto& b){return a.priority<b.priority;});
        if(lowest->priority>=priority)return false;
        pending_.erase(lowest);
    }
    pending_.push_back({topic,detail,priority,time_+18});
    return true;
}

void PrtsNarrator::BeginBattle() {
    if(battleStarted_)return;
    battleStarted_=true;battleFinished_=false;battle_={};
    nextCombat_=time_+RandomDelay(45,70);
    // A battle boundary cancels exploration chatter. Further reinforcement waves
    // remain part of this same encounter and do not produce another briefing.
    pending_.clear();Queue(Topic::Wave,80);
}

void PrtsNarrator::FinishBattle() {
    if(battleFinished_)return;
    battleFinished_=true;pending_.clear();
    const int attacks=battle_.ranged+battle_.melee;
    Topic topic=battle_.damage==0?Topic::CleanClear:
        battle_.seconds<18&&attacks>=4?Topic::FastClear:Topic::Clear;
    // One result bubble includes one style observation from the whole encounter.
    // No rolling window, no mid-fight classification, and no series of reports.
    if(battle_.skills>=3)topic=Topic::Skill;
    else if(battle_.dodges>=4)topic=Topic::Mobile;
    else if(battle_.melee>=4&&battle_.melee>battle_.ranged)topic=Topic::Melee;
    else if(battle_.ranged>=12&&battle_.ranged>battle_.melee*2)topic=Topic::Ranged;
    else if(battle_.switches>=2)topic=Topic::Switch;
    std::string result;
    if(topic==Topic::Skill||topic==Topic::Mobile||topic==Topic::Melee||topic==Topic::Ranged||topic==Topic::Switch) {
        result=lastLowHealth_?"清理完成，当前生命状态危险。":
            battle_.damage==0?"清理完成，全程未受伤。":"清理完成，期间有受伤。";
        for(const auto& lines:kLines)if(lines.topic==topic) {
            result+=lines.text[variants_[static_cast<int>(topic)]%3];break;
        }
    }
    Queue(topic,96,result);
    nextExploration_=time_+RandomDelay(30,50);
}

void PrtsNarrator::Notify(Event event,const std::string& detail) {
    lastAction_=time_;idle_=0;
    const bool fighting=battleStarted_&&!battleFinished_;
    switch(event) {
    case Event::Ranged:if(fighting)++battle_.ranged;else ++exploreShots_;break;
    case Event::Melee:if(fighting)++battle_.melee;break;
    case Event::Skill:if(fighting)++battle_.skills;break;
    case Event::Dodge:if(fighting)++battle_.dodges;break;
    case Event::Jump:if(!fighting)++exploreJumps_;break;
    case Event::Switch:if(fighting)++battle_.switches;break;
    case Event::Wave:BeginBattle();break;
    case Event::Damage:if(fighting)++battle_.damage;break;
    case Event::Clear:battleClearRequested_=true;break;
    case Event::Investigate:Queue(Topic::Investigate,78,detail);break;
    case Event::Unplanned:++defiance_;if(!fighting)Queue(Topic::Disobey,55);break;
    case Event::Locked:if(!fighting)Queue(Topic::Locked,60);break;
    case Event::Defeat:
        if(!defeatLatched_){defeatLatched_=true;battleFinished_=true;pending_.clear();Queue(Topic::Defeat,100);}
        break;
    // Damage, corrosion, reload and switching are recorded without automatic
    // commentary during combat. Combat status remains visible in the HUD.
    default:break;
    }
}

bool PrtsNarrator::Relevant(Topic topic,const Observation& o) const {
    if(topic==Topic::CombatAside||topic==Topic::LowHealth||topic==Topic::Hurt||topic==Topic::Corrosion)
        return battleStarted_&&!battleFinished_&&!o.dead&&
            (topic!=Topic::LowHealth||o.lowHealth)&&(topic!=Topic::Corrosion||o.corroded);
    if(topic==Topic::Idle)return !o.threat&&idle_>=20;
    if(topic==Topic::Backtrack)return backtrackLatched_&&!o.threat;
    if(topic==Topic::ExploreAside||topic==Topic::EmptyFire||topic==Topic::Vertical||topic==Topic::Disobey||topic==Topic::Locked)
        return (!battleStarted_||battleFinished_)&&!o.dead;
    return true;
}

void PrtsNarrator::RandomAside(const Observation& o,bool combat) {
    std::vector<Topic> options;
    if(combat) {
        options.push_back(Topic::CombatAside);
        if(o.lowHealth)options.push_back(Topic::LowHealth);
        if(o.corroded)options.push_back(Topic::Corrosion);
        if(battle_.damage>0)options.push_back(Topic::Hurt);
    } else {
        if(scene_.ending)return;
        if(backtrackLatched_)options.push_back(Topic::Backtrack);
        if(idle_>=20)options.push_back(Topic::Idle);
        if(o.elevated&&exploreJumps_>=2)options.push_back(Topic::Vertical);
        if(exploreShots_>=12)options.push_back(Topic::EmptyFire);
        if(options.empty())options.push_back(scene_.system?Topic::System:Topic::ExploreAside);
    }
    // Randomness changes timing/choice, never establishes evidence or reveals a
    // secret. Each scene allows at most two exploration asides and one in combat.
    const auto topic=options[Random()%options.size()];
    if(Queue(topic,35)) {if(combat)++combatAsides_;else ++explorationAsides_;}
}

void PrtsNarrator::Update(float dt,const Observation& o) {
    if(!std::isfinite(dt)||dt<=0)return;
    dt=std::min(dt,.1F);time_+=dt;age_+=dt;lastLowHealth_=o.lowHealth;
    if(o.dead)Notify(Event::Defeat);
    else if(battleClearRequested_)FinishBattle();
    battleClearRequested_=false;
    if(o.threat&&!battleStarted_&&!battleFinished_)BeginBattle();
    const bool fighting=battleStarted_&&!battleFinished_;
    if(fighting)battle_.seconds+=dt;
    const bool moving=std::abs(o.deltaX)>.1F;
    if(moving){idle_=0;lastAction_=time_;}else idle_+=dt;
    if(o.hasForward&&!fighting&&o.deltaX<-.1F) {
        backtrack_+=dt;forward_=0;
        if(backtrack_>=1.8F&&!backtrackLatched_){backtrackLatched_=true;++defiance_;Queue(Topic::Backtrack,50);}
    } else if(o.deltaX>.1F) {
        forward_+=dt;if(forward_>.8F){backtrack_=0;backtrackLatched_=false;}
    }
    if(!o.dead&&!Visible()&&pending_.empty()) {
        if(fighting&&combatAsides_<1&&time_>=nextCombat_) {
            nextCombat_=time_+RandomDelay(45,70);RandomAside(o,true);
        } else if(!fighting&&explorationAsides_<2&&time_>=nextExploration_) {
            nextExploration_=time_+RandomDelay(30,50);RandomAside(o,false);
        }
    }
    std::erase_if(pending_,[&](const auto& item){return item.expires<time_||!Relevant(item.topic,o);});
    if(pending_.empty())return;
    const auto best=std::max_element(pending_.begin(),pending_.end(),[](auto& a,auto& b){return a.priority<b.priority;});
    const bool result=best->priority>=90&&best->priority>priority_&&age_>=.9F;
    const bool story=best->priority>=75&&best->priority>priority_&&age_>=2.5F;
    if(best->topic!=Topic::Defeat&&!result&&!story&&(Visible()||age_<14))return;
    const auto item=*best;pending_.erase(best);Show(item.topic,item.detail,item.priority);
}

void PrtsNarrator::Show(Topic topic,const std::string& detail,int priority) {
    text_=detail;topic_=topic;priority_=priority;age_=0;duration_=7;
    const auto index=static_cast<int>(topic);
    if(topic!=Topic::Navigation)for(const auto& lines:kLines)if(lines.topic==topic) {
        unsigned variant=variants_[index]++%3;
        if(topic==Topic::Disobey||topic==Topic::Hidden)variant=std::min(2,std::max(0,defiance_-1));
        text_=lines.text[variant];break;
    }
    if(!detail.empty()) {text_=detail;duration_=topic==Topic::Investigate?10:7;}
    last_[index]=time_;++revision_;
}

float PrtsNarrator::Opacity() const {return std::clamp((duration_-age_)/.5F,0.0F,1.0F);}
bool PrtsNarrator::Caution() const {
    return topic_==Topic::Disobey||topic_==Topic::Hidden||topic_==Topic::Locked||
        topic_==Topic::Hurt||topic_==Topic::Corrosion||topic_==Topic::LowHealth||topic_==Topic::Defeat;
}
const char* PrtsNarrator::Channel() const {
    if(Caution())return "RHODES ISLAND / RISK ASSESSMENT";
    if(topic_==Topic::Navigation||topic_==Topic::Revisit)return "RHODES ISLAND / NAVIGATION";
    if(topic_==Topic::Ranged||topic_==Topic::Melee||topic_==Topic::Skill||topic_==Topic::Mobile||
       topic_==Topic::Wave||topic_==Topic::Reload||topic_==Topic::Switch||topic_==Topic::Clear||
       topic_==Topic::CleanClear||topic_==Topic::FastClear)return "RHODES ISLAND / TACTICAL ANALYSIS";
    return "RHODES ISLAND / OBSERVATION";
}
std::string PrtsNarrator::GlyphText() {
    std::string text="请沿主路继续前进前往任务路线已完成由你决定去向";
    for(const auto& lines:kLines)for(const auto* line:lines.text)text+=line;
    return text;
}
