#include "character_art.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace {

std::string AssetPath(const char* relativePath) {
    return (std::filesystem::path(GetApplicationDirectory()) / relativePath).string();
}

constexpr int kChibiColumns = 8;
constexpr int kChibiRows = 11;
constexpr int kBattleColumns = 12;
constexpr int kBattleRows = 3;
constexpr int kTexasSkill2Columns = 12;
constexpr int kTexasSkill2Rows = 3;

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

float SmoothStep(float value) {
    const float t = std::clamp(value, 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

float PulseWindow(float progress, float start, float end) {
    if (progress <= start || progress >= end) {
        return 0.0F;
    }
    const float local = (progress - start) / (end - start);
    return std::sin(local * PI);
}

float Hash01(int value) {
    unsigned int bits = static_cast<unsigned int>(value) * 747796405U +
                        2891336453U;
    bits = ((bits >> ((bits >> 28U) + 4U)) ^ bits) * 277803737U;
    bits = (bits >> 22U) ^ bits;
    return static_cast<float>(bits & 0xffffU) / 65535.0F;
}

Vector2 PolarPoint(Vector2 center, float angle, float radius) {
    return {center.x + std::cos(angle) * radius,
            center.y + std::sin(angle) * radius};
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

    const char* skillPaths[2] = {
        "assets/operators/exusiai_skill_2.png",
        "assets/operators/exusiai_skill_3.png"};
    for (int index = 0; index < 2; ++index) {
        const std::string skillPath = AssetPath(skillPaths[index]);
        if (FileExists(skillPath.c_str())) {
            skillIcons_[index] = LoadTexture(skillPath.c_str());
            SetTextureFilter(skillIcons_[index], TEXTURE_FILTER_BILINEAR);
        }
    }

    const std::string texasPortraitPath =
        AssetPath("assets/operators/texas_portrait.png");
    if (FileExists(texasPortraitPath.c_str())) {
        texasPortrait_ = LoadTexture(texasPortraitPath.c_str());
        SetTextureFilter(texasPortrait_, TEXTURE_FILTER_BILINEAR);
    }
    const std::string texasChibiPath =
        AssetPath("assets/operators/texas_chibi.png");
    if (FileExists(texasChibiPath.c_str())) {
        texasChibi_ = LoadTexture(texasChibiPath.c_str());
        SetTextureFilter(texasChibi_, TEXTURE_FILTER_BILINEAR);
    }
    const std::string texasSkill2BattlePath =
        AssetPath("assets/operators/texas_skill2_battle.png");
    if (FileExists(texasSkill2BattlePath.c_str())) {
        texasSkill2Battle_ = LoadTexture(texasSkill2BattlePath.c_str());
        SetTextureFilter(texasSkill2Battle_, TEXTURE_FILTER_BILINEAR);
    }
    const char* texasEffectPaths[4] = {
        "assets/operators/texas_skill2_aura.png",
        "assets/operators/texas_skill2_slash_a.png",
        "assets/operators/texas_skill2_slash_b.png",
        "assets/operators/texas_skill2_burst.png"};
    Texture2D* texasEffectTextures[4] = {
        &texasSkill2Aura_, &texasSkill2Slashes_[0],
        &texasSkill2Slashes_[1], &texasSkill2Burst_};
    for (int index = 0; index < 4; ++index) {
        const std::string effectPath = AssetPath(texasEffectPaths[index]);
        if (FileExists(effectPath.c_str())) {
            *texasEffectTextures[index] = LoadTexture(effectPath.c_str());
            SetTextureFilter(*texasEffectTextures[index],
                             TEXTURE_FILTER_BILINEAR);
        }
    }
    const char* texasSkillPaths[2] = {
        "assets/operators/texas_skill_e.png",
        "assets/operators/texas_skill_q.png"};
    for (int index = 0; index < 2; ++index) {
        const std::string skillPath = AssetPath(texasSkillPaths[index]);
        if (FileExists(skillPath.c_str())) {
            texasSkillIcons_[index] = LoadTexture(skillPath.c_str());
            SetTextureFilter(texasSkillIcons_[index], TEXTURE_FILTER_BILINEAR);
        }
    }
}

CharacterArt::~CharacterArt() {
    if (IsTextureValid(texasSkill2Burst_)) {
        UnloadTexture(texasSkill2Burst_);
    }
    for (Texture2D& slash : texasSkill2Slashes_) {
        if (IsTextureValid(slash)) {
            UnloadTexture(slash);
        }
    }
    if (IsTextureValid(texasSkill2Aura_)) {
        UnloadTexture(texasSkill2Aura_);
    }
    for (Texture2D& skillIcon : texasSkillIcons_) {
        if (IsTextureValid(skillIcon)) {
            UnloadTexture(skillIcon);
        }
    }
    if (IsTextureValid(texasSkill2Battle_)) {
        UnloadTexture(texasSkill2Battle_);
    }
    if (IsTextureValid(texasChibi_)) {
        UnloadTexture(texasChibi_);
    }
    if (IsTextureValid(texasPortrait_)) {
        UnloadTexture(texasPortrait_);
    }
    for (Texture2D& skillIcon : skillIcons_) {
        if (IsTextureValid(skillIcon)) {
            UnloadTexture(skillIcon);
        }
    }
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

bool CharacterArt::HasPortrait(OperatorKind operatorKind) const {
    return operatorKind == OperatorKind::Texas
               ? IsTextureValid(texasPortrait_)
               : HasPortrait();
}

bool CharacterArt::HasChibi() const {
    return IsTextureValid(chibi_);
}

bool CharacterArt::HasChibi(OperatorKind operatorKind) const {
    return operatorKind == OperatorKind::Texas
               ? IsTextureValid(texasChibi_)
               : HasChibi();
}

bool CharacterArt::HasBattleChibi() const {
    return IsTextureValid(battleChibi_);
}

bool CharacterArt::HasTexasSkill2Battle() const {
    return IsTextureValid(texasSkill2Battle_);
}

bool CharacterArt::HasTexasSkill2Effects() const {
    return IsTextureValid(texasSkill2Aura_) &&
           IsTextureValid(texasSkill2Slashes_[0]) &&
           IsTextureValid(texasSkill2Slashes_[1]) &&
           IsTextureValid(texasSkill2Burst_);
}

bool CharacterArt::HasSkillIcon(int skillIndex) const {
    return skillIndex >= 0 && skillIndex < 2 &&
           IsTextureValid(skillIcons_[skillIndex]);
}

bool CharacterArt::HasSkillIcon(OperatorKind operatorKind,
                                int skillIndex) const {
    if (skillIndex < 0 || skillIndex >= 2) {
        return false;
    }
    return operatorKind == OperatorKind::Texas
               ? IsTextureValid(texasSkillIcons_[skillIndex])
               : IsTextureValid(skillIcons_[skillIndex]);
}

void CharacterArt::DrawPortrait(Rectangle destination, Color tint) const {
    DrawPortrait(OperatorKind::Exusiai, destination, tint);
}

void CharacterArt::DrawPortrait(OperatorKind operatorKind,
                                Rectangle destination, Color tint) const {
    if (!HasPortrait(operatorKind)) {
        return;
    }
    const Texture2D& portrait = operatorKind == OperatorKind::Texas
                                    ? texasPortrait_
                                    : portrait_;
    const float sourceWidth = static_cast<float>(portrait.width);
    const float sourceHeight = static_cast<float>(portrait.height);
    const float scale = std::min(destination.width / sourceWidth,
                                 destination.height / sourceHeight);
    const Rectangle fitted{
        destination.x + (destination.width - sourceWidth * scale) / 2.0F,
        destination.y + destination.height - sourceHeight * scale,
        sourceWidth * scale,
        sourceHeight * scale};
    DrawTexturePro(portrait, {0.0F, 0.0F, sourceWidth, sourceHeight},
                   fitted, {}, 0.0F, tint);
}

void CharacterArt::DrawChibi(Vector2 feetPosition, int facingDirection,
                             float height, ChibiAnimation animation,
                             float animationTime, Color tint) const {
    DrawChibi(OperatorKind::Exusiai, feetPosition, facingDirection, height,
              animation, animationTime, tint);
}

void CharacterArt::DrawChibi(OperatorKind operatorKind, Vector2 feetPosition,
                             int facingDirection, float height,
                             ChibiAnimation animation, float animationTime,
                             Color tint) const {
    if (!HasChibi(operatorKind)) {
        return;
    }

    const Texture2D& chibi = operatorKind == OperatorKind::Texas
                                 ? texasChibi_
                                 : chibi_;

    const float frameWidth = static_cast<float>(chibi.width) /
                             static_cast<float>(kChibiColumns);
    const float frameHeight = static_cast<float>(chibi.height) /
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
    DrawTexturePro(chibi, source, destination,
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
    } else if (animation == BattleChibiAnimation::Defeated) {
        row = 2;
        frame = std::min(static_cast<int>(animationTime * 12.0F),
                         kBattleColumns - 1);
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

void CharacterArt::DrawTexasSkill2Battle(Vector2 feetPosition,
                                         int facingDirection, float height,
                                         bool attacking, bool ending,
                                         float animationTime,
                                         Color tint) const {
    if (!HasTexasSkill2Battle()) {
        return;
    }

    const float frameWidth = static_cast<float>(texasSkill2Battle_.width) /
                             static_cast<float>(kTexasSkill2Columns);
    const float frameHeight = static_cast<float>(texasSkill2Battle_.height) /
                              static_cast<float>(kTexasSkill2Rows);
    const int row = ending ? 2 : (attacking ? 1 : 0);
    const float framesPerSecond = ending ? 60.0F : (attacking ? 55.0F : 6.0F);
    const int rawFrame = static_cast<int>(animationTime * framesPerSecond);
    const int frame = ending ? std::min(rawFrame, kTexasSkill2Columns - 1)
                             : rawFrame % kTexasSkill2Columns;
    const Rectangle source{frameWidth * static_cast<float>(frame),
                           frameHeight * static_cast<float>(row),
                           facingDirection < 0 ? -frameWidth : frameWidth,
                           frameHeight};
    const float width = height * frameWidth / frameHeight;
    const float transparentBottom = height * 24.0F / frameHeight;
    const Rectangle destination{feetPosition.x,
                                feetPosition.y + transparentBottom,
                                width, height};
    DrawTexturePro(texasSkill2Battle_, source, destination,
                   {width / 2.0F, height}, 0.0F, tint);
}

void CharacterArt::DrawTexasSkill2Aura(Vector2 center, int facingDirection,
                                       float animationTime, bool rainMode,
                                       bool transitioning,
                                       float transitionProgress) const {
    if (!IsTextureValid(texasSkill2Aura_)) {
        return;
    }

    const float progress = std::clamp(transitionProgress, 0.0F, 1.0F);
    const float direction = static_cast<float>(facingDirection);
    const float idlePulse = 0.5F + 0.5F * std::sin(animationTime * 5.4F);
    const float auraAlpha = rainMode
                                ? (transitioning
                                       ? 0.12F + SmoothStep(progress) * 0.32F
                                       : 0.14F + idlePulse * 0.07F)
                                : (transitioning
                                       ? (1.0F - SmoothStep(progress)) * 0.23F
                                       : 0.0F);
    const float auraSize = rainMode
                               ? (transitioning ? 126.0F + progress * 92.0F
                                                : 181.0F + idlePulse * 7.0F)
                               : 181.0F - progress * 42.0F;
    const Rectangle auraSource{
        0.0F, 0.0F,
        facingDirection < 0 ? -static_cast<float>(texasSkill2Aura_.width)
                            : static_cast<float>(texasSkill2Aura_.width),
        static_cast<float>(texasSkill2Aura_.height)};
    const Rectangle auraDestination{center.x - direction * 17.0F,
                                    center.y - 23.0F - progress * 9.0F,
                                    auraSize, auraSize};

    // The official wolf-shaped effect reads best as a pale after-image.  A dark
    // silhouette beneath it gives the S2 transition its black/red contrast.
    DrawTexturePro(texasSkill2Aura_, auraSource,
                   {auraDestination.x + direction * 4.0F,
                    auraDestination.y + 4.0F,
                    auraDestination.width, auraDestination.height},
                   {auraSize / 2.0F, auraSize / 2.0F},
                   direction * -4.0F,
                   Fade(Color{20, 5, 9, 255}, auraAlpha * 0.82F));
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexturePro(texasSkill2Aura_, auraSource, auraDestination,
                   {auraSize / 2.0F, auraSize / 2.0F},
                   std::sin(animationTime * 2.4F) * 2.0F,
                   Fade(rainMode ? Color{245, 32, 46, 255}
                                 : Color{176, 184, 196, 255},
                        auraAlpha));
    EndBlendMode();

    if (!transitioning) {
        return;
    }

    const float expansion = SmoothStep(progress);
    const float ringAlpha = (1.0F - progress) * (rainMode ? 0.78F : 0.46F);
    const float ringRadius = rainMode ? 30.0F + expansion * 130.0F
                                      : 112.0F - expansion * 70.0F;
    DrawRing(center, std::max(0.0F, ringRadius - 11.0F), ringRadius,
             0.0F, 360.0F, 72, Fade(BLACK, ringAlpha * 0.72F));
    DrawRing(center, std::max(0.0F, ringRadius - 4.0F), ringRadius,
             0.0F, 360.0F, 72,
             Fade(rainMode ? Color{246, 31, 43, 255}
                           : Color{202, 210, 219, 255},
                  ringAlpha));

    const float shardAlpha = PulseWindow(progress, 0.02F, 0.88F);
    for (int shard = 0; shard < 14; ++shard) {
        const float angle = Hash01(shard * 3 + 1) * 2.0F * PI;
        const float offset = Hash01(shard * 3 + 2);
        const float travel = rainMode ? 24.0F + expansion * (74.0F + offset * 70.0F)
                                      : 118.0F - expansion * (58.0F + offset * 34.0F);
        const float length = 14.0F + Hash01(shard * 3 + 3) * 31.0F;
        const Vector2 start = PolarPoint(center, angle, travel);
        const Vector2 end = PolarPoint(center, angle, travel + length);
        DrawLineEx(start, end, 5.0F + offset * 5.0F,
                   Fade(BLACK, shardAlpha * 0.72F));
        DrawLineEx(start, end, 1.5F + offset * 2.0F,
                   Fade(shard % 3 == 0 ? RAYWHITE
                                       : Color{239, 27, 42, 255},
                        shardAlpha * (rainMode ? 0.88F : 0.55F)));
    }
}

void CharacterArt::DrawTexasSkill2TransitionOverlay(
    Vector2 center, int facingDirection, bool rainMode,
    float transitionProgress) const {
    const float progress = std::clamp(transitionProgress, 0.0F, 1.0F);
    const float direction = static_cast<float>(facingDirection);
    const float flash = PulseWindow(progress, 0.0F, rainMode ? 0.44F : 0.58F);
    if (flash <= 0.0F) {
        return;
    }

    const float spread = rainMode ? 72.0F + SmoothStep(progress) * 98.0F
                                  : 132.0F - SmoothStep(progress) * 48.0F;
    const float tilt = rainMode ? 0.62F : 0.48F;
    const Vector2 slashAStart{center.x - direction * spread,
                              center.y - spread * tilt};
    const Vector2 slashAEnd{center.x + direction * spread,
                            center.y + spread * tilt};
    const Vector2 slashBStart{center.x - direction * spread * 0.88F,
                              center.y + spread * tilt * 0.95F};
    const Vector2 slashBEnd{center.x + direction * spread * 0.88F,
                            center.y - spread * tilt * 0.95F};

    DrawLineEx(slashAStart, slashAEnd, 28.0F,
               Fade(BLACK, flash * 0.88F));
    DrawLineEx(slashBStart, slashBEnd, 22.0F,
               Fade(BLACK, flash * 0.82F));
    DrawLineEx(slashAStart, slashAEnd, 9.0F,
               Fade(RAYWHITE, flash));
    DrawLineEx(slashBStart, slashBEnd, 6.0F,
               Fade(Color{247, 31, 45, 255}, flash * 0.96F));

    const float coreRadius = 11.0F + flash * 19.0F;
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleGradient(static_cast<int>(center.x), static_cast<int>(center.y),
                       coreRadius, Fade(WHITE, flash * 0.88F),
                       Fade(Color{237, 25, 40, 255}, 0.0F));
    EndBlendMode();
}

void CharacterArt::DrawTexasSkill2Slash(Vector2 center, int facingDirection,
                                        bool secondStrike,
                                        bool artsDamage, float remainingLife,
                                        float radius) const {
    const Texture2D& slash = texasSkill2Slashes_[secondStrike ? 1 : 0];
    if (!IsTextureValid(slash)) {
        return;
    }

    const float initialLife = secondStrike ? 0.155F : 0.12F;
    const float progress = std::clamp(1.0F - remainingLife / initialLife,
                                      0.0F, 1.0F);
    const float alpha = std::clamp(1.0F - progress * 0.88F, 0.0F, 1.0F);
    const float width = radius * (2.3F + progress * 0.55F);
    const float height = radius * (1.85F + progress * 0.35F);
    const float rotation = (secondStrike ? -18.0F : 16.0F) *
                           static_cast<float>(facingDirection);
    const Rectangle source{
        0.0F, 0.0F,
        facingDirection < 0 ? -static_cast<float>(slash.width)
                            : static_cast<float>(slash.width),
        static_cast<float>(slash.height)};

    // Black cannot survive additive blending.  Draw the official texture with
    // normal alpha first, then place a solid black-and-white cutting edge over
    // it so the S2 double slash stays crisp against bright backgrounds.
    DrawTexturePro(slash, source,
                   {center.x, center.y, width, height},
                   {width / 2.0F, height / 2.0F}, rotation,
                   Fade(artsDamage ? RAYWHITE
                                   : Color{190, 222, 239, 255},
                        alpha * 0.92F));

    const float direction = static_cast<float>(facingDirection);
    const float diagonal = secondStrike ? -1.0F : 1.0F;
    const Vector2 start{center.x - direction * radius * 0.86F,
                        center.y - diagonal * radius * 0.63F};
    const Vector2 end{center.x + direction * radius * 0.9F,
                      center.y + diagonal * radius * 0.63F};
    const Vector2 lead{end.x + direction * radius * 0.22F,
                       end.y + diagonal * radius * 0.08F};
    const Color outer = Fade(BLACK, alpha);
    const Color inner = Fade(artsDamage ? RAYWHITE
                                        : Color{211, 235, 246, 255},
                             alpha);
    DrawLineEx(start, lead, artsDamage ? 25.0F : 19.0F, outer);
    DrawLineEx(start, end, artsDamage ? 11.0F : 8.0F, inner);
    DrawCircleV(end, artsDamage ? 7.0F : 5.0F, inner);

    if (artsDamage) {
        const Vector2 echoStart{start.x + direction * 13.0F,
                                start.y - diagonal * 21.0F};
        const Vector2 echoEnd{end.x - direction * 8.0F,
                              end.y - diagonal * 21.0F};
        DrawLineEx(echoStart, echoEnd, 13.0F, Fade(BLACK, alpha * 0.94F));
        DrawLineEx(echoStart, echoEnd, 4.5F,
                   Fade(RAYWHITE, alpha * 0.92F));
    }
}

void CharacterArt::DrawSkillIcon(int skillIndex, Rectangle destination,
                                 Color tint) const {
    DrawSkillIcon(OperatorKind::Exusiai, skillIndex, destination, tint);
}

void CharacterArt::DrawSkillIcon(OperatorKind operatorKind, int skillIndex,
                                 Rectangle destination, Color tint) const {
    const bool validIndex = skillIndex >= 0 && skillIndex < 2;
    const Texture2D& texture = operatorKind == OperatorKind::Texas
                                   ? texasSkillIcons_[validIndex ? skillIndex : 0]
                                   : skillIcons_[validIndex ? skillIndex : 0];
    if (!validIndex || !IsTextureValid(texture)) {
        return;
    }
    DrawTexturePro(texture,
                   {0.0F, 0.0F, static_cast<float>(texture.width),
                    static_cast<float>(texture.height)},
                   destination, {}, 0.0F, tint);
}
