#pragma once
#include "enemy_system.h"
#include "story_boss.h"
#include "ui_input.h"
#include <string>
#include <algorithm>

class UiFont;
enum class DeveloperCommand {None,Spawn,Clear,Restore,Leave};
enum class DeveloperFilter {All,Mobs,Bosses};

// Isolated sandbox owns enemies and fixed spawn parameters, never progression
// or adaptive learning. View/input and Game's operator integration are separate.
class DeveloperRoom {
public:
    static constexpr int kMaxMobs=64,kPageSize=6;
    void Enter(float width);
    void Clear();
    bool SpawnSelected(Vector2 player);
    bool UpdateEnemies(float dt,Vector2 player,int playerFacing=1);
    DeveloperCommand Controls(float dt,float cameraX);
    void DrawControls(const UiFont& font) const;
    void DrawMarker(const UiFont& font,Vector2 player) const;
    void Select(int index);
    int Selected() const {return selected_;}
    const char* SelectedName() const;
    std::string SelectedTactic() const;
    int CatalogSize() const {return int(kEnemyKindCount)+kStoryBossCount;}
    bool SelectedBoss() const {return selected_>=int(kEnemyKindCount);}
    int Count() const {return SelectedBoss()?1:count_;}
    void AdjustCount(int delta){if(!SelectedBoss())count_=std::clamp(count_+delta,1,20);}
    void SetFilter(DeveloperFilter filter);
    std::vector<int> VisibleCatalog() const;
    int ListStart() const;
    void SetSpawnMode(int mode){spawnMode_=std::clamp(mode,0,2);}
    void Mark(float x){marker_=std::clamp(x,115.0F,width_-115);spawnMode_=2;}
    float SpawnX(Vector2 player) const;
    void CycleDifficulty(){difficultyIndex_=(difficultyIndex_+1)%3;}
    CombatDifficulty Profile() const;
    const char* DifficultyName() const;
    void ToggleFrozen(){frozen_=!frozen_;}
    bool Frozen() const {return frozen_;}
    void RequestStep(){step_=true;}
    void ToggleGod(){god_=!god_;}
    bool God() const {return god_;}
    void CycleSpeed(){speedIndex_=(speedIndex_+1)%3;}
    float Speed() const {return speedIndex_==0?.5F:speedIndex_==2?2.0F:1.0F;}
    float Width() const {return width_;}
    float Seconds() const {return seconds_;}
    EnemySystem& Mobs(){return mobs_;}
    const EnemySystem& Mobs() const {return mobs_;}
    StoryBoss& Boss(){return boss_;}
    const StoryBoss& Boss() const {return boss_;}
    bool PanelOpen() const {return panelOpen_;}
    bool GlyphsSupported(const UiFont& font) const;
    static std::string Glyphs();
private:
    void MoveSelection(int delta);
    void Notice(std::string message){notice_=std::move(message);noticeTime_=3;}
    EnemySystem mobs_;
    StoryBoss boss_;
    int selected_=0,count_=1,spawnMode_=0,difficultyIndex_=0,speedIndex_=1;
    DeveloperFilter filter_=DeveloperFilter::All;
    bool frozen_=false,god_=true,step_=false,panelOpen_=true;
    float width_=2600,marker_=1300,seconds_=0,noticeTime_=0;
    std::string notice_;
    UiPointerState pointerState_{};
    UiPointer pointer_{};
};

namespace DeveloperUi {
inline constexpr Rectangle Panel{24,252,328,410};
inline constexpr Rectangle Filter(int i){return {36.0F+i*101,264,95,28};}
inline constexpr Rectangle Row(int i){return {36,303.0F+i*30,304,27};}
inline constexpr Rectangle Previous{36,490,65,27},Next{275,490,65,27};
inline constexpr Rectangle Minus{36,532,46,30},Plus{294,532,46,30};
inline constexpr Rectangle Position{36,572,304,31},Spawn{36,614,304,36};
inline constexpr Rectangle Difficulty{234,162,198,32},Speed{234,206,198,32};
inline constexpr Rectangle Freeze{450,154,180,33},God{640,154,180,33};
inline constexpr Rectangle Restore{830,154,180,33},Clear{1020,154,216,33};
inline constexpr Rectangle Hide{1140,30,96,26};
}
