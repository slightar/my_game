#include "character.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

using Json = nlohmann::json;
namespace {
void Check(bool condition, const std::string& message) {
    if (!condition) throw std::invalid_argument(message);
}
void Fields(const Json& j, std::initializer_list<const char*> fields) {
    Check(j.is_object(), "Expected a JSON object");
    const std::set<std::string> allowed(fields.begin(), fields.end());
    for (const auto& item : j.items()) Check(allowed.count(item.key()), "Unknown field: " + item.key());
}
void Number(float n, float low, float high, const char* field) {
    Check(std::isfinite(n) && n >= low && n <= high, std::string("Out of range: ") + field);
}
int Integer(const Json& j, const char* key, int fallback) {
    if (!j.contains(key)) return fallback;
    Check(j.at(key).is_number_integer(), std::string("Expected integer: ") + key);
    auto value = j.at(key).get<double>();
    Check(value >= 0 && value <= 100000, std::string("Integer out of range: ") + key);
    return static_cast<int>(value);
}
const char* EffectName(EffectType type) {
    switch (type) {
        case EffectType::Projectile: return "projectile";
        case EffectType::Melee: return "melee";
        case EffectType::Rain: return "rain";
        case EffectType::Heal: return "heal";
        case EffectType::Buff: return "buff";
    }
    throw std::invalid_argument("Invalid effect type");
}
Json EffectJson(const SkillEffect& e) {
    return {{"type", EffectName(e.type)}, {"damageType", e.damageType == DamageType::Arts ? "arts" : "physical"},
            {"amount", e.amount}, {"speed", e.speed}, {"radius", e.radius}, {"lifetime", e.lifetime},
            {"range", e.range}, {"stun", e.stun}, {"count", e.count}, {"spread", e.spread},
            {"destroysProjectiles", e.destroysProjectiles}, {"damageMultiplier", e.damageMultiplier},
            {"attackSpeedMultiplier", e.attackSpeedMultiplier}};
}
SkillEffect ReadEffect(const Json& j) {
    Fields(j, {"type", "damageType", "amount", "speed", "radius", "lifetime", "range", "stun",
               "count", "spread", "destroysProjectiles", "damageMultiplier", "attackSpeedMultiplier"});
    SkillEffect e;
    const auto type = j.value("type", std::string("projectile"));
    if (type == "projectile") e.type = EffectType::Projectile;
    else if (type == "melee") e.type = EffectType::Melee;
    else if (type == "rain") e.type = EffectType::Rain;
    else if (type == "heal") e.type = EffectType::Heal;
    else if (type == "buff") e.type = EffectType::Buff;
    else throw std::invalid_argument("Unknown effect type: " + type);
    const auto damage = j.value("damageType", std::string("physical"));
    Check(damage == "physical" || damage == "arts", "Unknown damageType");
    e.damageType = damage == "arts" ? DamageType::Arts : DamageType::Physical;
    e.amount = j.value("amount", e.amount); e.speed = j.value("speed", e.speed);
    e.radius = j.value("radius", e.radius); e.lifetime = j.value("lifetime", e.lifetime);
    e.range = j.value("range", e.range); e.stun = j.value("stun", e.stun);
    e.count = Integer(j, "count", e.count); e.spread = j.value("spread", e.spread);
    e.destroysProjectiles = j.value("destroysProjectiles", e.destroysProjectiles);
    e.damageMultiplier = j.value("damageMultiplier", e.damageMultiplier);
    e.attackSpeedMultiplier = j.value("attackSpeedMultiplier", e.attackSpeedMultiplier);
    return e;
}
void ValidateEffect(const SkillEffect& e) {
    EffectName(e.type);
    Check(e.damageType == DamageType::Physical || e.damageType == DamageType::Arts, "Invalid damageType");
    Number(e.amount, 0, 10000, "amount"); Number(e.speed, 1, 5000, "speed");
    Number(e.radius, 1, 300, "radius"); Number(e.lifetime, 0.01F, 10, "lifetime");
    Number(e.range, 1, 600, "range"); Number(e.stun, 0, 10, "stun");
    Number(e.spread, 0, 3.14F, "spread"); Check(e.count >= 1 && e.count <= 32, "count must be 1..32");
    Number(e.damageMultiplier, 0.1F, 10, "damageMultiplier");
    Number(e.attackSpeedMultiplier, 0.1F, 10, "attackSpeedMultiplier");
    if (e.type == EffectType::Heal) Check(std::floor(e.amount) == e.amount, "heal amount must be an integer");
}
void Asset(const std::string& path) {
    Check(path.size() <= 2048, "Asset reference too long");
    Check(path.find('\0') == std::string::npos, "NUL in asset reference");
}
}

