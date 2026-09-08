#pragma once

#include "audio_system.h"
#include "boss.h"
#include "character_art.h"
#include "game_types.h"
#include "main_menu.h"
#include "player.h"
#include "ui_font.h"

#include <vector>

class Game {
public:
    Game();

    void Update(float deltaTime);
    void Draw() const;

private:
    void Reset();
    void UpdateBullets(float deltaTime);
    void DrawPauseMenu() const;

    AudioSystem audio_;
    UiFont uiFont_;
    CharacterArt characterArt_;
    MainMenu mainMenu_;
    Player player_;
    Boss boss_;
    std::vector<Bullet> bullets_;
    bool inBattle_ = false;
    bool paused_ = false;
    int pauseSelection_ = 0;
};
