#include "developer_room.h"
#include <algorithm>

void DeveloperRoom::Enter(float width) {
    *this=DeveloperRoom{};width_=std::max(1280.0F,width);marker_=width_*.55F;Clear();
}
void DeveloperRoom::Clear() {
    mobs_.ResetEncounter({},width_*.55F,80,width_-80);boss_.Reset();step_=false;
}
void DeveloperRoom::Select(int index){selected_=std::clamp(index,0,CatalogSize()-1);}
const char* DeveloperRoom::SelectedName() const {
    return !SelectedBoss()?EnemyData(static_cast<EnemyKind>(selected_)).name:StoryBossName(static_cast<StoryBossKind>(selected_-int(kEnemyKindCount)+1));
}
std::string DeveloperRoom::SelectedTactic() const {
    if(!SelectedBoss())return EnemyData(static_cast<EnemyKind>(selected_)).tactic;
    switch(static_cast<StoryBossKind>(selected_-int(kEnemyKindCount)+1)) {
    case StoryBossKind::FrostNova:return "双阶段：扇形冰刃、变序冰雨、黑冰封印；重生后召唤寒潮。";
    case StoryBossKind::Patriot:return "双阶段：行军高防御与四连击；重生后落矛阵、献祭与投枪。";
    case StoryBossKind::W:return "走位、连环爆破、低空榴弹、地雷与 C4；留意空隙。";
    default:return "红线锁定后移动闪避，离开下斩落点，落地后反击。";
    }
}
std::vector<int> DeveloperRoom::VisibleCatalog() const {
    std::vector<int> result;
    const int begin=filter_==DeveloperFilter::Bosses?int(kEnemyKindCount):0;
    const int end=filter_==DeveloperFilter::Mobs?int(kEnemyKindCount):CatalogSize();
    for(int i=begin;i<end;++i)result.push_back(i);return result;
}
void DeveloperRoom::SetFilter(DeveloperFilter filter){filter_=filter;Select(VisibleCatalog().front());}
void DeveloperRoom::MoveSelection(int delta) {
    const auto list=VisibleCatalog();auto found=std::find(list.begin(),list.end(),selected_);
    const int position=found==list.end()?0:int(found-list.begin());
    selected_=list[(position+delta%int(list.size())+int(list.size()))%int(list.size())];
}
int DeveloperRoom::ListStart() const {
    const auto list=VisibleCatalog();const auto found=std::find(list.begin(),list.end(),selected_);
    return found==list.end()?0:int(found-list.begin())/kPageSize*kPageSize;
}
float DeveloperRoom::SpawnX(Vector2 player) const {
    return std::clamp(spawnMode_==2?marker_:player.x+(spawnMode_==0?340:-340),115.0F,width_-115);
}
CombatDifficulty DeveloperRoom::Profile() const {
    return difficultyIndex_==0?CombatDifficulty{}:CombatDifficulty::FromAbility(difficultyIndex_==1?.60F:1.0F);
}
const char* DeveloperRoom::DifficultyName() const {return difficultyIndex_==0?"原始参数":difficultyIndex_==1?"进阶参数":"高压参数";}
bool DeveloperRoom::SpawnSelected(Vector2 player) {
    const float x=SpawnX(player);
    if(SelectedBoss()) {
        boss_.StartSandbox(static_cast<StoryBossKind>(selected_-int(kEnemyKindCount)+1),width_,x,Profile());
        Notice(std::string("已生成领袖：")+SelectedName()+" · 仅保留一名领袖");return true;
    }
    mobs_.PruneDead();
    if(mobs_.Remaining()+Count()>kMaxMobs){Notice("场上小怪已达到数量上限，请先清场");return false;}
    for(int i=0;i<Count();++i) {
        const float at=std::clamp(x+(i-(Count()-1)/2.0F)*60,115.0F,width_-115);
        mobs_.Spawn(static_cast<EnemyKind>(selected_),at,Profile(),player.x>=at?1:-1);
    }
    Notice(std::string("已生成：")+SelectedName()+" × "+std::to_string(Count()));return true;
}
bool DeveloperRoom::UpdateEnemies(float dt,Vector2 player,int playerFacing) {
    if(frozen_&&!step_)return false;
    const float tick=frozen_?1.0F/60:dt;step_=false;seconds_+=tick;
    mobs_.Update(tick,player);boss_.Update(tick,player,playerFacing);mobs_.PruneDead();return true;
}
std::string DeveloperRoom::Glyphs() {
    return "扇形冰刃变序落矛阵连环爆破低空空隙冲刺空中飞刀半血开发者房间敌人生成测试场全部小怪领袖已选择数量上限请先清场恢复角色冻结敌人无敌开启关闭原始参数进阶高压仅影响新生成单位暂停单步标记位置玩家左侧右侧放置上一页下一页返回主页操作控制隐藏显示秒已生成仅保留一名替换当前血生命近身斩击闪现绕后走位换位反击榴弹连射地雷布设清空伤害判定视角按住点击速度过渡地图存档独立测试同场弹药技能冷却重置全员失去行动能力×";
}
