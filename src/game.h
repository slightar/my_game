#pragma once

#include "audio_system.h"
#include "boss.h"
#include "game_types.h"
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

    AudioSystem audio_;
    UiFont uiFont_;
    Player player_;
    Boss boss_;
    std::vector<Bullet> bullets_;
    bool started_ = false;
};
