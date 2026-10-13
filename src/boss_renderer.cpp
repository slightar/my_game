#include "boss.h"
#include "boss_client_fx.h"
#include "game_types.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "file_path.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr int kSpriteColumns=10, kSpriteRows=6;
constexpr float kSpriteHeight=120.0F;
}
BossRenderer::BossRenderer():effects_(std::make_unique<BossClientFx>()) {
    const auto path=(Utf8Path(GetApplicationDirectory())/"assets/enemies/crownslayer_battle.png").string();
    if(FileExists(path.c_str())) {
        sprite_=LoadTexture(path.c_str());
        if(IsTextureValid(sprite_))SetTextureFilter(sprite_,TEXTURE_FILTER_BILINEAR);
    }
}
BossRenderer::~BossRenderer(){if(IsTextureValid(sprite_))UnloadTexture(sprite_);}

void BossRenderer::Draw(const Boss& boss,const UiFont& font) const {boss.Draw(font,sprite_,effects_.get());}
bool BossRenderer::EffectsReady() const {return effects_&&effects_->ReadyFor(StoryBossKind::Crownslayer);}

void Boss::DrawBattleSprite(Texture2D battleSprite_) const {
    if (!IsTextureValid(battleSprite_)) {
        return;
    }

    int row = 0;
    bool loops = true;
    float framesPerSecond = 10.0F;
    float clipTime=animationTime_;
    switch (state_) {
        case State::Rush:
            row = 1;  // Move
            break;
        case State::MeleeWindup:
        case State::Melee:
        case State::DartWindup:
            row = 2;  // Attack
            framesPerSecond = 24.0F;
            loops = false;
            break;
        case State::AirSlash:
        case State::ShadowSlash:
        case State::ShadowFinal:
            row = 2;  // Attack
            framesPerSecond = 34.0F;
            loops = false;
            break;
        case State::RushTeleportOut:
            row = 3;  // Disappear
            framesPerSecond = 20.0F;
            loops = false;
            break;
        case State::RushTeleportIn:
        case State::AirSlashWindup:
        case State::ShadowFinalWindup:
            row = 4;  // Appear
            framesPerSecond = 20.0F;
            loops = false;
            break;
        case State::RushWindup:
            row = 2;  // Attack preparation
            framesPerSecond = 24.0F;
            loops = false;
            break;
        case State::ShadowWindup:
        case State::ShadowPause:
            // Finish appearing, then hold the ready pose until the actual slash.
            row = clipTime<.16F?4:2;
            if(clipTime>=.16F)clipTime=2.0F/20;
            framesPerSecond = 20.0F;
            loops = false;
            break;
        case State::FanRetreat:
            if(animationTime_<.45F)row=1;
            else {row=2;framesPerSecond=24;loops=false;clipTime=animationTime_-.45F;}
            break;
        case State::Defeated:
            row = 5;  // Die
            loops = false;
            break;
        case State::Idle:
        case State::Recover:
        default:
            row = 0;  // Idle
            break;
    }

    int frame = static_cast<int>(clipTime * framesPerSecond);
    frame = loops ? frame % kSpriteColumns
                  : std::min(frame, kSpriteColumns - 1);
    const float frameWidth = static_cast<float>(battleSprite_.width) /
                             static_cast<float>(kSpriteColumns);
    const float frameHeight = static_cast<float>(battleSprite_.height) /
                              static_cast<float>(kSpriteRows);
    const float spriteWidth = kSpriteHeight * frameWidth / frameHeight;
    const Rectangle source{frameWidth * static_cast<float>(frame),
                           frameHeight * static_cast<float>(row),
                           facingDirection_ < 0 ? -frameWidth : frameWidth,
                           frameHeight};
    const Rectangle destination{
        position_.x,
        position_.y + kRadius + kSpriteHeight * 0.13F,
        spriteWidth,
        kSpriteHeight};
    DrawTexturePro(battleSprite_, source, destination,
                   {spriteWidth / 2.0F, kSpriteHeight}, 0.0F,
                   state_ == State::Defeated
                       ? Fade(WHITE, 0.88F)
                       : (stunTimer_ > 0.0F ? Color{170, 215, 255, 255}
                                            : WHITE));
}

void Boss::Draw(const UiFont& font, Texture2D sprite,const BossClientFx* effects) const {
    if(!IsTextureValid(sprite)) {
        font.Draw("Boss 素材缺失",position_.x-50,position_.y-80,18,RED);
        return;
    }
    if(effects)effects->DrawCrown(*this,font,sprite);

    if (state_ == State::Defeated) {
        DrawBattleSprite(sprite);
        return;
    }

    DrawBattleSprite(sprite);
    font.Draw("弑君者", position_.x - 25.0F, position_.y - 50.0F, 14.0F, MAROON);
}

void Boss::DrawHud(const UiFont& font) const {
    constexpr float width = 520.0F;
    constexpr float height = 54.0F;
    const float x = static_cast<float>(GameConfig::kScreenWidth) - width - 32.0F;
    constexpr float y = 28.0F;
    const float ratio = health_ / maxHealth_;
    TacticalUi::DrawCutPanel({x, y, width, height}, TacticalUi::kPanel,
                             Fade(RAYWHITE, 0.20F), 13.0F);
    font.Draw("BOSS // 弑君者", x + 18.0F, y + 8.0F, 15.0F,
              TacticalUi::kPaper);
    font.Draw(TextFormat("%03i%%", static_cast<int>(ratio * 100.0F)),
              x + width - 66.0F, y + 8.0F, 13.0F, TacticalUi::kMuted);
    TacticalUi::DrawProgressLine({x + 18.0F, y + 39.0F}, width - 40.0F,
                                 ratio, TacticalUi::kRed, 7.0F);
}

