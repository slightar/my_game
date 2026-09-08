#include "character_art.h"

#include <algorithm>
#include <filesystem>
#include <string>

namespace {

std::string AssetPath(const char* relativePath) {
    return (std::filesystem::path(GetApplicationDirectory()) / relativePath).string();
}

constexpr int kChibiColumns = 8;
constexpr int kChibiRows = 11;
constexpr int kBattleColumns = 12;
constexpr int kBattleRows = 2;

struct AnimationStrip {
    int row;
    int frameCount;
    float framesPerSecond;
    bool hasDirectionalRows;
};

AnimationStrip StripFor(ChibiAnimation animation) {
    switch (animation) {
        case ChibiAnimation::Run:
            return {1, 8, 12.0F, true};
        case ChibiAnimation::Jump:
            return {4, 5, 10.0F, false};
        case ChibiAnimation::Dodge:
            return {1, 8, 20.0F, true};
        case ChibiAnimation::Wave:
            return {3, 4, 7.0F, false};
        case ChibiAnimation::Idle:
        default:
            return {0, 7, 6.0F, false};
    }
}

}  // namespace

CharacterArt::CharacterArt() {
    const std::string portraitPath =
        AssetPath("assets/operators/exusiai_portrait.png");
    if (FileExists(portraitPath.c_str())) {
        portrait_ = LoadTexture(portraitPath.c_str());
        SetTextureFilter(portrait_, TEXTURE_FILTER_BILINEAR);
    }

    const std::string chibiPath = AssetPath("assets/operators/exusiai_chibi.png");
    if (FileExists(chibiPath.c_str())) {
        chibi_ = LoadTexture(chibiPath.c_str());
        SetTextureFilter(chibi_, TEXTURE_FILTER_BILINEAR);
    }

    const std::string battlePath =
        AssetPath("assets/operators/exusiai_battle.png");
    if (FileExists(battlePath.c_str())) {
        battleChibi_ = LoadTexture(battlePath.c_str());
        SetTextureFilter(battleChibi_, TEXTURE_FILTER_BILINEAR);
    }
}

CharacterArt::~CharacterArt() {
    if (IsTextureValid(battleChibi_)) {
        UnloadTexture(battleChibi_);
    }
    if (IsTextureValid(chibi_)) {
        UnloadTexture(chibi_);
    }
    if (IsTextureValid(portrait_)) {
        UnloadTexture(portrait_);
    }
}

bool CharacterArt::HasPortrait() const {
    return IsTextureValid(portrait_);
}

bool CharacterArt::HasChibi() const {
    return IsTextureValid(chibi_);
}

bool CharacterArt::HasBattleChibi() const {
    return IsTextureValid(battleChibi_);
}

void CharacterArt::DrawPortrait(Rectangle destination, Color tint) const {
    if (!HasPortrait()) {
        return;
    }
    const float sourceWidth = static_cast<float>(portrait_.width);
    const float sourceHeight = static_cast<float>(portrait_.height);
    const float scale = std::min(destination.width / sourceWidth,
                                 destination.height / sourceHeight);
    const Rectangle fitted{
        destination.x + (destination.width - sourceWidth * scale) / 2.0F,
        destination.y + destination.height - sourceHeight * scale,
        sourceWidth * scale,
        sourceHeight * scale};
    DrawTexturePro(portrait_, {0.0F, 0.0F, sourceWidth, sourceHeight},
                   fitted, {}, 0.0F, tint);
}

void CharacterArt::DrawChibi(Vector2 feetPosition, int facingDirection,
                             float height, ChibiAnimation animation,
                             float animationTime, Color tint) const {
    if (!HasChibi()) {
        return;
    }

    const float frameWidth = static_cast<float>(chibi_.width) /
                             static_cast<float>(kChibiColumns);
    const float frameHeight = static_cast<float>(chibi_.height) /
                              static_cast<float>(kChibiRows);
    const AnimationStrip strip = StripFor(animation);
    const int frame = static_cast<int>(animationTime * strip.framesPerSecond) %
                      strip.frameCount;
    int row = strip.row;
    bool flip = facingDirection < 0;
    if (strip.hasDirectionalRows) {
        row = facingDirection >= 0 ? strip.row : strip.row + 1;
        flip = false;
    }

    const float width = height * frameWidth / frameHeight;
    const Rectangle source{frameWidth * static_cast<float>(frame),
                           frameHeight * static_cast<float>(row),
                           flip ? -frameWidth : frameWidth,
                           frameHeight};
    const Rectangle destination{feetPosition.x, feetPosition.y, width, height};
    DrawTexturePro(chibi_, source, destination,
                   {width / 2.0F, height}, 0.0F, tint);
}

void CharacterArt::DrawBattleChibi(Vector2 feetPosition, int facingDirection,
                                   float height, BattleChibiAnimation animation,
                                   float animationTime, Color tint) const {
    if (!HasBattleChibi()) {
        return;
    }

    const float frameWidth = static_cast<float>(battleChibi_.width) /
                             static_cast<float>(kBattleColumns);
    const float frameHeight = static_cast<float>(battleChibi_.height) /
                              static_cast<float>(kBattleRows);
    int row = 0;
    int frame = static_cast<int>(animationTime * 3.0F) % kBattleColumns;
    if (animation == BattleChibiAnimation::Attack) {
        row = 1;
        constexpr int kRaisedGunFirstFrame = 3;
        constexpr int kRaisedGunFrameCount = 6;
        frame = kRaisedGunFirstFrame +
                static_cast<int>(animationTime * 18.0F) % kRaisedGunFrameCount;
    }

    const float width = height * frameWidth / frameHeight;
    const Rectangle source{frameWidth * static_cast<float>(frame),
                           frameHeight * static_cast<float>(row),
                           facingDirection < 0 ? -frameWidth : frameWidth,
                           frameHeight};
    const Rectangle destination{feetPosition.x, feetPosition.y, width, height};
    DrawTexturePro(battleChibi_, source, destination,
                   {width / 2.0F, height}, 0.0F, tint);
}