CharacterPart DefaultCharacterPart(const std::string& slot) {
    CharacterPart p;
    if (slot == "torso") { p.y = -0.65F; p.height = 0.38F; }
    else if (slot == "head") { p.y = -0.67F; p.height = 0.36F; p.pivotY = 0.9F; }
    else if (slot == "armFront" || slot == "armBack") { p.x = slot == "armFront" ? 0.18F : -0.18F; p.y = -0.61F; p.height = 0.33F; }
    else if (slot == "legFront" || slot == "legBack") { p.x = slot == "legFront" ? 0.09F : -0.09F; p.y = -0.31F; p.height = 0.34F; }
    else if (slot == "weapon") { p.y = 0.85F; p.height = 0.4F; p.pivotY = 0.85F; }
    else throw std::invalid_argument("Unknown body part: " + slot);
    return p;
}

Character Character::FromJson(const std::string& text) {
    Check(text.size() <= 1024 * 1024, "Character JSON exceeds 1 MiB");
    const auto j = Json::parse(text);
    Fields(j, {"schemaVersion", "id", "name", "description", "assets", "stats", "attack", "skills"});
    Check(Integer(j, "schemaVersion", 1) == 1, "Unsupported schemaVersion");
    Character c;
    c.id = j.at("id").get<std::string>(); c.name = j.at("name").get<std::string>();
    c.description = j.value("description", std::string{});
    if (j.contains("assets")) {
        const auto& a = j.at("assets");
        Fields(a, {"portrait", "sprite", "columns", "rows", "idleRow", "runRow", "attackRow", "defeatedRow", "fps", "height",
                   "animationMode", "motionStrength", "jumpRow", "fallRow", "dodgeRow", "hurtRow", "parts", "rowFrameCounts"});
        c.assets.portrait = a.value("portrait", std::string{}); c.assets.sprite = a.value("sprite", std::string{});
        c.assets.columns = Integer(a, "columns", 1); c.assets.rows = Integer(a, "rows", 1);
        if (a.contains("rowFrameCounts")) {
            const auto& counts = a.at("rowFrameCounts");
            Check(counts.is_array() && counts.size() <= 64, "rowFrameCounts must be an array of at most 64 integers");
            for (const auto& count : counts) {
                Check(count.is_number_integer(), "Frame count must be an integer");
                const double value = count.get<double>();
                Check(value >= 1 && value <= c.assets.columns, "Frame count must be 1..columns");
                c.assets.rowFrameCounts.push_back(static_cast<int>(value));
            }
        }
        c.assets.idleRow = Integer(a, "idleRow", 0);
        c.assets.fps = a.value("fps", 8.0F); c.assets.height = a.value("height", 112.0F);
        c.assets.animationMode = a.value("animationMode", std::string("auto"));
        c.assets.motionStrength = a.value("motionStrength", 1.0F);
        const auto optionalRow = [&](const char* field) {
            if (!a.contains(field)) return -1;
            Check(a.at(field).is_number_integer(), std::string("Expected integer: ") + field);
            const double row = a.at(field).get<double>(); Check(row >= -1 && row <= 63, "Invalid animation row");
            return static_cast<int>(row);
        };
        c.assets.jumpRow = optionalRow("jumpRow"); c.assets.fallRow = optionalRow("fallRow");
        c.assets.runRow = optionalRow("runRow"); c.assets.attackRow = optionalRow("attackRow");
        c.assets.defeatedRow = optionalRow("defeatedRow");
        c.assets.dodgeRow = optionalRow("dodgeRow"); c.assets.hurtRow = optionalRow("hurtRow");
        if (a.contains("parts")) {
            Check(a.at("parts").is_object(), "parts must be an object");
            for (const auto& [slot, data] : a.at("parts").items()) {
                auto p = DefaultCharacterPart(slot);
                Fields(data, {"image", "x", "y", "height", "pivotX", "pivotY"});
                p.image = data.at("image").get<std::string>();
                p.x = data.value("x", p.x); p.y = data.value("y", p.y); p.height = data.value("height", p.height);
                p.pivotX = data.value("pivotX", p.pivotX); p.pivotY = data.value("pivotY", p.pivotY);
                c.assets.parts.emplace(slot, std::move(p));
            }
        }
    }
    if (j.contains("stats")) {
        const auto& s = j.at("stats"); Fields(s, {"health", "moveSpeed", "jumpSpeed", "jumps", "attackInterval"});
        c.stats.health = Integer(s, "health", 3); c.stats.jumps = Integer(s, "jumps", 2);
        c.stats.moveSpeed = s.value("moveSpeed", 320.0F); c.stats.jumpSpeed = s.value("jumpSpeed", 610.0F);
        c.stats.attackInterval = s.value("attackInterval", 0.25F);
    }
    if (j.contains("attack")) c.attack = ReadEffect(j.at("attack"));
    if (j.contains("skills")) {
        Check(j.at("skills").is_array() && j.at("skills").size() <= 2, "skills must be an array of at most 2 entries (E/Q)");
        for (const auto& s : j.at("skills")) {
            Fields(s, {"name", "description", "icon", "cooldown", "duration", "interval", "effects"});
            CharacterSkill skill;
            skill.name = s.at("name").get<std::string>(); skill.description = s.value("description", std::string{});
            skill.icon = s.value("icon", std::string{}); skill.cooldown = s.value("cooldown", 8.0F);
            skill.duration = s.value("duration", 0.0F); skill.interval = s.value("interval", 0.0F);
            Check(s.at("effects").is_array(), "effects must be an array");
            for (const auto& e : s.at("effects")) skill.effects.push_back(ReadEffect(e));
            c.skills.push_back(std::move(skill));
        }
    }
    c.Validate(); return c;
}

