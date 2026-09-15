#include "character_repository.h"
#include "character_image.h"
#include "file_path.h"
#include "character_animation.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <chrono>

namespace fs = std::filesystem;
namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class F> void Throws(F fn) {
    bool threw = false; try { fn(); } catch (const std::exception&) { threw = true; }
    Require(threw, "Expected an exception");
}
Character Example() {
    return Character::FromJson(R"({"id":"test_character","name":"测试角色","stats":{"health":8},
        "skills":[{"name":"组合技能","cooldown":2,"duration":0.5,"effects":[
        {"type":"heal","amount":2},{"type":"buff","damageMultiplier":3,"attackSpeedMultiplier":2}]}]})");
}
void Serialization() {
    auto c = Example();
    Require(Character::FromJson(c.ToJson()).ToJson() == c.ToJson(), "JSON round trip lost fields");
    Throws([] { (void)Character::FromJson(R"({"id":"../oops","name":"x"})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","schemaVersion":2})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","stats":{"health":2.2}})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","stats":{"health":0}})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","stats":{"health":4294967297}})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","attack":{"type":"typo"}})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","unknown":1})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","assets":{"rows":1,"attackRow":2}})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","skills":{}})"); });
    c.stats.moveSpeed = std::numeric_limits<float>::quiet_NaN(); Throws([&] { (void)c.ToJson(); });
    c = Example(); c.skills[0].interval = 0.001F; Throws([&] { c.Validate(); });
    c = Example(); c.skills[0].duration = 0; Throws([&] { c.Validate(); });
    c = Example(); c.skills.resize(3); Throws([&] { c.Validate(); });
    auto rig = Character::FromJson(R"({"id":"rig","name":"Rig","assets":{"parts":{"torso":{"image":"body.png"},"armFront":{"image":"arm.png"},"weapon":{"image":"sword.png"}}}})");
    Require(rig.assets.parts.at("armFront").y == -0.61F && rig.assets.parts.at("weapon").pivotY == 0.85F, "Default rig joints missing");
    Require(Character::FromJson(rig.ToJson()).ToJson() == rig.ToJson(), "Rig round trip failed");
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","assets":{"parts":{"hand":{"image":"x.png"}}}})"); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","assets":{"animationMode":"rig"}})"); });
    rig.assets.parts["torso"].pivotX = 1.2F; Throws([&] { rig.Validate(); });
}
void Animation() {
    CharacterAnimator animator, separate;
    animator.Update(0.1F, {{320, 0}, true});
    Require(animator.Action() == CharacterAction::Run && animator.Pose().legFront * animator.Pose().legBack < 0, "Legs do not alternate");
    Require(separate.Action() == CharacterAction::Idle && separate.Pose().legFront == 0, "Animator state shared");
    animator.Update(0.016F, {{320, -400}, false, false, true, false, true});
    Require(animator.Action() == CharacterAction::Jump, "Air attack lost airborne state");
    animator.Update(0.016F, {{0, 300}, false}); Require(animator.Action() == CharacterAction::Fall, "Fall state missing");
    animator.Update(0.016F, {{}, true}); Require(animator.Action() == CharacterAction::Land, "Landing event missing");
    animator.Update(0.016F, {{}, true, true}); Require(animator.Action() == CharacterAction::Dodge, "Dodge priority failed");
    animator.TriggerHurt(); animator.Update(0.016F, {{}, true, true}); Require(animator.Action() == CharacterAction::Hurt, "Hurt priority failed");
    for (int i = 0; i < 70; ++i) animator.Update(1.0F / 60, {{}, true, false, false, true});
    Require(animator.Action() == CharacterAction::Defeated && animator.Pose().angle > 80, "Defeat did not settle");
    const float settled = animator.Pose().angle;
    animator.Update(0.1F, {{}, true, false, false, true}); Require(std::abs(animator.Pose().angle - settled) < 0.1F, "Defeat loops");
    CharacterAssets assets;
    Require(animator.Row(assets) == -1, "Missing defeat animation did not use fallback");
    assets.defeatedRow = 3; Require(animator.Row(assets) == 3, "Explicit row ignored");
    animator.Update(-1, {}); Require(animator.Action() == CharacterAction::Defeated, "Negative dt mutated animator");
    auto c = Example(); CharacterRuntime combat(c); int health = 8; std::vector<Bullet> bullets;
    combat.Update(0, {}, 1, {}, true, {}, health, bullets); Require(combat.PerformedAction(), "Attack event missing");
    combat.Update(0, {}, 1, {}, true, {}, health, bullets); Require(!combat.PerformedAction(), "Holding attack retriggered animation during cooldown");
}
void SpriteFrames() {
    CharacterAssets assets; assets.columns = 4; assets.rows = 2;
    Image image = GenImageColor(32, 16, BLANK);
    ImageDrawRectangle(&image, 0, 0, 8, 8, RED);
    ImageDrawRectangle(&image, 16, 0, 8, 8, WHITE); // White artwork is valid; only alpha-empty cells are skipped.
    auto frames = FindCharacterFrames(image, assets);
    Require(frames[0] == std::vector<int>({0, 2}) && frames[1].empty(), "Empty frames not filtered");
    for (int i = 0; i < 100; ++i) {
        const auto frame = SampleCharacterFrame(frames, 0, 0, 8, i / 8.0F);
        Require(frame.valid && frame.column == (i % 2 == 0 ? 0 : 2), "Loop selected a transparent frame");
    }
    Require(SampleCharacterFrame(frames, 1, 0, 8, 0).row == 0, "Empty row did not fall back to idle");
    Require(SampleCharacterFrame(frames, 0, 0, 8, 100, false).column == 2, "Non-looping animation wrapped");
    assets.rowFrameCounts = {1}; frames = FindCharacterFrames(image, assets);
    Require(frames[0] == std::vector<int>({0}), "Explicit frame limit ignored");
    UnloadImage(image);
    auto c = Example(); c.assets.columns = 8; c.assets.rows = 2; c.assets.rowFrameCounts = {7, 4};
    Require(Character::FromJson(c.ToJson()).assets.rowFrameCounts == c.assets.rowFrameCounts, "Frame limits not persisted");
    c.assets.rowFrameCounts = {9}; Throws([&] { c.Validate(); });
    Throws([] { (void)Character::FromJson(R"({"id":"x","name":"x","assets":{"rowFrameCounts":[1.5]}})"); });
}
void Runtime() {
    const auto c = Example(); CharacterRuntime a(c), b(c);
    int healthA = 2, healthB = 2; std::vector<Bullet> bullets;
    a.Update(0, {200, 500}, 1, {400, 500}, true, {true, false}, healthA, bullets);
    Require(healthA == 4 && bullets.size() == 1 && bullets[0].damage == 3, "Combined healing/buff failed");
    b.Update(0, {200, 500}, -1, {400, 500}, true, {}, healthB, bullets);
    Require(healthB == 2 && bullets.back().damage == 1 && bullets.back().velocity.x < 0, "Shared state or facing bug");
    a.Update(0.1F, {}, 1, {}, false, {true, false}, healthA, bullets);
    Require(healthA == 4, "Cooldown allowed a second activation");
    for (int i = 0; i < 20; ++i) a.Update(0.1F, {}, 1, {}, false, {}, healthA, bullets);
    a.Update(0, {}, 1, {}, true, {}, healthA, bullets);
    Require(bullets.back().damage == 1, "Buff did not expire");
    a.Update(0, {}, 1, {}, false, {true, false}, healthA, bullets);
    Require(healthA == 6, "Cooldown did not expire");
    const auto size = bullets.size(); healthA = 0;
    a.Update(1, {}, 1, {}, true, {true, true}, healthA, bullets);
    Require(bullets.size() == size && healthA == 0, "Dead character attacked or revived");

    auto rain = c; rain.skills.clear(); rain.attack.type = EffectType::Rain;
    rain.attack.count = 3; rain.attack.damageType = DamageType::Arts; rain.attack.stun = 0.7F;
    CharacterRuntime r(rain); bullets.clear(); healthA = 8;
    r.Update(0, {}, 1, {600, 500}, true, {}, healthA, bullets);
    Require(bullets.size() == 3 && bullets[0].kind == BulletKind::FallingSword && bullets[0].damageType == DamageType::Arts && bullets[0].stunDuration == 0.7F, "Rain payload failed");
    rain.attack.type = EffectType::Melee; rain.attack.count = 1; rain.attack.range = 180;
    CharacterRuntime melee(rain); bullets.clear();
    melee.Update(0, {200, 500}, -1, {}, true, {}, healthA, bullets);
    Require(bullets[0].position.x == 110 && bullets[0].radius == 90 && bullets[0].velocity.x == 0, "Melee range or facing failed");

    auto repeat = Character::FromJson(R"({"id":"repeat","name":"Repeat","skills":[{"name":"Pulse","duration":1,"interval":0.25,"cooldown":2,"effects":[{"count":2}]}]})");
    CharacterRuntime pulse(repeat); bullets.clear();
    pulse.Update(0, {}, 1, {}, false, {true, false}, healthA, bullets);
    for (int i = 0; i < 4; ++i) pulse.Update(0.25F, {}, 1, {}, false, {}, healthA, bullets);
    Require(bullets.size() == 8, "Pulse boundary produced duplicate/missing effects");
}
void Repository(const fs::path& root) {
    LocalCharacterRepository repo(root / "store");
    Require(repo.LoadAll().characters.empty(), "Missing repository must be empty");
    auto c = Example(); repo.Save(c); c.name = "Updated"; repo.Save(c);
    auto read = repo.LoadAll(); Require(read.characters.size() == 1 && read.characters[0].name == "Updated", "Upsert failed");
    { std::ofstream out(root / "store" / "bad.json"); out << "{"; }
    read = repo.LoadAll(); Require(read.characters.size() == 1 && read.errors.size() == 1, "Bad record hid valid character");
    auto invalid = c; invalid.id = "../escape"; Throws([&] { repo.Save(invalid); });
    fs::create_directories(root / "input");
    c.assets.sprite = "sprite.png";
    { std::ofstream out(root / "input" / "character.json"); out << c.ToJson(); }
    Throws([&] { repo.Import(root / "input" / "character.json"); });
    Require(repo.LoadAll().characters[0].assets.sprite.empty(), "Failed import overwrote saved character");
    Image image = GenImageColor(8, 8, WHITE);
    Require(ExportImage(image, (root / "input" / "sprite.png").string().c_str()), "Test image write failed"); UnloadImage(image);
    const auto imported = repo.Import(root / "input" / "character.json");
    Require(fs::exists(repo.Root() / imported.assets.sprite), "Imported artwork was not copied");
    const auto unicodeInput = root / Utf8Path("中文角色目录"); fs::create_directories(unicodeInput);
    fs::copy_file(root / "input" / "sprite.png", unicodeInput / Utf8Path("小人.png"));
    c.assets.sprite = "小人.png";
    { std::ofstream out(unicodeInput / Utf8Path("角色.json")); out << c.ToJson(); }
    LocalCharacterRepository unicodeRepo(root / Utf8Path("中文角色库"));
    const auto unicodeCharacter = unicodeRepo.Import(unicodeInput / Utf8Path("角色.json"));
    Image unicodeImage = LoadCharacterImage(unicodeRepo.Root() / unicodeCharacter.assets.sprite);
    Require(unicodeImage.data != nullptr, "Cannot load imported image from Unicode path"); UnloadImage(unicodeImage);
    c.assets.parts["torso"] = DefaultCharacterPart("torso"); c.assets.parts["torso"].image = "小人.png";
    { std::ofstream out(unicodeInput / "rig.json"); out << c.ToJson(); }
    const auto importedRig = unicodeRepo.Import(unicodeInput / "rig.json");
    Require(fs::exists(unicodeRepo.Root() / importedRig.assets.parts.at("torso").image), "Rig part not copied into repository");
    c.assets.parts.clear();
    fs::remove(root / "input" / "sprite.png");
    Require(fs::exists(repo.Root() / imported.assets.sprite), "Imported artwork depended on original file");
    c.assets.sprite = "broken.png";
    { std::ofstream out(root / "input" / "broken.png"); out << "not a PNG"; }
    { std::ofstream out(root / "input" / "character.json"); out << c.ToJson(); }
    Throws([&] { repo.Import(root / "input" / "character.json"); });
}
void Cloud(const fs::path& root) {
    auto c = Example();
    CloudCharacterRepository missing({}); Throws([&] { missing.Save(c); }); Throws([&] { missing.LoadAll(); });
    std::string stored; int uploads = 0;
    CharacterCloudApi api;
    api.putCharacter = [&](const std::string& id, const std::string& json) { Require(id == c.id, "Cloud key mismatch"); stored = json; };
    api.listCharacters = [&] { return std::vector<std::string>{stored, "{"}; };
    api.uploadAsset = [&](const fs::path&) { ++uploads; return "https://assets.example.test/a.png"; };
    CloudCharacterRepository cloud(api, root);
    cloud.Save(c); Require(Character::FromJson(stored).id == c.id, "Cloud JSON invalid");
    auto list = cloud.LoadAll(); Require(list.characters.size() == 1 && list.errors.size() == 1, "Cloud record isolation failed");
    c.assets.sprite = "absent.png"; Throws([&] { cloud.Save(c); }); Require(uploads == 0, "Uploaded before preflight");
    { std::ofstream out(root / "cloud.png"); out << "upload test payload"; }
    c.assets.sprite = "cloud.png"; cloud.Save(c);
    Require(uploads == 1 && Character::FromJson(stored).assets.sprite.starts_with("https://"), "Cloud asset URI not persisted");
    Require(c.assets.sprite == "cloud.png", "Cloud save modified local character");
    c.assets.parts["torso"] = DefaultCharacterPart("torso"); c.assets.parts["torso"].image = "cloud.png";
    cloud.Save(c);
    Require(Character::FromJson(stored).assets.parts.at("torso").image.starts_with("https://"), "Rig part not uploaded");
    api.putCharacter = [](const std::string&, const std::string&) { throw std::runtime_error("HTTP 503"); };
    CloudCharacterRepository failing(api, root); Throws([&] { failing.Save(c); });
    api.uploadAsset = [](const fs::path&) { return std::string{}; };
    CloudCharacterRepository badUpload(api, root); Throws([&] { badUpload.Save(c); });
}
}
int main() {
    const auto parent = fs::temp_directory_path();
    const auto root = parent / ("my_game_character_tests_" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    try {
        fs::create_directories(root);
        Serialization(); Animation(); SpriteFrames(); Runtime(); Repository(root); Cloud(root);
        Require(fs::equivalent(root.parent_path(), parent) && root.filename().string().starts_with("my_game_character_tests_"), "Unsafe test cleanup");
        fs::remove_all(root);
        std::cout << "Character serialization, validation, combat, import and cloud contract tests passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << "\nTest files: " << root << '\n'; return 1; }
}
