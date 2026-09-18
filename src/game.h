#pragma once

#include "audio_system.h"
#include "boss.h"
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

    AudioSystem audio_;
    UiFont uiFont_;
    CharacterArt characterArt_;
    MainMenu mainMenu_;
    std::array<Player, 2> operators_;
    Player* player_ = &operators_[0];
    int activeOperator_ = 0;
    Boss boss_;
    std::vector<Bullet> bullets_;
    bool inBattle_ = false;
    bool paused_ = false;
    bool quitRequested_ = false;
    bool bossActive_ = false;
    float cameraX_ = static_cast<float>(GameConfig::kScreenWidth) / 2.0F;
    float gateCloseTimer_ = 0.0F;
    float encounterBannerTimer_ = 0.0F;
    int pauseSelection_ = 0;
    bool touchPreviouslyDown_ = false;
};
