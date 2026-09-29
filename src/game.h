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

private:
    void Reset();
    void SwitchOperator(int slot);
    void DrawOperatorSlots() const;
    void ReloadCharacters();
    void UpdateBullets(float deltaTime);
    void UpdateCamera(float deltaTime);
    void DrawMap() const;
    void DrawEncounterBanner() const;
    void DrawPauseMenu() const;
    bool EncounterCleared() const;

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
