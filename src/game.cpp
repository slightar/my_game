#include "game.h"

#include <algorithm>

Game::Game() {
    Reset();
}

void Game::Reset() {
    bullets_.clear();
    player_.Reset();
    boss_.Reset();
}

void Game::Update(float deltaTime) {
    if (!inBattle_) {
        if (mainMenu_.Update() == MenuAction::StartBattle) {
            Reset();
            inBattle_ = true;
        }
        return;
    }

    if (player_.IsDead() || boss_.IsDefeated()) {
        if (IsKeyPressed(KEY_ENTER)) {
            Reset();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            inBattle_ = false;
            mainMenu_.OpenHome();
        }
        return;
    }

    player_.Update(deltaTime, bullets_, audio_);
    boss_.Update(deltaTime, player_.Position());
    UpdateBullets(deltaTime);

    const bool touchingBoss = !boss_.IsDefeated() &&
                              CheckCollisionCircles(player_.Position(), player_.Radius(),
                                                    boss_.Position(), boss_.Radius());
    const bool hitByBossAttack = !boss_.IsDefeated() &&
                                 boss_.AttackHits(player_.Position(), player_.Radius());
    if (touchingBoss || hitByBossAttack) {
        if (player_.TakeDamage(boss_.Position())) {
            audio_.PlayPlayerHit();
        }
    }
}

void Game::UpdateBullets(float deltaTime) {
    for (Bullet& bullet : bullets_) {
        bullet.position.x += bullet.velocity.x * deltaTime;
        bullet.position.y += bullet.velocity.y * deltaTime;
        bullet.lifetime -= deltaTime;

        if (!boss_.IsDefeated() &&
            CheckCollisionCircles(bullet.position, bullet.radius,
                                  boss_.Position(), boss_.Radius())) {
            boss_.TakeDamage(bullet.damage);
            audio_.PlayBossHit();
            bullet.lifetime = 0.0F;
        }
    }

    std::erase_if(bullets_, [](const Bullet& bullet) {
        const Rectangle room = GameConfig::kRoom;
        const bool outside = bullet.position.x < room.x ||
                             bullet.position.x > room.x + room.width ||
                             bullet.position.y < room.y ||
                             bullet.position.y > room.y + room.height;
        return bullet.lifetime <= 0.0F || outside;
    });
}

void Game::Draw() const {
    if (!inBattle_) {
        mainMenu_.Draw(uiFont_);
        return;
    }

    ClearBackground({24, 27, 34, 255});
    DrawRectangleRec(GameConfig::kRoom, {222, 225, 229, 255});
    DrawRectangleLinesEx(GameConfig::kRoom, 5.0F, {88, 94, 105, 255});
    DrawRectangle(static_cast<int>(GameConfig::kRoom.x),
                  static_cast<int>(GameConfig::kFloorY),
                  static_cast<int>(GameConfig::kRoom.width),
                  static_cast<int>(GameConfig::kRoom.y + GameConfig::kRoom.height -
                                   GameConfig::kFloorY),
                  {112, 119, 130, 255});

    for (const Bullet& bullet : bullets_) {
        DrawCircleV(bullet.position, bullet.radius, {245, 183, 45, 255});
    }

    boss_.Draw(uiFont_);
    player_.Draw();
    player_.DrawHud(uiFont_, mainMenu_.SelectedOperatorName());
    boss_.DrawHud(uiFont_);

    if (player_.IsDead() || boss_.IsDefeated()) {
        DrawRectangle(0, 0, GameConfig::kScreenWidth, GameConfig::kScreenHeight,
                      Fade(BLACK, 0.58F));
        const char* title = player_.IsDead() ? "任务失败" : "弑君者已击败";
        const float titleWidth = uiFont_.Measure(title, 46.0F);
        uiFont_.Draw(title,
                     static_cast<float>(GameConfig::kScreenWidth) / 2.0F -
                         titleWidth / 2.0F,
                     290.0F, 46.0F, player_.IsDead() ? RED : SKYBLUE);
        const char* restart = "Enter 重新开始   Backspace 返回主页";
        const float restartWidth = uiFont_.Measure(restart, 24.0F);
        uiFont_.Draw(restart,
                     static_cast<float>(GameConfig::kScreenWidth) / 2.0F -
                         restartWidth / 2.0F,
                     360.0F, 24.0F, RAYWHITE);
    }
}
