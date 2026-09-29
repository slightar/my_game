#pragma once
#include "progress_system.h"
#include "game_settings.h"
#include "raylib.h"
#include "ui_input.h"
#include "action_map.h"
#include "character.h"
#include "game_types.h"
#include <array>
#include <string>
#include <vector>

class UiFont;
class CharacterArt;

enum class MenuAction { None, StartBattle, StartEnemyTrial, Quit };

class MainMenu {
public:
    MainMenu();
    explicit MainMenu(const std::filesystem::path& applicationDirectory);
    MenuAction Update(float deltaTime = 1.0F / 60.0F);
    void Draw(const UiFont& font, const CharacterArt& art) const;
    void OpenHome();
    void OpenStageSelect();
    void OpenArchive() { page_ = Page::Archive; }
    void OpenEquipment() { page_ = Page::Equipment; }
    void OpenSettings() { page_ = Page::Settings; }
    void OpenCharacterSelect();
    void SetCustomCharacters(std::vector<Character> characters);
    [[nodiscard]] int SelectedCharacterIndex(int slot) const;
    [[nodiscard]] const Character* CharacterForSelection(int selection) const;
    [[nodiscard]] std::string CharacterNameForSelection(int selection) const;
    void SetStatus(std::string status) { if (!status.empty()) status_ = std::move(status); }
    const GameSettings& Settings() const { return settings_; }
    bool IsProtocolSuppressed() const { return progress_.IsProtocolSuppressed(); }

private:
    enum class Page {
        Splash,
        Home, CharacterSelect,
        Map, Archive, Equipment, Settings, KeyBindings, ConfirmReset,
        NodeDetail, FirstReset, HiddenBoss, Ending
    };

    void DrawBackground(const UiFont& font, const char* section) const;
    void DrawTransition(const UiFont& font) const;
    void DrawHome(const UiFont& font, const CharacterArt& art) const;
    void DrawCharacterSelect(const UiFont& font, const CharacterArt& art) const;
    void DrawMap(const UiFont& font) const;
    void UpdateMap(float deltaTime);
    void DrawArchive(const UiFont& font) const;
    void DrawEquipment(const UiFont& font) const;
    void DrawSettings(const UiFont& font) const;
    void DrawKeyBindings(const UiFont& font) const;
    void DrawNodeDetail(const UiFont& font) const;
    void DrawEnding(const UiFont& font) const;
    void MoveExplorer(float deltaTime);
    const MapExit* NearbyExit() const;
    void SyncError();

    ProgressSystem progress_;
    GameSettings settings_;
    Page page_ = Page::Splash;
    Page returnPage_ = Page::Map;
    int homeSelection_ = 0;
    int characterSelection_ = 0;
    int characterSlot_ = 0;
    std::array<int, 2> characterLineup_{0, 1};
    std::array<int, 2> draftCharacterLineup_{0, 1};
    std::vector<Character> customCharacters_;
    int archiveCategory_ = 0, archiveSelection_ = 0;
    int equipmentSlot_ = 0, equipmentSelection_ = 0;
    int settingsSelection_ = 0, keySelection_ = 0;
    std::string mapPreview_;
    float mapScroll_ = 0;
    ActionMap::DragGesture mapDrag_;
    bool mapStartReady_ = false;
    bool capturingKey_ = false;
    Vector2 explorer_{640.0F, 390.0F};
    Vector2 walkTarget_{640.0F, 390.0F};
    bool walkingToPointer_ = false;
    std::string pendingExit_;
    UiPointer pointer_{};
    UiPointerState pointerState_;
    EndingType ending_ = EndingType::Normal;
    std::string status_;
};
