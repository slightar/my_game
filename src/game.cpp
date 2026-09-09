#include "game.h"

#include <algorithm>
#include <cmath>

Game::Game() {
    Reset();
}

void Game::Reset() {
    bullets_.clear();
    player_.Reset(mainMenu_.SelectedOperator());
    boss_.Reset();
    paused_ = false;
    pauseSelection_ = 0;
}

void Game::Update(float deltaTime) {
    if (!inBattle_) {
        const MenuAction action = mainMenu_.Update();
        if (action == MenuAction::StartBattle) {
            Reset();
            inBattle_ = true;
        } else if (action == MenuAction::Quit) {
            quitRequested_ = true;
        }
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        paused_ = !paused_;
        pauseSelection_ = 0;
        return;
    }

    if (paused_) {
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
            pauseSelection_ = std::max(0, pauseSelection_ - 1);
        }
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
            pauseSelection_ = std::min(1, pauseSelection_ + 1);
        }
        if (IsKeyPressed(KEY_ENTER)) {
            if (pauseSelection_ == 0) {
                paused_ = false;
            } else {
                paused_ = false;
                inBattle_ = false;
                mainMenu_.OpenHome();
            }
        }
        return;
    }

    if (player_.IsDead() || boss_.IsDefeated()) {
        if (player_.IsDead()) {
            player_.UpdateDefeatAnimation(deltaTime);
        }
        if (boss_.IsDefeated()) {
            boss_.Update(deltaTime, player_.Position(),
                         player_.FacingDirection());
        }
        if (!player_.IsDead() || player_.DefeatAnimationFinished()) {
            if (IsKeyPressed(KEY_ENTER)) {
                Reset();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                inBattle_ = false;
                mainMenu_.OpenHome();
            }
        }
        return;
    }

    player_.Update(deltaTime, boss_.Position(), boss_.Radius(), bullets_, audio_);
    boss_.Update(deltaTime, player_.Position(), player_.FacingDirection());
    UpdateBullets(deltaTime);

    const bool touchingBoss = boss_.CanDealContactDamage() &&
                              CheckCollisionCircleRec(boss_.Position(), boss_.Radius(),
                                                      player_.Hitbox());
    const bool hitByBossAttack = !boss_.IsDefeated() &&
                                 boss_.AttackHits(player_.Hitbox(),
                                                  player_.ProjectileHitbox());
    if (touchingBoss || hitByBossAttack) {
        if (player_.TakeDamage(boss_.Position())) {
            audio_.PlayPlayerHit();
        }
    }
}

bool Game::ShouldQuit() const {
    return quitRequested_;
}

void Game::DrawPauseMenu() const {
    DrawRectangle(0, 0, GameConfig::kScreenWidth, GameConfig::kScreenHeight,
                  Fade(BLACK, 0.68F));
    const Rectangle panel{410.0F, 176.0F, 460.0F, 370.0F};
    DrawRectangleRec(panel, Color{31, 36, 42, 248});
    DrawRectangleLinesEx(panel, 3.0F, Color{0, 156, 211, 255});
    DrawRectangle(static_cast<int>(panel.x), static_cast<int>(panel.y),
                  static_cast<int>(panel.width), 8, Color{255, 91, 18, 255});

    const char* title = "行动暂停";
    const float titleWidth = uiFont_.Measure(title, 42.0F);
    uiFont_.Draw(title, 640.0F - titleWidth / 2.0F, 220.0F,
                 42.0F, RAYWHITE);

    const Rectangle options[2] = {
        {470.0F, 310.0F, 340.0F, 65.0F},
        {470.0F, 395.0F, 340.0F, 65.0F}};
    const char* labels[2] = {"继续作战", "返回主页"};
    for (int index = 0; index < 2; ++index) {
        const bool selected = pauseSelection_ == index;
        DrawRectangleRec(options[index],
                         selected ? Color{0, 156, 211, 255}
                                  : Color{62, 69, 76, 255});
        DrawRectangleLinesEx(options[index], selected ? 3.0F : 1.0F,
                             selected ? RAYWHITE : Fade(RAYWHITE, 0.35F));
        const float labelWidth = uiFont_.Measure(labels[index], 25.0F);
        uiFont_.Draw(labels[index],
                     options[index].x + (options[index].width - labelWidth) / 2.0F,
                     options[index].y + 18.0F, 25.0F, RAYWHITE);
    }
    uiFont_.Draw("W/S 选择   Enter 确认   Esc 继续",
                 486.0F, 496.0F, 18.0F, Fade(RAYWHITE, 0.72F));
}

