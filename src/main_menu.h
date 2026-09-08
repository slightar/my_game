#pragma once
#include "raylib.h"

class UiFont;

enum class MenuAction {
    None,
    StartBattle
};

class MainMenu {
public:
    MenuAction Update();
    void Draw(const UiFont& font) const;
    void OpenHome();

    [[nodiscard]] const char* SelectedOperatorName() const;

private:
    enum class Page {
        Splash,
        Home,
        StageSelect,
        OperatorSelect
    };

    void DrawBackground(const UiFont& font, const char* section) const;
    void DrawSplash(const UiFont& font) const;
    void DrawHome(const UiFont& font) const;
    void DrawStageSelect(const UiFont& font) const;
    void DrawOperatorSelect(const UiFont& font) const;

    Page page_ = Page::Splash;
    int homeSelection_ = 0;
    int operatorCursor_ = 0;
};
