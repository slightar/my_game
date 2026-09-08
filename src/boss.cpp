#include "boss.h"

#include "game_types.h"
#include "ui_font.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

constexpr float kIdleDuration = 0.38F;
constexpr float kEnragedIdleDuration = 0.2F;
constexpr float kMeleeRange = 155.0F;
constexpr float kMeleeWindupDuration = 0.22F;
constexpr float kMeleeDuration = 0.18F;
constexpr float kMeleeReach = 64.0F;
constexpr float kRushWindupDuration = 0.34F;
constexpr float kRushDuration = 0.82F;
constexpr float kRushSpeed = 1040.0F;
constexpr float kAirWindupDuration = 0.44F;
constexpr float kAirSlashDuration = 0.28F;
constexpr float kDartWindupDuration = 0.42F;
constexpr float kDartSpeed = 720.0F;
constexpr float kDartRadius = 10.0F;
constexpr float kRecoverDuration = 0.28F;
constexpr float kEnragedRecoverDuration = 0.16F;
constexpr std::array<float, 3> kDartAngles{-0.24F, 0.0F, 0.24F};

Vector2 Lerp(Vector2 start, Vector2 end, float amount) {
    return {start.x + (end.x - start.x) * amount,
            start.y + (end.y - start.y) * amount};
}

}  // namespace

void Boss::Reset() {
    position_ = {1030.0F, GameConfig::kFloorY - kRadius};
    slashStart_ = position_;
    slashTarget_ = position_;
    state_ = State::Idle;
    stateTimer_ = kIdleDuration;
    facingDirection_ = -1;
    health_ = kMaxHealth;
    nextRangedAttack_ = 0;
    darts_.clear();
}

