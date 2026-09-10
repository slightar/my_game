#include "game.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kGateCloseDuration = 0.72F;
constexpr float kEncounterBannerDuration = 2.4F;
constexpr float kCameraFollowSharpness = 6.5F;

float SmoothStep(float value) {
    const float t = std::clamp(value, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

}  // namespace

Game::Game() {
    Reset();
}

void Game::Reset() {
    bullets_.clear();
    player_.Reset(mainMenu_.SelectedOperator());
    boss_.Reset();
    bossActive_ = false;
    cameraX_ = static_cast<float>(GameConfig::kScreenWidth) / 2.0F;
    gateCloseTimer_ = 0.0F;
    encounterBannerTimer_ = 0.0F;
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

    if (player_.IsDead() || (bossActive_ && boss_.IsDefeated())) {
        if (player_.IsDead()) {
            player_.UpdateDefeatAnimation(deltaTime);
        }
        if (bossActive_ && boss_.IsDefeated()) {
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

    if (!bossActive_ && player_.Position().x >= GameConfig::kBossTriggerX) {
        bossActive_ = true;
        gateCloseTimer_ = kGateCloseDuration;
        encounterBannerTimer_ = kEncounterBannerDuration;
        bullets_.clear();
    }
    if (bossActive_) {
        const float sealedLeft = GameConfig::kBossGateX +
                                 GameConfig::kBossGateWidth / 2.0F + 34.0F;
        const float worldRight = GameConfig::kRoom.x + GameConfig::kRoom.width -
                                 75.0F;
        player_.SetHorizontalBounds(sealedLeft, worldRight);
        gateCloseTimer_ = std::max(0.0F, gateCloseTimer_ - deltaTime);
        encounterBannerTimer_ =
            std::max(0.0F, encounterBannerTimer_ - deltaTime);
        boss_.Update(deltaTime, player_.Position(), player_.FacingDirection());
    }
    UpdateBullets(deltaTime);
    UpdateCamera(deltaTime);

    const bool touchingBoss = bossActive_ && boss_.CanDealContactDamage() &&
                              CheckCollisionCircleRec(boss_.Position(), boss_.Radius(),
                                                      player_.Hitbox());
    const bool hitByBossAttack = bossActive_ && !boss_.IsDefeated() &&
                                 boss_.AttackHits(player_.Hitbox(),
                                                  player_.ProjectileHitbox());
    if (touchingBoss || hitByBossAttack) {
        if (player_.TakeDamage(boss_.Position())) {
            audio_.PlayPlayerHit();
        }
    }
}

void Game::UpdateCamera(float deltaTime) {
    const float halfView = static_cast<float>(GameConfig::kScreenWidth) / 2.0F;
    const float minCameraX = halfView;
    const float maxCameraX = GameConfig::kRoom.x + GameConfig::kRoom.width -
                             halfView;
    const float desiredX = std::clamp(player_.Position().x,
                                      minCameraX, maxCameraX);
    const float blend = 1.0F - std::exp(-kCameraFollowSharpness * deltaTime);
    cameraX_ += (desiredX - cameraX_) * blend;
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

        if (!bullet.hasHit && bossActive_ && !boss_.IsDefeated() &&
            CheckCollisionCircles(bullet.position, bullet.radius,
                                  boss_.Position(), boss_.Radius())) {
            boss_.TakeDamage(bullet.damage);
            if (bullet.stunDuration > 0.0F) {
                boss_.Stun(bullet.stunDuration);
            }
            audio_.PlayBossHit();
            bullet.hasHit = true;
            if (bullet.kind != BulletKind::MeleeSlash) {
                bullet.lifetime = 0.0F;
            }
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

void Game::DrawMap() const {
    const Rectangle room = GameConfig::kRoom;
    DrawRectangleRec(room, Color{205, 211, 216, 255});

    // A cool, clean safe room separated from the combat space by a heavy gate.
    DrawRectangleRec(GameConfig::kSafeRoom, Color{188, 207, 211, 255});
    DrawRectangle(static_cast<int>(GameConfig::kSafeRoom.x),
                  static_cast<int>(GameConfig::kSafeRoom.y),
                  static_cast<int>(GameConfig::kSafeRoom.width), 18,
                  Color{63, 85, 91, 255});
    for (int panel = 0; panel < 4; ++panel) {
        const float panelX = GameConfig::kSafeRoom.x + 42.0F + panel * 164.0F;
        DrawRectangleRec({panelX, 101.0F, 118.0F, 12.0F},
                         Color{159, 238, 232, 255});
        DrawRectangleRec({panelX + 5.0F, 105.0F, 108.0F, 30.0F},
                         Fade(Color{124, 232, 224, 255}, 0.12F));
    }
    DrawRectangleRec({108.0F, 210.0F, 236.0F, 122.0F},
                     Color{104, 124, 130, 255});
    DrawRectangleLinesEx({108.0F, 210.0F, 236.0F, 122.0F}, 5.0F,
                         Color{52, 68, 73, 255});
    DrawRectangleRec({131.0F, 237.0F, 190.0F, 10.0F},
                     Color{130, 222, 211, 255});
    uiFont_.Draw("安全室 // SAFE", 137.0F, 274.0F, 22.0F,
                 Color{221, 244, 241, 255});

    DrawRectangleRec(GameConfig::kBossArena, Color{183, 187, 193, 255});
    for (int panel = 0; panel < 8; ++panel) {
        const float x = GameConfig::kBossArena.x + panel * 194.0F;
        DrawRectangleLinesEx({x, GameConfig::kBossArena.y, 194.0F,
                              GameConfig::kBossArena.height},
                             2.0F, Fade(Color{75, 80, 88, 255}, 0.22F));
    }
    DrawRectangleRec({GameConfig::kBossArena.x + 76.0F, 116.0F,
                      354.0F, 9.0F}, Color{178, 34, 43, 255});
    uiFont_.Draw("高危作战区域", GameConfig::kBossArena.x + 91.0F,
                 139.0F, 21.0F, Color{117, 31, 37, 255});

    DrawRectangleLinesEx(room, 5.0F, Color{69, 75, 84, 255});
    DrawRectangle(static_cast<int>(room.x),
                  static_cast<int>(GameConfig::kFloorY),
                  static_cast<int>(room.width),
                  static_cast<int>(room.y + room.height - GameConfig::kFloorY),
                  Color{97, 103, 113, 255});
    for (int marker = 0; marker < 24; ++marker) {
        const float x = room.x + 26.0F + marker * 101.0F;
        DrawRectangleRec({x, GameConfig::kFloorY + 4.0F, 52.0F, 5.0F},
                         marker < 8 ? Color{111, 187, 180, 255}
                                    : Color{139, 58, 62, 255});
    }

    constexpr float doorTop = 185.0F;
    const float gateLeft = GameConfig::kBossGateX -
                           GameConfig::kBossGateWidth / 2.0F;
    DrawRectangleRec({gateLeft - 17.0F, room.y, 17.0F,
                      GameConfig::kFloorY - room.y},
                     Color{55, 61, 69, 255});
    DrawRectangleRec({gateLeft + GameConfig::kBossGateWidth, room.y, 17.0F,
                      GameConfig::kFloorY - room.y},
                     Color{55, 61, 69, 255});
    DrawRectangleRec({gateLeft - 17.0F, doorTop - 22.0F,
                      GameConfig::kBossGateWidth + 34.0F, 22.0F},
                     Color{45, 50, 58, 255});

    if (bossActive_) {
        const float closeProgress = SmoothStep(
            1.0F - gateCloseTimer_ / kGateCloseDuration);
        const float gateHeight = (GameConfig::kFloorY - doorTop) * closeProgress;
        DrawRectangleRec({gateLeft, doorTop, GameConfig::kBossGateWidth,
                          gateHeight}, Color{58, 63, 71, 255});
        for (float y = doorTop + 12.0F; y < doorTop + gateHeight; y += 28.0F) {
            DrawRectangleRec({gateLeft + 3.0F, y,
                              GameConfig::kBossGateWidth - 6.0F, 5.0F},
                             Color{111, 117, 126, 255});
        }
        DrawRectangleRec({gateLeft, doorTop + gateHeight - 8.0F,
                          GameConfig::kBossGateWidth, 8.0F},
                         Color{207, 39, 47, 255});
    } else {
        uiFont_.Draw("→", gateLeft - 2.0F, 238.0F, 34.0F,
                     Color{175, 231, 226, 255});
        uiFont_.Draw("进入作战区", GameConfig::kBossGateX - 74.0F,
                     287.0F, 18.0F, Color{77, 91, 98, 255});
    }
}

void Game::DrawEncounterBanner() const {
    if (encounterBannerTimer_ <= 0.0F) {
        return;
    }
    const float elapsed = kEncounterBannerDuration - encounterBannerTimer_;
    const float fadeIn = std::clamp(elapsed / 0.25F, 0.0F, 1.0F);
    const float fadeOut = std::clamp(encounterBannerTimer_ / 0.45F, 0.0F, 1.0F);
    const float alpha = fadeIn * fadeOut;
    DrawRectangleRec({0.0F, 286.0F,
                      static_cast<float>(GameConfig::kScreenWidth), 112.0F},
                     Fade(BLACK, alpha * 0.68F));
    const char* title = "目标出现：弑君者";
    const float titleWidth = uiFont_.Measure(title, 38.0F);
    uiFont_.Draw(title,
                 static_cast<float>(GameConfig::kScreenWidth) / 2.0F -
                     titleWidth / 2.0F,
                 304.0F, 38.0F, Fade(RAYWHITE, alpha));
    const char* warning = "后门封锁 // 作战开始";
    const float warningWidth = uiFont_.Measure(warning, 19.0F);
    uiFont_.Draw(warning,
                 static_cast<float>(GameConfig::kScreenWidth) / 2.0F -
                     warningWidth / 2.0F,
                 356.0F, 19.0F,
                 Fade(Color{239, 57, 64, 255}, alpha));
}

void Game::Draw() const {
    if (!inBattle_) {
        mainMenu_.Draw(uiFont_, characterArt_);
        return;
    }

    ClearBackground({24, 27, 34, 255});
    const Camera2D worldCamera{
        {static_cast<float>(GameConfig::kScreenWidth) / 2.0F,
         static_cast<float>(GameConfig::kScreenHeight) / 2.0F},
        {cameraX_, static_cast<float>(GameConfig::kScreenHeight) / 2.0F},
        0.0F, 1.0F};
    BeginMode2D(worldCamera);
    DrawMap();

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
    EndMode2D();

    player_.DrawHud(uiFont_, characterArt_, mainMenu_.SelectedOperatorName());
    if (bossActive_) {
        boss_.DrawHud(uiFont_);
    }
    DrawEncounterBanner();

    const bool showResult = (bossActive_ && boss_.IsDefeated()) ||
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
