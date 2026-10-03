#pragma once

#include "audio_system.h"
#include "boss.h"
#include "enemy_system.h"
#include "enemy_renderer.h"
#include "character_art.h"
#include "game_types.h"
#include "main_menu.h"
#include "player.h"
#include "ui_font.h"
#include "world_scenery.h"
#include "scene_traversal.h"
#include "world_encounter.h"
#include "prts_narrator.h"
#include "character_repository.h"

#include <vector>
#include <array>

class Game {
public:
    Game();

    void Update(float deltaTime);
    void Draw(float displayScale, Vector2 displayOffset) const;
    [[nodiscard]] bool ShouldQuit() const;
    [[nodiscard]] int RemainingTrialEnemies() const { return enemyTrial_ ? enemies_.Remaining() : 0; }
    [[nodiscard]] int PendingTrialEnemies() const { return enemyTrial_ ? enemies_.Pending() : 0; }
    [[nodiscard]] const std::string& WorldRegion() const { return worldRegion_; }
    [[nodiscard]] bool InWorldPreview() const { return worldPreview_; }
    [[nodiscard]] Vector2 WorldPlayerPosition() const { return player_->Position(); }

private:
    // The retained world prototype is entered only by its integration test.
    friend struct WorldPreviewTestAccess;
    void Reset();
    void SwitchOperator(int slot);
    void DrawOperatorSlots() const;
    void ReloadCharacters();
    void UpdateBullets(float deltaTime);
    void DrawBullets() const;
    void UpdateCamera(float deltaTime);
    void DrawMap() const;
    void DrawEncounterBanner() const;
    void DrawPauseMenu() const;
    bool EncounterCleared() const;
    void EnterWorldRegion(const std::string& id, const std::string& from = "");
    void UpdateWorldPreview(float deltaTime);
    void DrawWorldPreview(float displayScale, Vector2 displayOffset) const;
    void ReportPrtsActions();
    void UpdatePrts(float deltaTime,float deltaX,bool hasForward,bool threat,bool combatArea);
    void DrawPrts(bool upper,bool combat) const;

    AudioSystem audio_;
    UiFont uiFont_;
    CharacterArt characterArt_;
    MainMenu mainMenu_;
    std::vector<Character> customCharacters_;
    std::array<Player, 2> operators_;
    Player* player_ = &operators_[0];
    int activeOperator_ = 0;
    Boss boss_;
    EnemySystem enemies_;
    EnemyRenderer enemyRenderer_;
    bool enemyTrial_ = false;
    std::vector<Bullet> bullets_;
    bool inBattle_ = false;
    WorldScenery worldScenery_;
    float worldTime_ = 0;
    PrtsNarrator prts_;
    SceneTraversal worldTraversal_;
    WorldEncounter worldEncounter_;
    float worldInspect_=0,worldNoteTime_=0;
    float worldTransition_ = -1;
    bool worldAttackHeld_ = false;
    std::string worldDestination_;
    bool worldTransitionLoaded_ = false;
    bool worldPreview_ = false;
    bool worldDesignPreview_ = false;
    std::string worldRegion_ = "clinic";
    bool paused_ = false;
    bool quitRequested_ = false;
    bool bossActive_ = false;
    float cameraX_ = static_cast<float>(GameConfig::kScreenWidth) / 2.0F;
    float gateCloseTimer_ = 0.0F;
    float encounterBannerTimer_ = 0.0F;
    int pauseSelection_ = 0;
    UiPointerState pointerState_;
    // Latest pointer snapshot, reused by the pause menu so buttons share the global press feedback.
    UiPointer pointer_{};
};