void Character::Validate() const {
    Check(!id.empty() && id.size() <= 64 && id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") == std::string::npos,
          "id must contain 1..64 lowercase ASCII letters, digits, _ or -");
    Check(!name.empty() && name.size() <= 120 && name.find('\0') == std::string::npos, "Invalid name");
    Check(description.size() <= 2048 && description.find('\0') == std::string::npos, "Invalid description");
    Asset(assets.portrait); Asset(assets.sprite);
    Check(assets.columns >= 1 && assets.columns <= 64 && assets.rows >= 1 && assets.rows <= 64, "Invalid sprite grid");
    Check(assets.rowFrameCounts.size() <= static_cast<std::size_t>(assets.rows), "Too many row frame counts");
    for (int count : assets.rowFrameCounts) Check(count >= 1 && count <= assets.columns, "Frame count must be 1..columns");
    Check(assets.idleRow >= 0 && assets.idleRow < assets.rows, "Animation row outside sprite grid");
    Number(assets.fps, 0.1F, 60, "fps"); Number(assets.height, 24, 300, "height");
    Check(assets.animationMode == "auto" || assets.animationMode == "procedural" ||
          assets.animationMode == "frames" || assets.animationMode == "rig", "Unknown animationMode");
    Number(assets.motionStrength, 0, 2, "motionStrength");
    for (int row : {assets.runRow, assets.attackRow, assets.jumpRow, assets.fallRow, assets.dodgeRow, assets.hurtRow, assets.defeatedRow})
        Check(row >= -1 && row < assets.rows, "Animation row outside sprite grid");
    if (!assets.parts.empty() || assets.animationMode == "rig") Check(assets.parts.count("torso"), "A rig needs a torso part");
    if (assets.parts.count("weapon")) Check(assets.parts.count("armFront"), "Weapon needs armFront as its parent");
    for (const auto& [slot, p] : assets.parts) {
        (void)DefaultCharacterPart(slot); Asset(p.image); Check(!p.image.empty(), "Body part image cannot be empty");
        Number(p.x, -2, 2, "part x"); Number(p.y, -2, 2, "part y"); Number(p.height, 0.01F, 2, "part height");
        Number(p.pivotX, 0, 1, "part pivotX"); Number(p.pivotY, 0, 1, "part pivotY");
    }
    Check(stats.health >= 1 && stats.health <= 10000, "health must be 1..10000");
    Check(stats.jumps >= 1 && stats.jumps <= 10, "jumps must be 1..10");
    Number(stats.moveSpeed, 1, 1200, "moveSpeed"); Number(stats.jumpSpeed, 1, 1500, "jumpSpeed");
    Number(stats.attackInterval, 0.03F, 10, "attackInterval"); ValidateEffect(attack);
    Check(attack.type != EffectType::Buff && attack.type != EffectType::Heal, "Basic attack must be projectile, melee or rain");
    Check(skills.size() <= 2, "Only E/Q skill slots are supported");
    for (const auto& s : skills) {
        Check(!s.name.empty() && s.name.size() <= 120 && s.name.find('\0') == std::string::npos, "Invalid skill name");
        Check(s.description.size() <= 2048 && s.description.find('\0') == std::string::npos, "Invalid skill description"); Asset(s.icon);
        Number(s.cooldown, 0.05F, 3600, "cooldown"); Number(s.duration, 0, 120, "duration");
        Number(s.interval, 0, 120, "interval");
        Check(s.interval == 0 || (s.interval >= 0.05F && s.duration > 0), "Repeated effects need duration > 0 and interval >= 0.05");
        Check(!s.effects.empty() && s.effects.size() <= 8, "A skill needs 1..8 effects");
        for (const auto& e : s.effects) {
            ValidateEffect(e);
            Check(e.type != EffectType::Buff || s.duration > 0, "Buff needs duration > 0");
        }
    }
}

