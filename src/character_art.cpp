#include "character_art.h"
#include "file_path.h"
#include "character_image.h"
#include "texas_animation.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace {

std::string AssetPath(const char* relativePath) {
    return (Utf8Path(GetApplicationDirectory()) / relativePath).string();
}

constexpr int kChibiColumns = 8;
constexpr int kChibiRows = 11;
constexpr int kBattleColumns = 12;
constexpr int kBattleRows = 3;
constexpr int kTexasSkill2Columns = 12;
constexpr int kTexasSkill2Rows = 3;
constexpr int kTexasSkill2CompositeColumns = 8;
constexpr int kTexasSkill2CompositeFrameCount = 65;
constexpr float kTexasSkill2CompositeDuration = 0.52F;

struct AnimationStrip {
    int row;
    int frameCount;
    float framesPerSecond;
    bool hasDirectionalRows;
};

AnimationStrip StripFor(ChibiAnimation animation) {
    switch (animation) {
        case ChibiAnimation::Run:
            return {1, 8, 9.0F, true};
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

void CharacterArt::RegisterCharacter(const Character& character, const std::filesystem::path& assetRoot) {
    const auto load = [&](const std::string& ref, CharacterFrameMap* frames = nullptr) -> Texture2D {
        if (ref.empty()) return {};
        if (ref.find("://") != std::string::npos) {
            TraceLog(LOG_WARNING, "Remote artwork requires an API cache resolver: %s", ref.c_str()); return {};
        }
        Image image = LoadCharacterImage(assetRoot / Utf8Path(ref));
        if (!image.data) { TraceLog(LOG_WARNING, "Cannot load character artwork: %s", ref.c_str()); return {}; }
        if (frames) *frames = FindCharacterFrames(image, character.assets);
        auto texture = LoadTextureFromImage(image);
        UnloadImage(image);
        if (texture.id) SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
        return texture;
    };
    auto& textures = custom_[character.id];
    for (const auto& [slot, texture] : textures.parts) if (texture.id) UnloadTexture(texture);
    for (auto texture : {textures.portrait, textures.sprite, textures.icons[0], textures.icons[1]})
        if (texture.id) UnloadTexture(texture);
    textures = {};
    textures.portrait = load(character.assets.portrait);
    textures.sprite = load(character.assets.sprite, &textures.frames);
    for (const auto& [slot, part] : character.assets.parts) textures.parts[slot] = load(part.image);
    if (textures.sprite.id && (textures.sprite.width < character.assets.columns ||
        textures.sprite.height < character.assets.rows || textures.sprite.width % character.assets.columns != 0 ||
        textures.sprite.height % character.assets.rows != 0)) {
        TraceLog(LOG_WARNING, "Invalid sprite dimensions for character: %s", character.id.c_str());
        UnloadTexture(textures.sprite); textures.sprite = {};
    }
    for (std::size_t i = 0; i < character.skills.size(); ++i) textures.icons[i] = load(character.skills[i].icon);
}

void CharacterArt::DrawCustomPortrait(const std::string& id, Rectangle destination, Color tint) const {
    const auto found = custom_.find(id);
    if (found == custom_.end() || !found->second.portrait.id) {
        DrawCircle(static_cast<int>(destination.x + destination.width / 2),
                   static_cast<int>(destination.y + destination.height * 0.35F), 38, Fade(SKYBLUE, 0.6F));
        DrawRectangleRec({destination.x + destination.width * 0.25F, destination.y + destination.height * 0.5F,
                          destination.width * 0.5F, destination.height * 0.45F}, Fade(SKYBLUE, 0.4F));
        return;
    }
    const auto t = found->second.portrait;
    const float scale = std::min(destination.width / t.width, destination.height / t.height);
    DrawTexturePro(t, {0, 0, static_cast<float>(t.width), static_cast<float>(t.height)},
                   {destination.x + (destination.width - t.width * scale) / 2,
                    destination.y + destination.height - t.height * scale, t.width * scale, t.height * scale}, {}, 0, tint);
}

void CharacterArt::DrawCustomSprite(const Character& c, Vector2 feet, int facing, int row, float time, Color tint) const {
    if (!c.assets.parts.empty() && (c.assets.animationMode == "auto" || c.assets.animationMode == "rig")) {
        CharacterAnimator animator; animator.Update(0.016F, {});
        DrawAnimatedCharacter(c, feet, facing, animator, tint); return;
    }
    const auto found = custom_.find(c.id);
    if (found == custom_.end() || !found->second.sprite.id) {
        DrawRectangleRec({feet.x - 19, feet.y - 78, 38, 78}, tint);
        DrawLineEx({feet.x, feet.y - 55}, {feet.x + facing * 40.0F, feet.y - 55}, 5, SKYBLUE);
        return;
    }
    const auto t = found->second.sprite;
    const float w = static_cast<float>(t.width) / c.assets.columns;
    const float h = static_cast<float>(t.height) / c.assets.rows;
    const auto frame = SampleCharacterFrame(found->second.frames, row, c.assets.idleRow, c.assets.fps, time);
    if (!frame.valid) return;
    const float scale = std::min(c.assets.height / h, c.assets.height * 1.5F / w);
    const float width = w * scale, height = h * scale;
    DrawTexturePro(t, {frame.column * w, frame.row * h, facing < 0 ? -w : w, h},
                   {feet.x - width / 2, feet.y - height, width, height}, {}, 0, tint);
}

void CharacterArt::DrawAnimatedCharacter(const Character& c, Vector2 feet, int facing,
                                         const CharacterAnimator& animator, Color tint) const {
    const auto found = custom_.find(c.id);
    if (found == custom_.end()) return;
    const auto& textures = found->second;
    const auto& a = c.assets;
    const bool rig = (a.animationMode == "auto" || a.animationMode == "rig") &&
                     textures.parts.count("torso") && textures.parts.at("torso").id;
    const bool frames = !rig && (a.animationMode == "frames" ||
        (a.animationMode == "auto" && (a.columns > 1 || a.rows > 1)));
    const int row = animator.Row(a);
    auto pose = animator.Pose();
    const float strength = frames && row >= 0 ? 0 : a.motionStrength;
    pose.scale = {1 + (pose.scale.x - 1) * strength, 1 + (pose.scale.y - 1) * strength};
    const float rootAngle = pose.angle * strength * facing;
    const Vector2 base{feet.x + pose.offset.x * a.height * strength * facing,
                       feet.y + pose.offset.y * a.height * strength};
    const auto rotate = [](Vector2 point, float degrees) {
        const float angle = degrees * DEG2RAD;
        return Vector2{point.x * std::cos(angle) - point.y * std::sin(angle),
                       point.x * std::sin(angle) + point.y * std::cos(angle)};
    };
    const auto angleFor = [&](const std::string& slot) {
        if (slot == "armFront" || slot == "weapon") return pose.armFront * strength;
        if (slot == "armBack") return pose.armBack * strength;
        if (slot == "legFront") return pose.legFront * strength;
        if (slot == "legBack") return pose.legBack * strength;
        return slot == "head" ? pose.head * strength : 0.0F;
    };
    if (rig) {
        // Back limbs first; weapon inherits the front arm's joint transform.
        for (const char* slot : {"legBack", "armBack", "torso", "legFront", "head", "armFront", "weapon"}) {
            const auto part = a.parts.find(slot); const auto tex = textures.parts.find(slot);
            if (part == a.parts.end() || tex == textures.parts.end() || !tex->second.id) continue;
            const auto& p = part->second; const auto t = tex->second;
            Vector2 joint{p.x * a.height, p.y * a.height};
            if (std::string(slot) == "weapon") {
                if (!textures.parts.at("armFront").id) continue;
                const auto& arm = a.parts.at("armFront");
                const auto grip = rotate({p.x * arm.height * a.height, p.y * arm.height * a.height}, angleFor("armFront"));
                joint = {arm.x * a.height + grip.x, arm.y * a.height + grip.y};
            }
            joint = rotate({joint.x * pose.scale.x * facing, joint.y * pose.scale.y}, rootAngle);
            const float height = p.height * a.height * pose.scale.y;
            const float width = p.height * a.height * static_cast<float>(t.width) / t.height * pose.scale.x;
            DrawTexturePro(t, {0, 0, facing < 0 ? -static_cast<float>(t.width) : static_cast<float>(t.width), static_cast<float>(t.height)},
                           {base.x + joint.x, base.y + joint.y, width, height},
                           {(facing < 0 ? 1 - p.pivotX : p.pivotX) * width, p.pivotY * height},
                           rootAngle + angleFor(slot) * facing, tint);
        }
        return;
    }
    if (!textures.sprite.id) {
        DrawRectanglePro({base.x, base.y, 38 * pose.scale.x, 78 * pose.scale.y},
                         {19 * pose.scale.x, 78 * pose.scale.y}, rootAngle, tint); return;
    }
    const auto t = textures.sprite;
    const float w = static_cast<float>(t.width) / a.columns, h = static_cast<float>(t.height) / a.rows;
    const auto frame = SampleCharacterFrame(textures.frames, row, a.idleRow, a.fps, animator.ActionTime(),
                                           animator.Action() != CharacterAction::Defeated);
    if (!frame.valid) {
        DrawRectangleRec({feet.x - 19, feet.y - 78, 38, 78}, tint); return;
    }
    const float scale = std::min(a.height / h, a.height * 1.5F / w);
    const float width = w * scale * pose.scale.x, height = h * scale * pose.scale.y;
    DrawTexturePro(t, {frame.column * w, frame.row * h, facing < 0 ? -w : w, h},
                   {base.x, base.y, width, height}, {width / 2, height}, rootAngle, tint);
}

void CharacterArt::DrawCustomSkill(const std::string& id, int slot, Rectangle destination) const {
    const auto found = custom_.find(id);
    if (found == custom_.end() || slot < 0 || slot >= 2 || !found->second.icons[slot].id) return;
    const auto t = found->second.icons[slot];
    DrawTexturePro(t, {0, 0, static_cast<float>(t.width), static_cast<float>(t.height)}, destination, {}, 0, WHITE);
}

void CharacterArt::DrawFacingRing(Vector2 ground, int facing) const {
    DrawEllipse(int(ground.x), int(ground.y), 34, 7, Fade(BLACK, 0.25F));
    if (facingRing_.id) {
        DrawTexturePro(facingRing_, {0, 0, float(facingRing_.width), float(facingRing_.height)},
                       {ground.x - 39, ground.y - 10, 78, 20}, {}, 0, Fade(WHITE, 0.8F));
    } else DrawEllipseLines(int(ground.x), int(ground.y), 34, 8, WHITE);
    if (facingArrow_.id) {
        DrawTexturePro(facingArrow_, {0, 0, float(facingArrow_.width) * (facing < 0 ? -1 : 1), float(facingArrow_.height)},
                       {ground.x + facing * 40 - 8, ground.y - 4, 16, 8}, {}, 0, WHITE);
    }
}

CharacterArt::CharacterArt() {
    const auto ringPath = AssetPath("assets/ui/client/ring.png");
    const auto arrowPath = AssetPath("assets/ui/client/direction.png");
    if (FileExists(ringPath.c_str())) {
        facingRing_ = LoadTexture(ringPath.c_str());
        SetTextureFilter(facingRing_, TEXTURE_FILTER_BILINEAR);
    }
    if (FileExists(arrowPath.c_str())) {
        facingArrow_ = LoadTexture(arrowPath.c_str());
        SetTextureFilter(facingArrow_, TEXTURE_FILTER_BILINEAR);
    }
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
    const std::string texasBattlePath = AssetPath("assets/operators/texas_battle.png");
    if (FileExists(texasBattlePath.c_str())) {
        texasBattle_ = LoadTexture(texasBattlePath.c_str());
        SetTextureFilter(texasBattle_, TEXTURE_FILTER_BILINEAR);
    }
    const std::string texasSkill2BattlePath =
        AssetPath("assets/operators/texas_skill2_battle.png");
    if (FileExists(texasSkill2BattlePath.c_str())) {
        texasSkill2Battle_ = LoadTexture(texasSkill2BattlePath.c_str());
        SetTextureFilter(texasSkill2Battle_, TEXTURE_FILTER_BILINEAR);
    }
    const char* texasEffectPaths[9] = {
        "assets/operators/texas_skill2_aura.png",
        "assets/operators/texas_skill2_slash_a.png",
        "assets/operators/texas_skill2_slash_b.png",
        "assets/operators/texas_skill2_burst.png",
        "assets/operators/texas_skill2_composite.png",
        "assets/operators/texas_skill2_hit_composite.png",
        "assets/operators/texas_skill2_dark_trail.png",
        "assets/operators/texas_skill2_arc.png",
        "assets/operators/texas_skill2_impact.png"};
    Texture2D* texasEffectTextures[9] = {
        &texasSkill2Aura_, &texasSkill2Slashes_[0],
        &texasSkill2Slashes_[1], &texasSkill2Burst_,
        &texasSkill2Composite_, &texasSkill2HitComposite_,
        &texasSkill2DarkTrail_, &texasSkill2Arc_, &texasSkill2Impact_};
    for (int index = 0; index < 9; ++index) {
        const std::string effectPath = AssetPath(texasEffectPaths[index]);
        if (FileExists(effectPath.c_str())) {
            *texasEffectTextures[index] = LoadTexture(effectPath.c_str());
            SetTextureFilter(*texasEffectTextures[index],
                             TEXTURE_FILTER_BILINEAR);
        }
    }
    const std::string texasSwordPath =
        AssetPath("assets/operators/texas_sword.png");
    if (FileExists(texasSwordPath.c_str())) {
        texasSword_ = LoadTexture(texasSwordPath.c_str());
        SetTextureFilter(texasSword_, TEXTURE_FILTER_BILINEAR);
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

void CharacterArt::ClearCustomCharacters() {
    for (const auto& [id, textures] : custom_) {
        for (const auto& [slot, texture] : textures.parts) if (texture.id) UnloadTexture(texture);
        for (auto texture : {textures.portrait, textures.sprite, textures.icons[0], textures.icons[1]})
            if (texture.id) UnloadTexture(texture);
    }
    custom_.clear();
}

CharacterArt::~CharacterArt() {
    if (facingRing_.id) UnloadTexture(facingRing_);
    if (facingArrow_.id) UnloadTexture(facingArrow_);
    ClearCustomCharacters();
    if (IsTextureValid(texasSkill2Impact_)) {
        UnloadTexture(texasSkill2Impact_);
    }
    if (IsTextureValid(texasSword_)) {
        UnloadTexture(texasSword_);
    }
    if (IsTextureValid(texasSkill2Arc_)) {
        UnloadTexture(texasSkill2Arc_);
    }
    if (IsTextureValid(texasSkill2DarkTrail_)) {
        UnloadTexture(texasSkill2DarkTrail_);
    }
    if (IsTextureValid(texasSkill2Composite_)) {
        UnloadTexture(texasSkill2Composite_);
    }
    if (IsTextureValid(texasSkill2HitComposite_)) {
        UnloadTexture(texasSkill2HitComposite_);
    }
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
    if (IsTextureValid(texasBattle_)) {
        UnloadTexture(texasBattle_);
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

bool CharacterArt::HasTexasBattle() const {
    return IsTextureValid(texasBattle_) &&
           texasBattle_.width == TexasBattleAnimation::Columns * TexasBattleAnimation::Cell &&
           texasBattle_.height == TexasBattleAnimation::Rows * TexasBattleAnimation::Cell;
}

bool CharacterArt::HasTexasSkill2Effects() const {
    return IsTextureValid(texasSkill2Aura_) &&
           IsTextureValid(texasSkill2Composite_) &&
           IsTextureValid(texasSkill2HitComposite_) &&
           IsTextureValid(texasSkill2DarkTrail_) &&
           IsTextureValid(texasSkill2Arc_) &&
           IsTextureValid(texasSkill2Impact_);
}

bool CharacterArt::HasTexasSword() const {
    return IsTextureValid(texasSword_);
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

void CharacterArt::DrawExusiai(Vector2 feetPosition, int facingDirection, ChibiAnimation movement,
                                bool attacking, bool defeated, float movementTime, float attackTime,
                                float defeatTime, Color tint) const {
    // Keep the authored gun pose while borrowing the walking sheet's lower body.
    // This lets Exusiai move and fire without making her weapon disappear.
    if (!defeated && attacking && movement == ChibiAnimation::Run &&
        HasChibi() && HasBattleChibi()) {
        const float walkFrameWidth = static_cast<float>(chibi_.width) /
                                     static_cast<float>(kChibiColumns);
        const float walkFrameHeight = static_cast<float>(chibi_.height) /
                                      static_cast<float>(kChibiRows);
        const AnimationStrip walkStrip = StripFor(ChibiAnimation::Run);
        const int walkFrame =
            static_cast<int>(movementTime * walkStrip.framesPerSecond) %
            walkStrip.frameCount;
        const int walkRow = facingDirection >= 0
                                ? walkStrip.row
                                : walkStrip.row + 1;
        constexpr float lowerBodyStart = 0.48F;
        constexpr float walkHeight = 132.0F;
        const float walkWidth = walkHeight * walkFrameWidth / walkFrameHeight;
        const float lowerBodyHeight = walkHeight * (1.0F - lowerBodyStart);
        DrawTexturePro(
            chibi_,
            {walkFrameWidth * static_cast<float>(walkFrame),
             walkFrameHeight * (static_cast<float>(walkRow) + lowerBodyStart),
             walkFrameWidth, walkFrameHeight * (1.0F - lowerBodyStart)},
            {feetPosition.x, feetPosition.y, walkWidth, lowerBodyHeight},
            {walkWidth / 2.0F, lowerBodyHeight}, 0.0F, tint);

        const float attackFrameWidth = static_cast<float>(battleChibi_.width) /
                                       static_cast<float>(kBattleColumns);
        const float attackFrameHeight = static_cast<float>(battleChibi_.height) /
                                        static_cast<float>(kBattleRows);
        constexpr int raisedGunFirstFrame = 3;
        constexpr int raisedGunFrameCount = 6;
        const int attackFrame = raisedGunFirstFrame +
            static_cast<int>(attackTime * 18.0F) % raisedGunFrameCount;
        constexpr float upperBodyEnd = 0.72F;
        constexpr float attackHeight = 120.0F;
        const float attackWidth =
            attackHeight * attackFrameWidth / attackFrameHeight;
        DrawTexturePro(
            battleChibi_,
            {attackFrameWidth * static_cast<float>(attackFrame),
             attackFrameHeight,
             facingDirection < 0 ? -attackFrameWidth : attackFrameWidth,
             attackFrameHeight * upperBodyEnd},
            {feetPosition.x, feetPosition.y,
             attackWidth, attackHeight * upperBodyEnd},
            {attackWidth / 2.0F, attackHeight}, 0.0F, tint);
        return;
    }

    // The 12x3 combat sheet has no locomotion. Use the authored directional walk sheet.
    if (!defeated && !attacking && movement != ChibiAnimation::Idle && HasChibi()) {
        DrawChibi(OperatorKind::Exusiai, {feetPosition.x, feetPosition.y + 4.0F}, facingDirection,
                  132.0F, movement, movementTime, tint);
    } else if (HasBattleChibi()) {
        DrawBattleChibi(feetPosition, facingDirection, 120.0F,
                       defeated ? BattleChibiAnimation::Defeated : attacking ? BattleChibiAnimation::Attack : BattleChibiAnimation::Idle,
                       defeated ? defeatTime : attacking ? attackTime : movementTime, tint);
    } else if (HasChibi()) {
        DrawChibi(OperatorKind::Exusiai, feetPosition, facingDirection, 132.0F,
                  movement, movementTime, tint);
    }
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

void CharacterArt::DrawTexas(Vector2 feetPosition, int facingDirection,
                             ChibiAnimation movement, bool attacking, bool defeated,
                             float movementTime, float attackTime, float defeatTime,
                             Color tint) const {
    if (!defeated && !attacking && movement != ChibiAnimation::Idle &&
        HasChibi(OperatorKind::Texas)) {
        DrawChibi(OperatorKind::Texas, feetPosition, facingDirection, 112.0F,
                  movement, movementTime, tint);
        return;
    }
    if (!HasTexasBattle()) {
        // Missing combat art must never turn an attack into a building interaction.
        DrawChibi(OperatorKind::Texas, feetPosition, facingDirection, 112.0F,
                  movement, movementTime, tint);
        return;
    }
    using namespace TexasBattleAnimation;
    const int row = defeated ? 2 : attacking ? 1 : 0;
    const float time = std::max(0.0F, defeated ? defeatTime : attacking ? attackTime : movementTime);
    const float duration = defeated ? DefeatDuration : attacking ? AttackDuration : IdleDuration;
    const int frame = row == 0 ? static_cast<int>(std::fmod(time, duration) / duration * Columns)
                              : std::min(static_cast<int>(time / duration * (Columns - 1)), Columns - 1);
    const float scale = VisibleHeight / IdleHeight;
    const bool flipped = facingDirection < 0;
    DrawTexturePro(texasBattle_, {frame * Cell, row * Cell, flipped ? -Cell : Cell, Cell},
                   {feetPosition.x, feetPosition.y - GroundInset, Cell * scale, Cell * scale},
                   {(flipped ? Cell - AnchorX : AnchorX) * scale, AnchorY * scale}, 0.0F, tint);
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
    if (!IsTextureValid(texasSkill2Arc_) ||
        !IsTextureValid(texasSkill2Impact_) ||
        !IsTextureValid(texasSkill2DarkTrail_)) {
        return;
    }

    const float progress = std::clamp(transitionProgress, 0.0F, 1.0F);
    const float direction = static_cast<float>(facingDirection);
    const float firstArc = PulseWindow(progress, 0.01F,
                                       rainMode ? 0.62F : 0.76F);
    const float secondArc = PulseWindow(progress, 0.09F,
                                        rainMode ? 0.79F : 0.92F);
    const float expansion = rainMode ? 0.88F + SmoothStep(progress) * 0.24F
                                     : 1.08F - SmoothStep(progress) * 0.18F;
    const Rectangle arcSource{
        0.0F, 0.0F,
        facingDirection < 0 ? -static_cast<float>(texasSkill2Arc_.width)
                            : static_cast<float>(texasSkill2Arc_.width),
        static_cast<float>(texasSkill2Arc_.height)};
    const Rectangle darkSource{
        0.0F, 0.0F,
        facingDirection < 0
            ? -static_cast<float>(texasSkill2DarkTrail_.width)
            : static_cast<float>(texasSkill2DarkTrail_.width),
        static_cast<float>(texasSkill2DarkTrail_.height)};

    const auto drawArc = [&](float width, float height, float rotation,
                             Color color, float alpha) {
        DrawTexturePro(texasSkill2Arc_, arcSource,
                       {center.x, center.y - 7.0F, width, height},
                       {width / 2.0F, height / 2.0F}, rotation,
                       Fade(color, std::clamp(alpha, 0.0F, 1.0F)));
    };

    // The official effect does not bake black into its additive blade texture.
    // texas2_daoguang_an renders trail_47_C as a separate translucent ribbon.
    // Draw that ribbon with normal alpha before the blue/white additive layers.
    const auto drawDarkTrail = [&](float width, float height, float rotation,
                                   float opacity) {
        DrawTexturePro(texasSkill2DarkTrail_, darkSource,
                       {center.x, center.y - 8.0F, width, height},
                       {width / 2.0F, height / 2.0F}, rotation,
                       Fade(Color{88, 88, 88, 255},
                            std::clamp(opacity, 0.0F, 1.0F)));
    };
    drawDarkTrail(330.0F * expansion, 166.0F * expansion,
                  5.0F * direction, firstArc * 0.18F);
    drawDarkTrail(305.0F * expansion, 142.0F * expansion,
                  -13.0F * direction, secondArc * 0.46F);

    // The second half of the recorded attack is a red-black upper crescent.
    // Together with the first lower sweep it closes into a fast cross cut.
    drawArc(170.0F * expansion, 318.0F * expansion,
            -76.0F * direction, Color{35, 8, 13, 255}, secondArc * 0.92F);
    drawArc(158.0F * expansion, 306.0F * expansion,
            -76.0F * direction, Color{139, 19, 31, 255}, secondArc * 0.52F);

    BeginBlendMode(BLEND_ADDITIVE);
    drawArc(160.0F * expansion, 326.0F * expansion,
            88.0F * direction, Color{91, 159, 255, 255}, firstArc * 0.62F);
    drawArc(142.0F * expansion, 310.0F * expansion,
            88.0F * direction, RAYWHITE, firstArc * 0.98F);
    drawArc(150.0F * expansion, 292.0F * expansion,
            -76.0F * direction, Color{218, 36, 55, 255}, secondArc * 0.50F);
    drawArc(136.0F * expansion, 278.0F * expansion,
            -76.0F * direction, Color{255, 225, 230, 255},
            secondArc * 0.80F);

    const float impactAlpha = PulseWindow(progress, 0.025F, 0.52F);
    const float impactSize = 72.0F + SmoothStep(progress) * 34.0F;
    DrawTexturePro(
        texasSkill2Impact_,
        {0.0F, 0.0F, static_cast<float>(texasSkill2Impact_.width),
         static_cast<float>(texasSkill2Impact_.height)},
        {center.x - direction * 75.0F, center.y - 13.0F,
         impactSize, impactSize},
        {impactSize / 2.0F, impactSize / 2.0F},
        -14.0F * direction,
        Fade(Color{220, 237, 255, 255}, impactAlpha));
    EndBlendMode();
}

void CharacterArt::DrawTexasSkill2Slash(Vector2 center, int facingDirection,
                                        bool secondStrike,
                                        bool artsDamage, float remainingLife,
                                        float radius) const {
    if (artsDamage && IsTextureValid(texasSkill2Composite_) &&
        IsTextureValid(texasSkill2HitComposite_)) {
        // The supplied transparent render already contains both halves of the
        // cross cut.  The second melee bullet still applies delayed damage,
        // but must not draw the complete animation a second time.
        if (secondStrike) {
            return;
        }
        const float progress = std::clamp(
            1.0F - remainingLife / kTexasSkill2CompositeDuration,
            0.0F, 0.9999F);
        const int frame = std::min(
            static_cast<int>(progress * kTexasSkill2CompositeFrameCount),
            kTexasSkill2CompositeFrameCount - 1);
        const int row = frame / kTexasSkill2CompositeColumns;
        const int column = frame % kTexasSkill2CompositeColumns;
        const float frameWidth = static_cast<float>(
            texasSkill2Composite_.width / kTexasSkill2CompositeColumns);
        const float frameHeight = frameWidth * 0.5F;
        const float sourceX = facingDirection < 0
                                  ? (static_cast<float>(column) + 1.0F) *
                                        frameWidth
                                  : static_cast<float>(column) * frameWidth;
        const Rectangle source{
            sourceX, static_cast<float>(row) * frameHeight,
            facingDirection < 0 ? -frameWidth : frameWidth, frameHeight};
        const float direction = static_cast<float>(facingDirection);
        const Vector2 effectCenter{
            center.x - direction * radius * 1.02F + direction * 10.0F,
            center.y - 22.0F};
        constexpr float drawWidth = 430.0F;
        constexpr float drawHeight = 215.0F;
        DrawTexturePro(texasSkill2Composite_, source,
                       {effectCenter.x, effectCenter.y,
                        drawWidth, drawHeight},
                       {drawWidth / 2.0F, drawHeight / 2.0F}, 0.0F, WHITE);
        DrawTexturePro(texasSkill2HitComposite_, source,
                       {effectCenter.x, effectCenter.y,
                        drawWidth, drawHeight},
                       {drawWidth / 2.0F, drawHeight / 2.0F}, 0.0F, WHITE);
        return;
    }

    const float initialLife = artsDamage ? 0.26F : 0.17F;
    const float progress = std::clamp(1.0F - remainingLife / initialLife,
                                      0.0F, 1.0F);
    const float direction = static_cast<float>(facingDirection);
    if (!artsDamage || !IsTextureValid(texasSkill2Arc_) ||
        !IsTextureValid(texasSkill2Impact_) ||
        !IsTextureValid(texasSkill2DarkTrail_)) {
        const Texture2D& slash = texasSkill2Slashes_[secondStrike ? 1 : 0];
        if (!IsTextureValid(slash)) {
            return;
        }
        const float alpha = 1.0F - SmoothStep(progress);
        const float size = radius * (2.1F + progress * 0.35F);
        DrawTexturePro(
            slash,
            {0.0F, 0.0F,
             facingDirection < 0 ? -static_cast<float>(slash.width)
                                 : static_cast<float>(slash.width),
             static_cast<float>(slash.height)},
            {center.x, center.y, size, size}, {size / 2.0F, size / 2.0F},
            (secondStrike ? -12.0F : 12.0F) * direction,
            Fade(Color{204, 229, 247, 255}, alpha));
        return;
    }

    const float fadeIn = std::clamp(progress / 0.09F, 0.0F, 1.0F);
    const float fadeOut = 1.0F - SmoothStep(
        std::clamp((progress - 0.16F) / 0.84F, 0.0F, 1.0F));
    const float alpha = fadeIn * fadeOut;
    const float darkFadeOut = 1.0F - SmoothStep(
        std::clamp((progress - 0.30F) / 0.70F, 0.0F, 1.0F));
    const float darkAlpha = fadeIn * darkFadeOut;
    const float growth = 0.91F + SmoothStep(progress) * 0.23F;
    const Vector2 effectCenter{center.x - direction * radius * 1.02F,
                               center.y - 7.0F};
    const float rotation = (secondStrike ? -78.0F : 89.0F) * direction;
    const Rectangle arcSource{
        0.0F, 0.0F,
        facingDirection < 0 ? -static_cast<float>(texasSkill2Arc_.width)
                            : static_cast<float>(texasSkill2Arc_.width),
        static_cast<float>(texasSkill2Arc_.height)};
    const Rectangle darkSource{
        0.0F, 0.0F,
        facingDirection < 0
            ? -static_cast<float>(texasSkill2DarkTrail_.width)
            : static_cast<float>(texasSkill2DarkTrail_.width),
        static_cast<float>(texasSkill2DarkTrail_.height)};
    const auto drawArc = [&](float width, float height, float angle,
                             Color color, float opacity) {
        DrawTexturePro(texasSkill2Arc_, arcSource,
                       {effectCenter.x, effectCenter.y, width, height},
                       {width / 2.0F, height / 2.0F}, angle,
                       Fade(color, std::clamp(opacity, 0.0F, 1.0F)));
    };

    const float darkRotation = (secondStrike ? -12.0F : 7.0F) * direction;
    const float darkWidth = (secondStrike ? 318.0F : 342.0F) * growth;
    const float darkHeight = (secondStrike ? 142.0F : 158.0F) * growth;
    const Vector2 darkCenter{
        effectCenter.x - direction * (secondStrike ? 4.0F : 12.0F),
        effectCenter.y + (secondStrike ? 3.0F : -5.0F)};
    DrawTexturePro(
        texasSkill2DarkTrail_, darkSource,
        {darkCenter.x, darkCenter.y, darkWidth, darkHeight},
        {darkWidth / 2.0F, darkHeight / 2.0F}, darkRotation,
        Fade(Color{88, 88, 88, 255},
             darkAlpha * (secondStrike ? 0.36F : 0.0F)));
    // A short offset copy reproduces the broad, fast-moving dark wake without
    // putting a synthetic black outline around the white cutting edge.
    DrawTexturePro(
        texasSkill2DarkTrail_, darkSource,
        {darkCenter.x - direction * 14.0F, darkCenter.y + 5.0F,
         darkWidth * 0.94F, darkHeight * 0.86F},
        {darkWidth * 0.47F, darkHeight * 0.43F},
        darkRotation - direction * 4.0F,
        Fade(Color{56, 58, 64, 255},
             darkAlpha * (secondStrike ? 0.16F : 0.0F)));

    if (secondStrike) {
        // The source video shows a broad black/red upper blade, not another
        // blue-white copy.  Its pale leading edge is added in the next pass.
        drawArc(174.0F * growth, 338.0F * growth,
                rotation - direction * 2.0F,
                Color{31, 7, 12, 255}, darkAlpha * 0.96F);
        drawArc(160.0F * growth, 324.0F * growth, rotation,
                Color{142, 17, 29, 255}, darkAlpha * 0.58F);
    }

    BeginBlendMode(BLEND_ADDITIVE);
    // Three close rotations create the swift motion echo visible in the
    // official S2 attack, while every layer fades instead of gaining an edge.
    if (secondStrike) {
        const float leadingEdge = 1.0F - SmoothStep(
            std::clamp((progress - 0.36F) / 0.46F, 0.0F, 1.0F));
        drawArc(143.0F * growth, 310.0F * growth,
                rotation - direction * 5.0F,
                Color{206, 27, 45, 255}, alpha * leadingEdge * 0.30F);
        drawArc(122.0F * growth, 286.0F * growth, rotation,
                Color{255, 226, 232, 255}, alpha * leadingEdge * 0.74F);
    } else {
        drawArc(148.0F * growth, 330.0F * growth,
                rotation - direction * 9.0F,
                Color{72, 118, 235, 255}, alpha * 0.22F);
        drawArc(140.0F * growth, 322.0F * growth,
                rotation - direction * 4.0F,
                Color{105, 169, 255, 255}, alpha * 0.48F);
        drawArc(130.0F * growth, 310.0F * growth, rotation,
                Color{236, 245, 255, 255}, alpha * 0.98F);
    }

    const float impactAlpha = PulseWindow(progress, 0.015F, 0.58F);
    const float impactSize = 64.0F + SmoothStep(progress) * 28.0F;
    const Vector2 impactCenter{
        effectCenter.x + direction * (secondStrike ? 68.0F : 72.0F),
        effectCenter.y - (secondStrike ? 15.0F : 4.0F)};
    DrawTexturePro(
        texasSkill2Impact_,
        {0.0F, 0.0F, static_cast<float>(texasSkill2Impact_.width),
         static_cast<float>(texasSkill2Impact_.height)},
        {impactCenter.x, impactCenter.y, impactSize, impactSize},
        {impactSize / 2.0F, impactSize / 2.0F},
        (secondStrike ? 18.0F : -12.0F) * direction,
        Fade(RAYWHITE, impactAlpha));
    EndBlendMode();
}

void CharacterArt::DrawTexasSword(Vector2 center, float length,
                                  float rotationDegrees, Color tint) const {
    if (!IsTextureValid(texasSword_) || length <= 0.0F) {
        return;
    }
    const float scale = length / static_cast<float>(texasSword_.height);
    const float drawWidth = static_cast<float>(texasSword_.width) * scale;
    const float drawHeight = static_cast<float>(texasSword_.height) * scale;
    DrawTexturePro(texasSword_,
                   {0.0F, 0.0F, static_cast<float>(texasSword_.width),
                    static_cast<float>(texasSword_.height)},
                   {center.x, center.y, drawWidth, drawHeight},
                   {drawWidth / 2.0F, drawHeight / 2.0F}, rotationDegrees, tint);
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
