#include "game_settings.h"
#include "raylib.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("my_game_settings_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    {
        GameSettings settings(root);
        assert(settings.MasterVolume() == 100 && settings.SoundVolume() == 100);
        settings.AdjustMaster(-25); settings.AdjustSound(-40);
        assert(settings.Bind(GameAction::Attack, KEY_F));
        assert(!settings.Bind(GameAction::Jump, KEY_F));
        assert(!settings.Bind(GameAction::Jump, KEY_F5));
    }
    {
        GameSettings restored(root);
        assert(restored.MasterVolume() == 75 && restored.SoundVolume() == 60);
        assert(restored.Key(GameAction::Attack) == KEY_F);
        std::ofstream(root / "save" / "game_settings.json") << "{invalid";
        assert(!restored.Load());
        assert(restored.MasterVolume() == 100 && restored.Key(GameAction::Attack) == KEY_J);
    }
    std::filesystem::remove(root / "save" / "game_settings.json");
    std::filesystem::remove(root / "save");
    std::filesystem::remove(root);
}
