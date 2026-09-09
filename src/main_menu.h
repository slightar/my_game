#pragma once
#include "game_types.h"
#include "raylib.h"

class UiFont;
class CharacterArt;

enum class MenuAction {
    None,
    StartBattle,
    Quit
};

class MainMenu {
public:
    MenuAction Update();
    void Draw(const UiFont& font, const CharacterArt& art) const;
    void OpenHome();

    [[nodiscard]] const char* SelectedOperatorName() const;
    [[nodiscard]] OperatorKind SelectedOperator() const;

private:
    enum class Page {
        Splash,
        Home,
        StageSelect,
        OperatorSelect
    };

    void DrawBackground(const UiFont& font, const char* section) const;
    void DrawSplash(const UiFont& font) const;
    void DrawHome(const UiFont& font, const CharacterArt& art) const;
    void DrawStageSelect(const UiFont& font) const;
    void DrawOperatorSelect(const UiFont& font, const CharacterArt& art) const;

    Page page_ = Page::Splash;
    int homeSelection_ = 0;
    int operatorCursor_ = 0;
    OperatorKind selectedOperator_ = OperatorKind::Exusiai;
};