void Boss::Update(float deltaTime, Vector2 playerPosition) {
    if (state_ == State::Defeated) {
        return;
    }

    UpdateDarts(deltaTime);
    stateTimer_ -= deltaTime;
    switch (state_) {
        case State::Idle:
            facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
            if (stateTimer_ <= 0.0F) {
                if (std::abs(playerPosition.x - position_.x) <= kMeleeRange) {
                    StartMelee(playerPosition);
                } else {
                    switch (nextRangedAttack_) {
                        case 0:
                            StartRush(playerPosition);
                            break;
                        case 1:
                            StartAirSlash(playerPosition);
                            break;
                        default:
                            StartDartVolley(playerPosition);
                            break;
                    }
                    nextRangedAttack_ = (nextRangedAttack_ + 1) % 3;
                }
            }
            break;
        case State::MeleeWindup:
            facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
            if (stateTimer_ <= 0.0F) {
                state_ = State::Melee;
                stateTimer_ = kMeleeDuration;
            }
            break;
        case State::Melee:
            position_.x += static_cast<float>(facingDirection_) * 170.0F * deltaTime;
            if (stateTimer_ <= 0.0F) {
                state_ = State::Recover;
                stateTimer_ = health_ <= kMaxHealth / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        case State::RushWindup:
            if (stateTimer_ <= 0.0F) {
                state_ = State::Rush;
                stateTimer_ = kRushDuration;
            }
            break;
        case State::Rush: {
            position_.x += static_cast<float>(facingDirection_) * kRushSpeed * deltaTime;
            const float left = GameConfig::kRoom.x + 28.0F + kRadius;
            const float right = GameConfig::kRoom.x + GameConfig::kRoom.width - 28.0F - kRadius;
            const float clampedX = std::clamp(position_.x, left, right);
            const bool hitWall = clampedX != position_.x;
            position_.x = clampedX;
            if (stateTimer_ <= 0.0F || hitWall) {
                state_ = State::Recover;
                stateTimer_ = health_ <= kMaxHealth / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        }
        case State::AirSlashWindup:
            if (stateTimer_ <= 0.0F) {
                state_ = State::AirSlash;
                stateTimer_ = kAirSlashDuration;
            }
            break;
        case State::AirSlash: {
            const float progress = std::clamp(1.0F - stateTimer_ / kAirSlashDuration,
                                              0.0F, 1.0F);
            position_ = Lerp(slashStart_, slashTarget_, progress * progress);
            if (stateTimer_ <= 0.0F) {
                position_ = slashTarget_;
                state_ = State::Recover;
                stateTimer_ = health_ <= kMaxHealth / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        }
        case State::DartWindup:
            facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
            if (stateTimer_ <= 0.0F) {
                LaunchDarts();
                state_ = State::Recover;
                stateTimer_ = health_ <= kMaxHealth / 2
                                  ? kEnragedRecoverDuration
                                  : kRecoverDuration;
            }
            break;
        case State::Recover:
            if (stateTimer_ <= 0.0F) {
                state_ = State::Idle;
                stateTimer_ = health_ <= kMaxHealth / 2
                                  ? kEnragedIdleDuration
                                  : kIdleDuration;
            }
            break;
        case State::Defeated:
            break;
    }
}

void Boss::StartMelee(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    position_.y = GameConfig::kFloorY - kRadius;
    state_ = State::MeleeWindup;
    stateTimer_ = kMeleeWindupDuration;
}

void Boss::StartRush(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    position_.y = GameConfig::kFloorY - kRadius;
    state_ = State::RushWindup;
    stateTimer_ = kRushWindupDuration;
}

void Boss::StartAirSlash(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    const float left = GameConfig::kRoom.x + 80.0F;
    const float right = GameConfig::kRoom.x + GameConfig::kRoom.width - 80.0F;
    slashStart_ = {
        std::clamp(playerPosition.x - static_cast<float>(facingDirection_) * 260.0F,
                   left, right),
        GameConfig::kRoom.y + 120.0F};
    slashTarget_ = {std::clamp(playerPosition.x, left, right),
                    GameConfig::kFloorY - kRadius};
    position_ = slashStart_;
    state_ = State::AirSlashWindup;
    stateTimer_ = kAirWindupDuration;
}

void Boss::StartDartVolley(Vector2 playerPosition) {
    facingDirection_ = playerPosition.x >= position_.x ? 1 : -1;
    position_.y = GameConfig::kFloorY - kRadius;
    state_ = State::DartWindup;
    stateTimer_ = kDartWindupDuration;
}

void Boss::LaunchDarts() {
    const Vector2 origin{
        position_.x + static_cast<float>(facingDirection_) * 34.0F,
        position_.y - 24.0F};
    for (const float angle : kDartAngles) {
        darts_.push_back({
            origin,
            {static_cast<float>(facingDirection_) * std::cos(angle) * kDartSpeed,
             std::sin(angle) * kDartSpeed},
            angle});
    }
}

void Boss::UpdateDarts(float deltaTime) {
    for (Dart& dart : darts_) {
        dart.position.x += dart.velocity.x * deltaTime;
        dart.position.y += dart.velocity.y * deltaTime;
        dart.rotation += 12.0F * deltaTime;
    }

    std::erase_if(darts_, [](const Dart& dart) {
        const Rectangle room = GameConfig::kRoom;
        return dart.position.x < room.x ||
               dart.position.x > room.x + room.width ||
               dart.position.y < room.y ||
               dart.position.y > room.y + room.height;
    });
}

void Boss::DrawDarts() const {
    for (const Dart& dart : darts_) {
        const Vector2 blade{
            std::cos(dart.rotation) * 13.0F,
            std::sin(dart.rotation) * 13.0F};
        const Vector2 cross{-blade.y * 0.72F, blade.x * 0.72F};
        DrawCircleV(dart.position, kDartRadius + 3.0F, Fade(RED, 0.22F));
        DrawLineEx({dart.position.x - blade.x, dart.position.y - blade.y},
                   {dart.position.x + blade.x, dart.position.y + blade.y},
                   5.0F, LIGHTGRAY);
        DrawLineEx({dart.position.x - cross.x, dart.position.y - cross.y},
                   {dart.position.x + cross.x, dart.position.y + cross.y},
                   4.0F, MAROON);
    }
}

void Boss::Draw(const UiFont& font) const {
    DrawDarts();

    if (state_ == State::Defeated) {
        DrawCircleV(position_, kRadius, Fade(DARKPURPLE, 0.28F));
        return;
    }

    if (state_ == State::MeleeWindup) {
        const float direction = static_cast<float>(facingDirection_);
        DrawLineEx({position_.x + direction * 22.0F, position_.y - 6.0F},
                   {position_.x + direction * 96.0F, position_.y - 6.0F},
                   10.0F, Fade(ORANGE, 0.55F));
        font.Draw("挥砍", position_.x - 24.0F, position_.y - 72.0F, 20.0F, ORANGE);
    }
    if (state_ == State::Melee) {
        const float direction = static_cast<float>(facingDirection_);
        DrawLineEx({position_.x + direction * 12.0F, position_.y - 22.0F},
                   {position_.x + direction * 102.0F, position_.y + 24.0F},
                   18.0F, Fade(RED, 0.72F));
    }

    if (state_ == State::RushWindup) {
        const float endX = facingDirection_ > 0
                               ? GameConfig::kRoom.x + GameConfig::kRoom.width - 30.0F
                               : GameConfig::kRoom.x + 30.0F;
        DrawLineEx({position_.x, GameConfig::kFloorY - 6.0F},
                   {endX, GameConfig::kFloorY - 6.0F}, 8.0F, Fade(RED, 0.55F));
        font.Draw("突袭", position_.x - 24.0F, position_.y - 72.0F, 20.0F, RED);
    }
    if (state_ == State::AirSlashWindup) {
        DrawLineEx(slashStart_, slashTarget_, 9.0F, Fade(RED, 0.55F));
        DrawCircleV(slashTarget_, 18.0F, Fade(RED, 0.35F));
        font.Draw("空中斩", position_.x - 28.0F, position_.y - 62.0F, 18.0F, RED);
    }
    if (state_ == State::DartWindup) {
        const Vector2 origin{
            position_.x + static_cast<float>(facingDirection_) * 30.0F,
            position_.y - 24.0F};
        for (const float angle : kDartAngles) {
            const Vector2 target{
                origin.x + static_cast<float>(facingDirection_) *
                               std::cos(angle) * 190.0F,
                origin.y + std::sin(angle) * 190.0F};
            DrawLineEx(origin, target, 4.0F, Fade(ORANGE, 0.48F));
        }
        font.Draw("飞镖", position_.x - 24.0F, position_.y - 72.0F,
                  20.0F, ORANGE);
    }
    if (state_ == State::Melee || state_ == State::Rush ||
        state_ == State::AirSlash) {
        DrawCircleV({position_.x - static_cast<float>(facingDirection_) * 28.0F,
                     position_.y - 4.0F},
                    kRadius - 6.0F, Fade(PURPLE, 0.24F));
    }

    DrawCircleV(position_, kRadius + 5.0F, BLACK);
    DrawCircleV(position_, kRadius, DARKPURPLE);
    DrawRectangle(static_cast<int>(position_.x) - 21,
                  static_cast<int>(position_.y) - 12, 42, 12, BLACK);
    const float direction = static_cast<float>(facingDirection_);
    DrawLineEx({position_.x + direction * 18.0F, position_.y + 5.0F},
               {position_.x + direction * 62.0F, position_.y + 30.0F},
               7.0F, LIGHTGRAY);
    font.Draw("弑君者", position_.x - 25.0F, position_.y - 50.0F, 14.0F, MAROON);
}

void Boss::DrawHud(const UiFont& font) const {
    constexpr int width = 540;
    constexpr int height = 22;
    const int x = GameConfig::kScreenWidth / 2 - width / 2;
    const int y = GameConfig::kScreenHeight - 44;
    const float ratio = static_cast<float>(health_) / static_cast<float>(kMaxHealth);
    DrawRectangle(x, y, width, height, Color{55, 58, 67, 255});
    DrawRectangle(x, y, static_cast<int>(static_cast<float>(width) * ratio), height, MAROON);
    DrawRectangleLines(x, y, width, height, RAYWHITE);
    font.Draw("弑君者", static_cast<float>(x), static_cast<float>(y - 22),
              18.0F, RAYWHITE);
}

void Boss::TakeDamage(int damage) {
    if (state_ == State::Defeated) {
        return;
    }
    health_ = std::max(0, health_ - damage);
    if (health_ == 0) {
        state_ = State::Defeated;
        darts_.clear();
    }
}

Vector2 Boss::Position() const { return position_; }
float Boss::Radius() const { return kRadius; }
bool Boss::AttackHits(Vector2 playerPosition, float playerRadius) {
    const auto dartHit = std::find_if(
        darts_.begin(), darts_.end(),
        [playerPosition, playerRadius](const Dart& dart) {
            return CheckCollisionCircles(dart.position, kDartRadius,
                                         playerPosition, playerRadius);
        });
    if (dartHit != darts_.end()) {
        darts_.erase(dartHit);
        return true;
    }

    if (state_ == State::Melee) {
        const Vector2 hitboxCenter{
            position_.x + static_cast<float>(facingDirection_) * kMeleeReach,
            position_.y - 2.0F};
        return CheckCollisionCircles(hitboxCenter, 48.0F,
                                     playerPosition, playerRadius);
    }

    return false;
}
bool Boss::IsDefeated() const { return state_ == State::Defeated; }
