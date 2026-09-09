#include "main_menu.h"

#include "character_art.h"
#include "game_types.h"
#include "ui_font.h"

#include <algorithm>

namespace {

constexpr Color kBlue{0, 156, 211, 255};
constexpr Color kOrange{255, 91, 18, 255};
constexpr Color kInk{24, 28, 32, 255};
constexpr Color kPaper{238, 239, 236, 255};

void DrawTechLines() {
    for (int x = -180; x < GameConfig::kScreenWidth + 240; x += 190) {
        DrawLineEx({static_cast<float>(x), 0.0F},
                   {static_cast<float>(x + 460),
                    static_cast<float>(GameConfig::kScreenHeight)},
                   2.0F, Fade(RAYWHITE, 0.055F));
    }
    for (int y = 100; y < GameConfig::kScreenHeight; y += 135) {
        DrawLine(0, y, GameConfig::kScreenWidth, y, Fade(RAYWHITE, 0.035F));
    }
}

void DrawMenuCard(const UiFont& font, Rectangle bounds, const char* title,
                  const char* subtitle, bool selected, bool dark = false) {
    const Color fill = dark ? Color{42, 47, 52, 245} : Color{239, 240, 237, 245};
    const Color text = dark ? RAYWHITE : kInk;
    DrawRectangleRec(bounds, fill);
    DrawTriangle({bounds.x, bounds.y},
                 {bounds.x + 48.0F, bounds.y},
                 {bounds.x, bounds.y + 48.0F},
                 selected ? kOrange : Fade(kBlue, 0.55F));
    DrawRectangle(static_cast<int>(bounds.x),
                  static_cast<int>(bounds.y + bounds.height - 7.0F),
                  static_cast<int>(bounds.width), 7,
                  selected ? kOrange : Fade(BLACK, 0.18F));
    if (selected) {
        DrawRectangleLinesEx(bounds, 4.0F, kBlue);
        DrawRectangle(static_cast<int>(bounds.x - 14.0F),
                      static_cast<int>(bounds.y), 8,
                      static_cast<int>(bounds.height), kOrange);
    }
    font.Draw(title, bounds.x + 38.0F, bounds.y + 35.0F, 52.0F, text);
    font.Draw(subtitle, bounds.x + 40.0F, bounds.y + 105.0F, 22.0F,
              dark ? Fade(RAYWHITE, 0.65F) : Color{83, 88, 92, 255});
}

void DrawOperatorSilhouette(Vector2 center, float scale, Color color) {
    DrawCircleV({center.x, center.y - 105.0F * scale}, 58.0F * scale, color);
    DrawTriangle({center.x - 92.0F * scale, center.y + 95.0F * scale},
                 {center.x + 92.0F * scale, center.y + 95.0F * scale},
                 {center.x, center.y - 65.0F * scale}, color);
    DrawLineEx({center.x - 55.0F * scale, center.y - 145.0F * scale},
               {center.x - 25.0F * scale, center.y - 215.0F * scale},
               13.0F * scale, color);
    DrawLineEx({center.x + 55.0F * scale, center.y - 145.0F * scale},
               {center.x + 25.0F * scale, center.y - 215.0F * scale},
               13.0F * scale, color);
}

}  // namespace

MenuAction MainMenu::Update() {
    switch (page_) {
        case Page::Splash:
            if (IsKeyPressed(KEY_ESCAPE)) {
                return MenuAction::Quit;
            }
            if (IsKeyPressed(KEY_ENTER)) {
                page_ = Page::Home;
            }
            break;
        case Page::Home:
            if (IsKeyPressed(KEY_ESCAPE)) {
                return MenuAction::Quit;
            }
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
                homeSelection_ = std::max(0, homeSelection_ - 1);
            }
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
                homeSelection_ = std::min(1, homeSelection_ + 1);
            }
            if (IsKeyPressed(KEY_ENTER)) {
                page_ = homeSelection_ == 0 ? Page::StageSelect
                                             : Page::OperatorSelect;
            }
            break;
        case Page::StageSelect:
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                page_ = Page::Home;
            } else if (IsKeyPressed(KEY_ENTER)) {
                return MenuAction::StartBattle;
            }
            break;
        case Page::OperatorSelect:
            if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) {
                operatorCursor_ = std::max(0, operatorCursor_ - 1);
            }
            if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) {
                operatorCursor_ = std::min(2, operatorCursor_ + 1);
            }
            if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                page_ = Page::Home;
            } else if (IsKeyPressed(KEY_ENTER) && operatorCursor_ == 0) {
                page_ = Page::Home;
            }
            break;
    }
    return MenuAction::None;
}

