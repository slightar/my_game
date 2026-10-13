#include "developer_room.h"
#include "ui_font.h"
#include "terminal_ui.h"
#include "ui_theme.h"
#include <algorithm>

DeveloperCommand DeveloperRoom::Controls(float dt,float cameraX) {
    pointer_=ReadUiPointer(pointerState_);noticeTime_=std::max(0.0F,noticeTime_-dt);
    if(IsKeyPressed(KEY_ESCAPE))return DeveloperCommand::Leave;
    if(IsKeyPressed(KEY_F1)||pointer_.Clicked(DeveloperUi::Hide))panelOpen_=!panelOpen_;
    if(IsKeyPressed(KEY_F4))ToggleFrozen();
    if(IsKeyPressed(KEY_F5))RequestStep();
    if(IsKeyPressed(KEY_F9))ToggleGod();
    if(IsKeyPressed(KEY_ENTER))return DeveloperCommand::Spawn;
    if(IsKeyPressed(KEY_DELETE))return DeveloperCommand::Clear;
    if(IsKeyPressed(KEY_F3))return DeveloperCommand::Restore;
    if(IsKeyPressed(KEY_TAB))SetFilter(static_cast<DeveloperFilter>((int(filter_)+1)%3));
    if(IsKeyPressed(KEY_UP))MoveSelection(-1);
    if(IsKeyPressed(KEY_DOWN))MoveSelection(1);
    if(IsKeyPressed(KEY_PAGE_UP))MoveSelection(-kPageSize);
    if(IsKeyPressed(KEY_PAGE_DOWN))MoveSelection(kPageSize);
    if(IsKeyPressed(KEY_KP_SUBTRACT)||IsKeyPressed(KEY_MINUS))AdjustCount(-1);
    if(IsKeyPressed(KEY_KP_ADD)||IsKeyPressed(KEY_EQUAL))AdjustCount(1);
    if(panelOpen_) {
        if(pointer_.Hit(DeveloperUi::Panel)) {
            const float wheel=GetMouseWheelMove();if(wheel!=0)MoveSelection(wheel>0?-1:1);
        }
        for(int i=0;i<3;++i)if(pointer_.Clicked(DeveloperUi::Filter(i)))SetFilter(static_cast<DeveloperFilter>(i));
        const auto list=VisibleCatalog();const int start=ListStart();
        for(int i=0;i<kPageSize&&start+i<int(list.size());++i)if(pointer_.Clicked(DeveloperUi::Row(i)))Select(list[start+i]);
        if(pointer_.Clicked(DeveloperUi::Previous))MoveSelection(-kPageSize);
        if(pointer_.Clicked(DeveloperUi::Next))MoveSelection(kPageSize);
        if(pointer_.Clicked(DeveloperUi::Minus))AdjustCount(-1);
        if(pointer_.Clicked(DeveloperUi::Plus))AdjustCount(1);
        if(pointer_.Clicked(DeveloperUi::Position))SetSpawnMode((spawnMode_+1)%3);
        if(pointer_.Clicked(DeveloperUi::Difficulty))CycleDifficulty();
        if(pointer_.Clicked(DeveloperUi::Speed))CycleSpeed();
        if(pointer_.Clicked(DeveloperUi::Freeze))ToggleFrozen();
        if(pointer_.Clicked(DeveloperUi::God))ToggleGod();
        if(pointer_.Clicked(DeveloperUi::Restore))return DeveloperCommand::Restore;
        if(pointer_.Clicked(DeveloperUi::Clear))return DeveloperCommand::Clear;
        if(pointer_.Clicked(DeveloperUi::Spawn))return DeveloperCommand::Spawn;
    }
    const bool shift=IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT);
    const Rectangle field{panelOpen_?360.0F:0,252,panelOpen_?615.0F:975.0F,390};
    if(shift&&pointer_.Clicked(field)){Mark(cameraX+pointer_.position.x-640);Notice("已标记生成位置");}
    return DeveloperCommand::None;
}
bool DeveloperRoom::GlyphsSupported(const UiFont& font) const {return font.SupportsText(Glyphs());}
void DeveloperRoom::DrawControls(const UiFont& font) const {
    using namespace TerminalUi;
    using namespace DeveloperUi;
    SetPointerState(pointer_);
    Surface({447,20,797,122},Fade(Ink,.95F),Blue);
    font.Draw("开发者房间",465,31,27,Paper);
    font.Draw("SANDBOX / ENEMY LAB",692,42,13,Muted);
    const auto& boss=Boss();
    font.Draw(TextFormat("小怪 %d / %d · 领袖 %s · 测试 %.1fs",mobs_.Remaining(),kMaxMobs,
        boss.Active()?StoryBossName(boss.Kind()):"--",seconds_),465,75,16,Paper);
    if(boss.Present()) {
        TacticalUi::DrawProgressLine({958,82},256,boss.Health()/boss.MaxHealth(),TacticalUi::kRed,5);
        font.Draw(TextFormat("HP %.0f / %.0f%s",boss.Health(),boss.MaxHealth(),boss.TwoLives()?(boss.PhaseTwo()?" / II":" / I"):""),958,98,13,Paper);
    }
    const char* hint="Enter 生成 · Shift + 点击标记 · Esc 返回主页";
    if(boss.TwoLives()&&boss.Active())hint=boss.State()==StoryBossState::Revive?"倒地重生 · 第二条生命尚未解除封锁":
        boss.Invulnerability()>0?"重生无敌 · 先避开危险区域":boss.Kind()==StoryBossKind::FrostNova?
        boss.PhaseTwo()?"第二阶段 · 寒潮与三重黑冰":"第一阶段 · 寒冷与冰环":
        boss.PhaseTwo()?"第二阶段 · 毁灭 / 投枪与献祭":"第一阶段 · 行军 / 高防御四连击";
    font.Draw(hint,465,111,14,boss.TwoLives()?Orange:Muted);
    Button(font,Hide,panelOpen_?"隐藏 / F1":"显示 / F1",pointer_.Hit(Hide),Ink,Paper);
    if(panelOpen_) {
        Button(font,Difficulty,DifficultyName(),pointer_.Hit(Difficulty),Ink,Paper);
        Button(font,DeveloperUi::Speed,TextFormat("速度 × %.1f",Speed()),pointer_.Hit(DeveloperUi::Speed),Ink,Paper);
        Button(font,Freeze,frozen_?"敌人冻结 / F4":"敌人运行 / F4",frozen_,Ink,Paper);
        Button(font,DeveloperUi::God,god_?"无敌开启 / F9":"无敌关闭 / F9",god_,Ink,Paper);
        Button(font,Restore,"恢复角色 / F3",pointer_.Hit(Restore),Ink,Paper);
        Button(font,DeveloperUi::Clear,"清空敌人 / Delete",pointer_.Hit(DeveloperUi::Clear),Ink,Paper);
        Surface(Panel,Fade(Ink,.95F),Blue);
        const char* filters[]{"全部","小怪","领袖"};
        for(int i=0;i<3;++i)Button(font,Filter(i),filters[i],int(filter_)==i,int(filter_)==i?Blue:Ink,Paper);
        const auto list=VisibleCatalog();const int start=ListStart();
        for(int i=0;i<kPageSize&&start+i<int(list.size());++i) {
            const int index=list[start+i];const char* name=index<int(kEnemyKindCount)?EnemyData(static_cast<EnemyKind>(index)).name:StoryBossName(static_cast<StoryBossKind>(index-int(kEnemyKindCount)+1));
            const auto row=Row(i);const bool selected=index==selected_;
            Surface(PressedRect(row),selected?Blue:Color{44,51,58,255});
            Fit(font,std::string(index<int(kEnemyKindCount)?"ENEMY / ":"BOSS / ")+name,{row.x+8,row.y+4,row.width-16,20},17,Paper);
            PressedVeil(row);
        }
        Button(font,Previous,"<",pointer_.Hit(Previous),Ink,Paper);Button(font,Next,">",pointer_.Hit(Next),Ink,Paper);
        Fit(font,TextFormat("%d / %d",start/kPageSize+1,(int(list.size())+kPageSize-1)/kPageSize),{113,492,153,22},16,Paper);
        Button(font,Minus,"-",!SelectedBoss()&&pointer_.Hit(Minus),Ink,SelectedBoss()?Muted:Paper);
        Button(font,Plus,"+",!SelectedBoss()&&pointer_.Hit(Plus),Ink,SelectedBoss()?Muted:Paper);
        Fit(font,TextFormat("数量  %d%s",Count(),SelectedBoss()?" / 单名领袖":""),{88,537,200,21},16,Paper);
        const char* positions[]{"玩家右侧","玩家左侧","标记位置"};
        Button(font,Position,positions[spawnMode_],pointer_.Hit(Position),Ink,Paper);
        Button(font,Spawn,"生成所选敌人 / Enter",pointer_.Hit(Spawn),Blue,Paper);
        Surface({447,200,797,42},Fade(Ink,.85F));
        Wrap(font,noticeTime_>0?notice_:SelectedTactic(),{461,209,770,27},14,noticeTime_>0?Orange:Paper);
    }
    Surface({610,671,430,22},Fade(Ink,.9F));
    Fit(font,"↑↓ 选择 · Tab 分类 · +/- 数量 · F5 单步 · 1/2 切换",{618,674,414,17},12,Paper);
    font.Draw("独立测试 · 参数仅影响新生成单位",36,700,13,Muted);
}
void DeveloperRoom::DrawMarker(const UiFont& font,Vector2 player) const {
    const float x=SpawnX(player);
    DrawEllipseLines(int(x),int(GameConfig::kFloorY),30,7,TacticalUi::kCyan);
    DrawLineEx({x,GameConfig::kFloorY-30},{x,GameConfig::kFloorY-12},2,TacticalUi::kCyan);
    font.Draw("生成位置",x-28,GameConfig::kFloorY-53,14,TacticalUi::kCyan);
}
