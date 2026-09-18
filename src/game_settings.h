#pragma once

#include <array>
#include <filesystem>
#include <string>

enum class GameAction { MoveLeft, MoveRight, Jump, Dodge, Attack, Reload,
                        SkillOne, SkillTwo, OperatorOne, OperatorTwo, Pause, Count };

class GameSettings {
public:
    explicit GameSettings(std::filesystem::path applicationDirectory);
    static constexpr int kActionCount = static_cast<int>(GameAction::Count);
    static const char* ActionName(GameAction action);
    static std::string KeyName(int key);
    int Key(GameAction action) const { return keys_[static_cast<int>(action)]; }
    bool Pressed(GameAction action) const;
    bool Down(GameAction action) const;
    bool Bind(GameAction action, int key);
    int MasterVolume() const { return masterVolume_; }
    int SoundVolume() const { return soundVolume_; }
    void AdjustMaster(int delta);
    void AdjustSound(int delta);
    bool Save();
    bool Load();
    const std::string& Error() const { return error_; }

private:
    static bool AllowedKey(int key);
    bool LegacyPressed(GameAction action) const;
    bool LegacyDown(GameAction action) const;
    std::filesystem::path savePath_;
    std::array<int, kActionCount> keys_{};
    int masterVolume_ = 100;
    int soundVolume_ = 100;
    bool defaultKeymap_ = true;
    std::string error_;
};