void MainMenu::Draw(const UiFont& font, const CharacterArt& art) const {
    switch (page_) {
        case Page::Splash:
            DrawSplash(font);
            break;
        case Page::Home:
            DrawHome(font, art);
            break;
        case Page::StageSelect:
            DrawStageSelect(font);
            break;
        case Page::OperatorSelect:
            DrawOperatorSelect(font, art);
            break;
    }
}

void MainMenu::OpenHome() {
    page_ = Page::Home;
}

const char* MainMenu::SelectedOperatorName() const {
    return "能天使";
}

void MainMenu::DrawBackground(const UiFont& font, const char* section) const {
    ClearBackground(kInk);
    DrawRectangleGradientV(0, 0, GameConfig::kScreenWidth,
                           GameConfig::kScreenHeight,
                           Color{80, 91, 96, 255}, Color{16, 21, 25, 255});
    DrawTechLines();
    DrawTriangle({0.0F, 0.0F}, {670.0F, 0.0F}, {410.0F, 720.0F},
                 Fade(BLACK, 0.38F));
    DrawRectangle(0, 0, GameConfig::kScreenWidth, 74, Fade(BLACK, 0.72F));
    DrawRectangle(0, 70, GameConfig::kScreenWidth, 4, kBlue);
    font.Draw("ARKNIGHTS-GO", 42.0F, 19.0F, 30.0F, RAYWHITE);
    font.Draw("// 罗德岛战术终端", 275.0F, 27.0F, 18.0F,
              Fade(RAYWHITE, 0.65F));
    const float sectionWidth = font.Measure(section, 20.0F);
    font.Draw(section, 1225.0F - sectionWidth, 27.0F, 20.0F, kOrange);
}

void MainMenu::DrawSplash(const UiFont& font) const {
    DrawBackground(font, "系统接入");
    DrawRectangle(0, 505, GameConfig::kScreenWidth, 215, Fade(BLACK, 0.58F));
    DrawLineEx({150.0F, 395.0F}, {1130.0F, 395.0F}, 3.0F, kBlue);
    const char* title = "ARKNIGHTS-GO";
    const float titleWidth = font.Measure(title, 70.0F);
    font.Draw(title, 640.0F - titleWidth / 2.0F, 235.0F, 70.0F, RAYWHITE);
    const char* subtitle = "战术终端";
    const float subtitleWidth = font.Measure(subtitle, 27.0F);
    font.Draw(subtitle, 640.0F - subtitleWidth / 2.0F, 325.0F,
              27.0F, Fade(RAYWHITE, 0.72F));
    const char* prompt = "按 Enter 进入主页";
    const float promptWidth = font.Measure(prompt, 28.0F);
    font.Draw(prompt, 640.0F - promptWidth / 2.0F, 570.0F,
              28.0F, RAYWHITE);
    DrawRectangle(640 - 150, 620, 300, 6, kOrange);
}

void MainMenu::DrawHome(const UiFont& font, const CharacterArt& art) const {
    DrawBackground(font, "主页");

    if (art.HasPortrait()) {
        DrawRectangle(48, 82, 478, 585, Fade(BLACK, 0.8F));
        art.DrawPortrait({77.0F, 82.0F, 390.0F, 585.0F});
    } else {
        DrawOperatorSilhouette({315.0F, 480.0F}, 1.25F, Fade(BLACK, 0.82F));
    }
    DrawRectangle(52, 555, 460, 112, Fade(BLACK, 0.72F));
    DrawRectangle(52, 555, 8, 112, kBlue);
    font.Draw("当前出战", 82.0F, 574.0F, 20.0F, Fade(RAYWHITE, 0.65F));
    font.Draw(SelectedOperatorName(), 82.0F, 608.0F, 35.0F, RAYWHITE);

    DrawMenuCard(font, {650.0F, 145.0F, 560.0F, 205.0F},
                 "作战", "选择关卡，开始行动", homeSelection_ == 0);
    DrawMenuCard(font, {730.0F, 390.0F, 480.0F, 165.0F},
                 "干员", "选择出战角色", homeSelection_ == 1);

    DrawRectangle(650, 600, 560, 54, Fade(BLACK, 0.7F));
    font.Draw("W/S 或方向键选择   Enter 确认   Esc 退出", 665.0F, 616.0F,
              19.0F, RAYWHITE);
}