void Game::UpdateBullets(float deltaTime) {
    for (Bullet& bullet : bullets_) {
        if (bullet.activationDelay > 0.0F) {
            bullet.activationDelay =
                std::max(0.0F, bullet.activationDelay - deltaTime);
            continue;
        }
        bullet.position.x += bullet.velocity.x * deltaTime;
        bullet.position.y += bullet.velocity.y * deltaTime;
        bullet.lifetime -= deltaTime;

        if (bullet.destroysEnemyProjectile &&
            boss_.DestroyProjectileAt(bullet.position, bullet.radius + 7.0F)) {
            bullet.lifetime = 0.0F;
            continue;
        }

        if (!boss_.IsDefeated() &&
            CheckCollisionCircles(bullet.position, bullet.radius,
                                  boss_.Position(), boss_.Radius())) {
            boss_.TakeDamage(bullet.damage);
            if (bullet.stunDuration > 0.0F) {
                boss_.Stun(bullet.stunDuration);
            }
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
        mainMenu_.Draw(uiFont_, characterArt_);
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
        if (bullet.activationDelay > 0.0F) {
            continue;
        }
        if (bullet.kind == BulletKind::MeleeSlash) {
            if (characterArt_.HasTexasSkill2Effects()) {
                characterArt_.DrawTexasSkill2Slash(
                    bullet.position, player_.FacingDirection(),
                    bullet.visualVariant == 1,
                    bullet.damageType == DamageType::Arts,
                    bullet.lifetime, bullet.radius);
                continue;
            }
            const Color slashColor = bullet.damageType == DamageType::Arts
                                         ? Color{237, 42, 57, 255}
                                         : Color{215, 230, 240, 255};
            DrawCircleV(bullet.position, bullet.radius,
                        Fade(slashColor, 0.08F));
            DrawLineEx({bullet.position.x - bullet.radius * 0.75F,
                        bullet.position.y - bullet.radius * 0.58F},
                       {bullet.position.x + bullet.radius * 0.75F,
                        bullet.position.y + bullet.radius * 0.58F},
                       13.0F, Fade(BLACK, 0.72F));
            DrawLineEx({bullet.position.x - bullet.radius * 0.72F,
                        bullet.position.y - bullet.radius * 0.55F},
                       {bullet.position.x + bullet.radius * 0.72F,
                        bullet.position.y + bullet.radius * 0.55F},
                       8.0F, Fade(slashColor, 0.86F));
            if (bullet.damageType == DamageType::Arts) {
                DrawLineEx({bullet.position.x - bullet.radius * 0.68F,
                            bullet.position.y + bullet.radius * 0.62F},
                           {bullet.position.x + bullet.radius * 0.68F,
                            bullet.position.y - bullet.radius * 0.62F},
                           11.0F, Fade(BLACK, 0.68F));
                DrawLineEx({bullet.position.x - bullet.radius * 0.64F,
                            bullet.position.y + bullet.radius * 0.58F},
                           {bullet.position.x + bullet.radius * 0.64F,
                            bullet.position.y - bullet.radius * 0.58F},
                           6.0F, Fade(RED, 0.78F));
            }
            continue;
        }
        if (bullet.kind == BulletKind::SwordWave) {
            const float direction = bullet.velocity.x >= 0.0F ? 1.0F : -1.0F;
            DrawLineEx({bullet.position.x - direction * 14.0F,
                        bullet.position.y - 28.0F},
                       {bullet.position.x + direction * 9.0F,
                        bullet.position.y},
                       7.0F, Fade(RAYWHITE, 0.88F));
            DrawLineEx({bullet.position.x + direction * 9.0F,
                        bullet.position.y},
                       {bullet.position.x - direction * 14.0F,
                        bullet.position.y + 28.0F},
                       7.0F, Fade(SKYBLUE, 0.82F));
            continue;
        }
        if (bullet.kind == BulletKind::FallingSword) {
            DrawLineEx({bullet.position.x, bullet.position.y - 34.0F},
                       {bullet.position.x, bullet.position.y + 22.0F},
                       7.0F, RAYWHITE);
            DrawLineEx({bullet.position.x - 12.0F, bullet.position.y - 7.0F},
                       {bullet.position.x + 12.0F, bullet.position.y - 7.0F},
                       5.0F, Color{95, 174, 231, 255});
            DrawCircleV(bullet.position, 22.0F, Fade(SKYBLUE, 0.12F));
            continue;
        }
        const float speed = std::sqrt(bullet.velocity.x * bullet.velocity.x +
                                      bullet.velocity.y * bullet.velocity.y);
        const Vector2 direction = speed > 0.0F
                                      ? Vector2{bullet.velocity.x / speed,
                                                bullet.velocity.y / speed}
                                      : Vector2{1.0F, 0.0F};
        const Vector2 tail{bullet.position.x - direction.x * 24.0F,
                           bullet.position.y - direction.y * 24.0F};
        DrawLineEx(tail, bullet.position, 6.0F, Fade(ORANGE, 0.78F));
        DrawCircleV(bullet.position, bullet.radius + 7.0F,
                    Fade(GOLD, 0.22F));
        DrawCircleV(bullet.position, bullet.radius, Color{255, 231, 127, 255});
        DrawCircleV(bullet.position, bullet.radius * 0.42F, RAYWHITE);
    }

    boss_.Draw(uiFont_);
    player_.Draw(characterArt_);
    player_.DrawHud(uiFont_, characterArt_, mainMenu_.SelectedOperatorName());
    boss_.DrawHud(uiFont_);

    const bool showResult = boss_.IsDefeated() ||
                            (player_.IsDead() &&
                             player_.DefeatAnimationFinished());
    if (showResult) {
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

    if (paused_) {
        DrawPauseMenu();
    }
}
