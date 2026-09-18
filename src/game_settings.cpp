#include "game_settings.h"
#include "raylib.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace {
constexpr std::array<int, GameSettings::kActionCount> kDefaultKeys = {
    KEY_A, KEY_D, KEY_W, KEY_S, KEY_J, KEY_R, KEY_Q, KEY_E, KEY_ONE, KEY_TWO, KEY_ESCAPE};
constexpr std::array<const char*, GameSettings::kActionCount> kIds = {
    "moveLeft", "moveRight", "jump", "dodge", "attack", "reload", "skillOne", "skillTwo", "operatorOne", "operatorTwo", "pause"};
constexpr std::array<const char*, GameSettings::kActionCount> kNames = {
    "左移", "右移", "跳跃", "闪避", "攻击", "装填", "技能一", "技能二", "切换一号", "切换二号", "暂停"};
bool Alternate(GameAction action, bool pressed) {
    const auto check = [pressed](int key) { return pressed ? IsKeyPressed(key) : IsKeyDown(key); };
    switch (action) {
    case GameAction::MoveLeft: return check(KEY_LEFT);
    case GameAction::MoveRight: return check(KEY_RIGHT);
    case GameAction::Jump: return check(KEY_K) || check(KEY_SPACE) || check(KEY_UP);
    case GameAction::Dodge: return check(KEY_L) || check(KEY_LEFT_SHIFT) || check(KEY_RIGHT_SHIFT);
    default: return false;
    }
}
}

GameSettings::GameSettings(std::filesystem::path applicationDirectory)
    : savePath_(std::move(applicationDirectory) / "save" / "game_settings.json"), keys_(kDefaultKeys) { Load(); }

const char* GameSettings::ActionName(GameAction action) { return kNames[static_cast<int>(action)]; }
std::string GameSettings::KeyName(int key) {
    if (key >= KEY_A && key <= KEY_Z) return std::string(1, static_cast<char>('A' + key - KEY_A));
    if (key >= KEY_ZERO && key <= KEY_NINE) return std::string(1, static_cast<char>('0' + key - KEY_ZERO));
    switch (key) {
    case KEY_UP: return "↑"; case KEY_DOWN: return "↓"; case KEY_LEFT: return "←"; case KEY_RIGHT: return "→";
    case KEY_SPACE: return "Space"; case KEY_LEFT_SHIFT: return "L Shift"; case KEY_RIGHT_SHIFT: return "R Shift";
    case KEY_ESCAPE: return "Esc"; case KEY_TAB: return "Tab";
    default: return "?";
    }
}
bool GameSettings::AllowedKey(int key) {
    return (key >= KEY_A && key <= KEY_Z) || (key >= KEY_ZERO && key <= KEY_NINE) ||
           key == KEY_UP || key == KEY_DOWN || key == KEY_LEFT || key == KEY_RIGHT ||
           key == KEY_SPACE || key == KEY_LEFT_SHIFT || key == KEY_RIGHT_SHIFT || key == KEY_TAB;
}
bool GameSettings::LegacyPressed(GameAction action) const { return defaultKeymap_ && Alternate(action, true); }
bool GameSettings::LegacyDown(GameAction action) const { return defaultKeymap_ && Alternate(action, false); }
bool GameSettings::Pressed(GameAction action) const { return IsKeyPressed(Key(action)) || LegacyPressed(action); }
bool GameSettings::Down(GameAction action) const { return IsKeyDown(Key(action)) || LegacyDown(action); }
bool GameSettings::Bind(GameAction action, int key) {
    const int index = static_cast<int>(action);
    if (index < 0 || index >= kActionCount || (!AllowedKey(key) && !(action == GameAction::Pause && key == KEY_ESCAPE))) return false;
    for (int i = 0; i < kActionCount; ++i) if (i != index && keys_[i] == key) return false;
    keys_[index] = key;
    defaultKeymap_ = keys_ == kDefaultKeys;
    Save(); return true;
}
void GameSettings::AdjustMaster(int delta) { masterVolume_ = std::clamp(masterVolume_ + delta, 0, 100); Save(); }
void GameSettings::AdjustSound(int delta) { soundVolume_ = std::clamp(soundVolume_ + delta, 0, 100); Save(); }
bool GameSettings::Save() {
    try {
        std::filesystem::create_directories(savePath_.parent_path());
        nlohmann::json bindings = nlohmann::json::object();
        for (int i = 0; i < kActionCount; ++i) bindings[kIds[i]] = keys_[i];
        const nlohmann::json data = {{"version", 1}, {"masterVolume", masterVolume_},
                                     {"soundVolume", soundVolume_}, {"bindings", bindings}};
        const auto temporary = savePath_.string() + ".tmp";
        { std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
          if (!out) throw std::runtime_error("无法写入设置"); out << data.dump(2);
          if (!out) throw std::runtime_error("设置写入中断"); }
        std::filesystem::copy_file(temporary, savePath_, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(temporary); error_.clear(); return true;
    } catch (const std::exception& e) { error_ = std::string("设置保存失败: ") + e.what(); return false; }
}
bool GameSettings::Load() {
    try {
        if (!std::filesystem::exists(savePath_)) return true;
        std::ifstream in(savePath_, std::ios::binary);
        if (!in) throw std::runtime_error("无法读取设置");
        const auto data = nlohmann::json::parse(in);
        if (data.at("version").get<int>() != 1) throw std::runtime_error("设置版本不兼容");
        const int master = data.at("masterVolume").get<int>();
        const int sound = data.at("soundVolume").get<int>();
        if (master < 0 || master > 100 || sound < 0 || sound > 100) throw std::runtime_error("音量无效");
        std::array<int, kActionCount> keys;
        for (int i = 0; i < kActionCount; ++i) {
            keys[i] = data.at("bindings").at(kIds[i]).get<int>();
            if (!AllowedKey(keys[i]) && !(i == static_cast<int>(GameAction::Pause) && keys[i] == KEY_ESCAPE))
                throw std::runtime_error("键位无效");
            for (int j = 0; j < i; ++j) if (keys[i] == keys[j]) throw std::runtime_error("键位重复");
        }
        masterVolume_ = master; soundVolume_ = sound; keys_ = keys;
        defaultKeymap_ = keys_ == kDefaultKeys; error_.clear(); return true;
    } catch (const std::exception& e) {
        keys_ = kDefaultKeys; masterVolume_ = soundVolume_ = 100; defaultKeymap_ = true;
        error_ = std::string("设置读取失败，已使用默认值: ") + e.what(); return false;
    }
}