void MainMenu::DrawStageSelect(const UiFont& font) const {
    DrawBackground(font, "作战 / 关卡选择");
    font.Draw("行动档案", 72.0F, 112.0F, 30.0F, RAYWHITE);

    DrawMenuCard(font, {76.0F, 175.0F, 660.0F, 295.0F},
                 "1-1  近卫交锋", "目标：击败弑君者", true, true);
    DrawRectangle(112, 386, 580, 48, kBlue);
    font.Draw("可部署干员：能天使", 136.0F, 398.0F, 20.0F, RAYWHITE);

    DrawMenuCard(font, {785.0F, 175.0F, 410.0F, 128.0F},
                 "1-2", "尚未开放", false, true);
    DrawMenuCard(font, {785.0F, 335.0F, 410.0F, 128.0F},
                 "1-3", "尚未开放", false, true);
    DrawRectangle(76, 560, 1119, 70, Fade(BLACK, 0.72F));
    font.Draw("Enter 开始行动   Backspace/Esc 返回主页", 108.0F, 581.0F,
              22.0F, RAYWHITE);
}

void MainMenu::DrawOperatorSelect(const UiFont& font, const CharacterArt& art) const {
    DrawBackground(font, "干员 / 出战选择");
    font.Draw("选择一名出战干员", 72.0F, 112.0F, 30.0F, RAYWHITE);

    const Rectangle cards[3] = {
        {72.0F, 180.0F, 345.0F, 370.0F},
        {468.0F, 180.0F, 345.0F, 370.0F},
        {864.0F, 180.0F, 345.0F, 370.0F}};
    for (int index = 0; index < 3; ++index) {
        const bool selected = operatorCursor_ == index;
        DrawRectangleRec(cards[index], index == 0 ? kPaper : Color{55, 60, 64, 245});
        DrawRectangleLinesEx(cards[index], selected ? 5.0F : 2.0F,
                             selected ? kBlue : Fade(RAYWHITE, 0.24F));
        if (selected) {
            DrawRectangle(static_cast<int>(cards[index].x),
                          static_cast<int>(cards[index].y),
                          static_cast<int>(cards[index].width), 8, kOrange);
        }
    }

    if (art.HasPortrait()) {
        DrawRectangle(97, 190, 295, 338, BLACK);
        art.DrawPortrait({132.0F, 190.0F, 225.0F, 338.0F});
        if (art.HasChibi()) {
            DrawCircle(348, 472, 51, Fade(kBlue, 0.22F));
            art.DrawChibi({348.0F, 521.0F}, 1, 105.0F,
                          ChibiAnimation::Wave,
                          static_cast<float>(GetTime()));
        }
    } else {
        DrawOperatorSilhouette({244.0F, 405.0F}, 0.72F, kInk);
    }
    font.Draw("能天使", 102.0F, 208.0F, 34.0F,
              art.HasPortrait() ? RAYWHITE : kInk);
    font.Draw("已选择", 304.0F, 216.0F, 18.0F, kOrange);
    font.Draw("高速射击 / 弹匣容量 35", 102.0F, 496.0F, 18.0F,
              Color{69, 75, 80, 255});

    font.Draw("干员 02", 500.0F, 208.0F, 31.0F, Fade(RAYWHITE, 0.52F));
    font.Draw("开发中", 587.0F, 345.0F, 25.0F, Fade(RAYWHITE, 0.42F));
    font.Draw("干员 03", 896.0F, 208.0F, 31.0F, Fade(RAYWHITE, 0.52F));
    font.Draw("开发中", 983.0F, 345.0F, 25.0F, Fade(RAYWHITE, 0.42F));

    DrawRectangle(72, 590, 1137, 58, Fade(BLACK, 0.72F));
    font.Draw("方向键查看   Enter 确认   Backspace/Esc 返回主页",
              104.0F, 608.0F, 20.0F, RAYWHITE);
}