std::string Character::ToJson() const {
    Validate();
    Json j{{"schemaVersion", 1}, {"id", id}, {"name", name}, {"description", description},
           {"assets", {{"portrait", assets.portrait}, {"sprite", assets.sprite}, {"columns", assets.columns},
                        {"rows", assets.rows}, {"idleRow", assets.idleRow}, {"runRow", assets.runRow},
                        {"attackRow", assets.attackRow}, {"defeatedRow", assets.defeatedRow}, {"fps", assets.fps}, {"height", assets.height}}},
           {"stats", {{"health", stats.health}, {"moveSpeed", stats.moveSpeed}, {"jumpSpeed", stats.jumpSpeed},
                       {"jumps", stats.jumps}, {"attackInterval", stats.attackInterval}}},
           {"attack", EffectJson(attack)}, {"skills", Json::array()}};
    j["assets"]["animationMode"] = assets.animationMode;
    j["assets"]["rowFrameCounts"] = assets.rowFrameCounts;
    j["assets"]["motionStrength"] = assets.motionStrength;
    j["assets"]["jumpRow"] = assets.jumpRow; j["assets"]["fallRow"] = assets.fallRow;
    j["assets"]["dodgeRow"] = assets.dodgeRow; j["assets"]["hurtRow"] = assets.hurtRow;
    j["assets"]["parts"] = Json::object();
    for (const auto& [slot, p] : assets.parts)
        j["assets"]["parts"][slot] = {{"image", p.image}, {"x", p.x}, {"y", p.y}, {"height", p.height}, {"pivotX", p.pivotX}, {"pivotY", p.pivotY}};
    for (const auto& s : skills) {
        Json effects = Json::array(); for (const auto& e : s.effects) effects.push_back(EffectJson(e));
        j["skills"].push_back({{"name", s.name}, {"description", s.description}, {"icon", s.icon},
                              {"cooldown", s.cooldown}, {"duration", s.duration}, {"interval", s.interval}, {"effects", effects}});
    }
    return j.dump(2, ' ', false, Json::error_handler_t::strict);
}

CharacterRuntime::CharacterRuntime(Character character) : character_(std::move(character)) { character_.Validate(); }

void CharacterRuntime::Apply(const SkillEffect& e, Vector2 pos, int facing, Vector2 target,
                             float multiplier, int& health, std::vector<Bullet>& bullets) {
    if (e.type == EffectType::Heal) { health = std::min(character_.stats.health, health + static_cast<int>(e.amount)); return; }
    if (e.type == EffectType::Buff) return;
    for (int i = 0; i < e.count && bullets.size() < 4096; ++i) {
        const float offset = static_cast<float>(i) - static_cast<float>(e.count - 1) / 2;
        Bullet b;
        b.position = {pos.x + facing * 48.0F, pos.y - 16};
        b.velocity = {facing * std::cos(offset * e.spread) * e.speed, std::sin(offset * e.spread) * e.speed};
        b.lifetime = e.lifetime; b.radius = e.radius; b.damage = e.amount * multiplier;
        b.damageType = e.damageType; b.stunDuration = e.stun; b.destroysEnemyProjectile = e.destroysProjectiles;
        if (e.type == EffectType::Melee) {
            b.kind = BulletKind::MeleeSlash; b.velocity = {};
            b.position = {pos.x + facing * e.range / 2, pos.y - 8};
            b.radius = e.range / 2;
            b.facingDirection = facing;
        } else if (e.type == EffectType::Rain) {
            b.kind = BulletKind::FallingSword;
            b.position = {std::clamp(target.x + offset * e.range / e.count, GameConfig::kRoom.x + 25, GameConfig::kRoom.x + GameConfig::kRoom.width - 25),
                          GameConfig::kRoom.y + 18};
            b.velocity = {0, e.speed};
        }
        bullets.push_back(b);
    }
}

void CharacterRuntime::Update(float dt, Vector2 pos, int facing, Vector2 target, bool attacking,
                              std::array<bool, 2> activated, int& health, std::vector<Bullet>& bullets) {
    performedAction_ = false;
    performedAttack_ = false;
    if (!std::isfinite(dt) || dt < 0 || health <= 0) return;
    // Bound catch-up work on stalls. Normal frames retain their full delta.
    dt = std::min(dt, 0.25F);
    float damage = 1, speed = 1;
    for (std::size_t i = 0; i < character_.skills.size(); ++i) {
        const auto& skill = character_.skills[i]; auto& state = skills_[i];
        state.cooldown = std::max(0.0F, state.cooldown - dt);
        const float activeStep = std::min(dt, state.remaining);
        if (state.remaining > 0 && skill.interval > 0) {
            while (state.nextPulse <= activeStep && state.nextPulse < state.remaining) {
                for (const auto& e : skill.effects) Apply(e, pos, facing, target, 1, health, bullets);
                state.nextPulse += skill.interval;
            }
            state.nextPulse -= activeStep;
        }
        state.remaining = std::max(0.0F, state.remaining - dt);
        if (activated[i] && state.cooldown <= 0 && state.remaining <= 0) {
            performedAction_ = true;
            state.cooldown = skill.cooldown; state.remaining = skill.duration; state.nextPulse = skill.interval;
            for (const auto& e : skill.effects) Apply(e, pos, facing, target, 1, health, bullets);
        }
        if (state.remaining > 0) for (const auto& e : skill.effects) if (e.type == EffectType::Buff) {
            damage *= e.damageMultiplier; speed *= e.attackSpeedMultiplier;
        }
    }
    attackCooldown_ = std::max(0.0F, attackCooldown_ - dt);
    if (attacking && attackCooldown_ <= 0) {
        performedAction_ = true;
        performedAttack_ = true;
        Apply(character_.attack, pos, facing, target, std::min(damage, 100.0F), health, bullets);
        attackCooldown_ = std::max(0.03F, character_.stats.attackInterval / std::min(speed, 10.0F));
    }
}
