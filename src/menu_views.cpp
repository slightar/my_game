#include "main_menu.h"
#include "world_layout.h"
#include "character_art.h"
#include "ui_font.h"
#include "terminal_ui.h"
#include <algorithm>
#include <cmath>

namespace {
using namespace TerminalUi;
Color Accent(const EquipmentItem& e) {
    return e.kind == EquipmentKind::Protocol ? Blue : e.kind == EquipmentKind::Echo ? Warm : Orange;
}
const char* KindName(EquipmentKind k) {
    return k == EquipmentKind::Protocol ? "协议" : k == EquipmentKind::Echo ? "残响" : "藏品";
}
void Rule(float x, float y, float width, Color color = Muted) {
    DrawLineEx({x, y}, {x + width, y}, 1, Fade(color, .35F));
}
}

void MainMenu::DrawBackground(const UiFont& font, const char* section) const {
    TerminalUi::Background(font, section, "RHODES ISLAND / OPERATIONS SYSTEM");
}
void MainMenu::Draw(const UiFont& font, const CharacterArt& art) const {
    TerminalUi::SetPointerState(pointer_);
    switch (page_) {
    case Page::Splash:
        TerminalUi::Background(font, "ARKNIGHTS-GO", "FAN PROJECT / DEVELOPMENT BUILD", false);
        font.Skin().Draw("rhodes", {525, 142, 230, 245}, Paper);
        font.Draw("明日方舟 · 记忆档案", 392, 431, 43, Paper);
        Button(font, {430, 546, 420, 64}, "点击或按 Enter 接入终端", true);
        break;
    case Page::Home: DrawHome(font, art); break;
    case Page::CharacterSelect: DrawCharacterSelect(font, art); break;
    case Page::Map: DrawMap(font); break;
    case Page::CityStage: DrawCityStage(font); break;
    case Page::WorldMap: DrawWorldMap(font); break;
    case Page::Archive: DrawArchive(font); break;
    case Page::Equipment: DrawEquipment(font); break;
    case Page::Settings: DrawSettings(font); break;
    case Page::KeyBindings: DrawKeyBindings(font); break;
    case Page::NodeDetail: DrawNodeDetail(font); break;
    case Page::Ending: DrawEnding(font); break;
    default: DrawTransition(font); break;
    }
    if (!status_.empty()) Fit(font, status_, {350, 696, 885, 18}, 12, Paper);
}

void MainMenu::DrawHome(const UiFont& font, const CharacterArt& art) const {
    constexpr Color Blue{0,153,204,255}, Orange{221,112,45,255};
    TerminalUi::Background(font, "行动终端", "HOME / RHODES ISLAND", false);
    // Existing portrait and client terminal texture carry the home visual identity.
    // TODO: Allow the player to choose an assistant and an authored scene background.
    font.Draw("RHODES", 39, 131, 86, Fade(Paper, .12F));
    font.Draw("ISLAND", 41, 221, 86, Fade(Paper, .12F));
    const int leader = characterLineup_[0];
    if (const auto* custom = CharacterForSelection(leader))
        art.DrawCustomPortrait(custom->id, {10, 85, 737, 760});
    else
        art.DrawPortrait(leader == 1 ? OperatorKind::Texas : OperatorKind::Exusiai,
                         {10, 85, 737, 760});
    DrawRectangleGradientH(546, 90, 182, 560, BLANK, Fade(Ink, .26F));
    const auto* current = WorldLayout::Find(progress_.SelectedRegion());
    const std::string leaderName = CharacterNameForSelection(leader);
    Button(font, OperatorButton, ("编队 / " + leaderName).c_str(),
           pointer_.Hit(OperatorButton), Ink, Paper);
    Button(font, EnemyTrialButton, "小怪试炼 / N", pointer_.Hit(EnemyTrialButton), Ink, Paper);
    Button(font, DeveloperButton, "开发者房间 / F12",pointer_.Hit(DeveloperButton),Ink,Paper);
    HomeSurface({43, 457, 607, 174}, Fade(Ink, .87F));
    DrawRectangle(43, 457, 5, 174, Orange);
    font.Draw("CURRENT OPERATION", 65, 472, 12, Muted);
    Fit(font, "切尔诺伯格 / 区域探索", {65, 493, 560, 30}, 26, Paper);
    Fit(font, current ? current->name : "切尔诺伯格", {65, 535, 554, 29}, 25, Paper);
    Fit(font, current ? current->subtitle : "继续当前行动", {65, 577, 552, 30}, 18, Muted);

    HomeSurface({720, 105, 492, 37}, Fade(Ink, .82F));
    font.Draw(TextFormat("PRTS  %02d", progress_.Compliance()), 735, 116, 17, Paper);
    font.Draw(TextFormat("真相  %02d", progress_.Truth()), 903, 116, 17, Paper);
    font.Draw(TextFormat("记忆  %02d", progress_.MemoryIntegrity()), 1064, 116, 17, Paper);
    const auto main = HomeButtons[0];
    const auto mainDraw = PressedRect(main);
    HomeSurface(mainDraw, Paper);
    if (!font.Skin().Draw("home_battle", mainDraw)) font.Draw("终端", mainDraw.x + 30, mainDraw.y + 19, 47, Ink);
    DrawRectangle(int(mainDraw.x), int(mainDraw.y + 103), 492, 44, Ink);
    font.Draw("探索地图", mainDraw.x + 24, mainDraw.y + 113, 25, Paper);
    font.Draw("REGION / ACCESS  >", mainDraw.x + 287, mainDraw.y + 120, 12, Muted);
    if (homeSelection_ == 0 || pointer_.Hit(main)) DrawRectangle(int(mainDraw.x), int(mainDraw.y + 143), 492, 4, Orange);
    PressedVeil(main);
    const char* names[] = {"探索地图", "档案", "装备", "设置", "继续行动", "退出"};
    const char* subtitles[] = {"", "ARCHIVES", "EQUIPMENT", "SETTINGS", "CONTINUE OPERATION", "EXIT"};
    for (int i = 1; i < 6; ++i) {
        const auto r = HomeButtons[i];
        const bool selected = i == homeSelection_ || pointer_.Hit(r);
        const bool primary = i == 4;
        HomeSurface(PressedRect(r), primary ? Blue : Paper);
        if (i == 1 || i == 2) Emblem({r.x + r.width - 45, r.y + 45}, 33, i - 1, {190, 198, 200, 255});
        const auto draw = PressedRect(r);
        font.Draw(names[i], draw.x + 22, draw.y + (i < 3 ? 17 : 11), i < 3 ? 36 : 27, primary ? Paper : Ink);
        font.Draw(subtitles[i], draw.x + 24, draw.y + draw.height - 23, 11, primary ? Paper : Color{86, 94, 99, 255});
        if (selected) DrawRectangleRec({draw.x, draw.y + draw.height - 4, draw.width, 4}, primary ? Paper : Orange);
        PressedVeil(r);
    }
    int exits = 0;
    if (current) for (const auto& e : WorldLayout::Ports(current->id))
        if(progress_.RegionVisible(e.target->id))++exits;
    font.Draw(TextFormat("开放出口  %02d", exits), 724, 608, 16, Paper);
    std::string slots = "装备  ";
    for (const auto& id : progress_.Slots()) {
        const auto* item = progress_.FindEquipment(id);
        slots += item ? std::string(KindName(item->kind)) + " / " : "空 / ";
    }
    Fit(font, slots, {912, 603, 300, 27}, 15, Paper);
    Footer(font, "点击功能块 / ↑↓ 选择 · Enter 确认    C 角色编队    F4 图片创建角色    B 弑君者演示    N 小怪试炼    F12 开发者房间");
}

void MainMenu::DrawCharacterSelect(const UiFont& font, const CharacterArt& art) const {
    DrawBackground(font, "角色选择 / 行动编队");
    Button(font, {1028, 94, 204, 27}, "图片创建角色 / F4", pointer_.Hit({1028, 94, 204, 27}), Blue, Paper);
    const int count = 2 + static_cast<int>(customCharacters_.size());
    const int selected = std::clamp(characterSelection_, 0, count - 1);
    const Character* custom = CharacterForSelection(selected);
    const bool deletable=custom&&deletableCharacters_.contains(custom->id);
    Button(font,CharacterDelete,"删除角色 / Del",false,Ink,deletable?Paper:Muted);
    const std::string name = CharacterNameForSelection(selected);
    const OperatorKind kind = selected == 1 ? OperatorKind::Texas : OperatorKind::Exusiai;

    Surface({48, 124, 274, 508}, Ink);
    font.Draw("可用干员", 60, 140, 24, Paper);
    font.Draw(TextFormat("%02d", count), 265, 139, 28, Muted);
    const int start = std::clamp(selected - 3, 0, std::max(0, count - 6));
    for (int row = 0; row < std::min(6, count - start); ++row) {
        const int choice = start + row;
        const Rectangle hit=RosterRow(row);
        const Rectangle draw = PressedRect(hit);
        const bool active = choice == selected;
        const bool deployed = choice == draftCharacterLineup_[0] ||
                              choice == draftCharacterLineup_[1];
        Surface(draw, active ? Paper : Panel);
        DrawRectangleRec({draw.x, draw.y, active ? 6.0F : 3.0F, draw.height},
                         active ? Blue : Fade(Paper, .30F));
        Fit(font, CharacterNameForSelection(choice),
            {draw.x + 16, draw.y + 7, 176, 27}, 21, active?Ink:Paper);
        font.Draw(choice < 2 ? (choice == 0 ? "狙击 / 远程" : "先锋 / 近战")
                             : "自定义 / 本地",
                  draw.x + 17, draw.y + 35, 12, active?OnPaper:Muted);
        if (deployed) {
            const char* slot = draftCharacterLineup_[0] == choice ? "01" : "02";
            Surface({draw.x + 208, draw.y + 13, 32, 29}, active ? Blue : Paper);
            font.Draw(slot, draw.x + 215, draw.y + 19, 14,
                      active ? Paper : Ink);
        }
        PressedVeil(hit);
    }

    Surface({342, 124, 450, 508}, SoftPaper);
    font.Draw("OPERATOR",358,137,58,Fade(Ink,.10F));
    font.Draw(TextFormat("PERSONNEL / %03d",selected+1),363,203,12,OnPaper);
    DrawRectangle(362,228,54,3,Blue);
    if (custom)
        art.DrawCustomPortrait(custom->id, {398, 155, 380, 402});
    else
        art.DrawPortrait(kind, {398, 155, 380, 402});
    DrawRectangleGradientV(342, 444, 450, 100, BLANK, Fade(Ink, .65F));
    Surface({342, 540, 450, 92}, Ink);
    DrawRectangle(342,540,4,92,Blue);
    Fit(font, name, {365, 548, 400, 43}, 35, Paper);
    font.Draw(custom ? "CUSTOM / LOCAL ASSET" :
              (selected == 0 ? "SNIPER / RHODES ISLAND" : "VANGUARD / RHODES ISLAND"),
              366, 601, 12, Muted);

    Surface({816, 124, 416, 508}, Paper);
    DrawRectangle(816, 124, 416, 48, Ink);
    font.Draw("出战配置", 840, 138, 23, Paper);
    font.Draw("SQUAD / 02",1103,147,11,Muted);
    for (int slot = 0; slot < 2; ++slot) {
        const Rectangle hit = CharacterSlots[slot];
        const Rectangle draw = PressedRect(hit);
        Surface(draw, slot == characterSlot_ ? Ink : Color{218, 222, 221, 255});
        font.Draw(TextFormat("SLOT %02d", slot + 1), draw.x + 14, draw.y + 10,
                  12, slot == characterSlot_ ? Muted : Color{94, 103, 108, 255});
        Fit(font, CharacterNameForSelection(draftCharacterLineup_[slot]),
            {draw.x + 14, draw.y + 31, draw.width - 28, 32}, 21,
            slot == characterSlot_ ? Paper : Ink);
        if (slot == characterSlot_)
            DrawRectangleRec({draw.x, draw.y + draw.height - 4, draw.width, 4}, Blue);
        PressedVeil(hit);
    }

    const int health = custom ? custom->stats.health : 3;
    const int speed = static_cast<int>(custom ? custom->stats.moveSpeed : 320.0F);
    const int jump = static_cast<int>(custom ? custom->stats.jumpSpeed : 610.0F);
    const float interval = custom ? custom->stats.attackInterval
                                  : selected == 0 ? .12F : .25F;
    Rule(840, 283, 368, Ink);
    font.Draw("作战参数", 840, 296, 18, Ink);
    font.Draw("HP",841,330,11,OnPaper); font.Draw(TextFormat("%d",health),887,325,27,Ink);
    font.Draw("MOVE",1009,330,11,OnPaper);font.Draw(TextFormat("%d",speed),1083,325,27,Ink);
    font.Draw("JUMP",841,366,11,OnPaper);font.Draw(TextFormat("%d",jump),899,359,24,Ink);
    font.Draw("ATK",1009,366,11,OnPaper);font.Draw(TextFormat("%.2fs",interval),1061,359,24,Ink);
    Rule(840, 398, 368, Ink);
    const std::string description = custom ? custom->description
        : selected == 0 ? "高速远程输出，擅长持续火力与弹幕压制。"
                        : "快速近战切入，擅长剑气与范围剑雨。";
    Wrap(font, description, {840, 414, 365, 54}, 16, Ink);
    for (int slot = 0; slot < 2; ++slot) {
        const float y = 475.0F + slot * 35.0F;
        std::string skill;
        if (custom && slot < static_cast<int>(custom->skills.size()))
            skill = custom->skills[slot].name;
        else if (!custom)
            skill = selected == 0 ? (slot == 0 ? "E / 扫射" : "Q / 过载")
                                  : (slot == 0 ? "E / 初始" : "Q / 剑雨");
        else
            skill = slot == 0 ? "E / 未配置" : "Q / 未配置";
        DrawRectangleRec({840, y, 368, 28}, Color{224, 227, 225, 255});
        DrawRectangleRec({840, y, 4, 28}, slot == 0 ? Blue : Orange);
        Fit(font, skill, {852, y, 344, 28}, 15, Ink);
    }
    Button(font, CharacterAssign, "编入当前栏", pointer_.Hit(CharacterAssign), Ink, Paper);
    Button(font, CharacterConfirm, "保存编队", pointer_.Hit(CharacterConfirm), Blue, Paper);
    Footer(font, "点击干员查看详情 · 点击 SLOT 选择位置 · Enter 编入 · C 保存编队 · ↑↓/滚轮选择 · Esc 返回");
    if(confirmingDeletion_) {
        DrawRectangle(0,89,1280,564,Fade(BLACK,.78F));
        Surface({370,219,540,286},Paper);
        font.Draw("删除自定义角色？",402,246,30,Ink);
        Fit(font,name,{403,297,470,36},24,Ink);
        Wrap(font,"删除后将退出编队。文件保留在本地回收区。",{403,349,470,55},18,OnPaper);
        Button(font,DeleteCancel,"取消",pointer_.Hit(DeleteCancel),Ink,Paper);
        Button(font,DeleteConfirm,"确认删除",pointer_.Hit(DeleteConfirm),Color{161,63,54,255},Paper);
    }
}

void MainMenu::DrawArchive(const UiFont& font) const {
    DrawBackground(font, "档案 / 行动记录");
    const auto entries = progress_.Archives(archiveCategory_ == 0 ? ArchiveCategory::Story : ArchiveCategory::Item);
    font.Draw("DATABASE / RECORDS", 58, 112, 12, Muted);
    for (int i = 0; i < 2; ++i) Button(font, Category(i), i == 0 ? "关键剧情" : "道具记录", i == archiveCategory_, i == archiveCategory_ ? Paper : Ink, i == archiveCategory_ ? Ink : Paper);
    font.Draw("ARCHIVE",58,352,47,Paper);
    DrawRectangle(58,420,54,3,Blue);
    font.Draw(TextFormat("%02d",int(entries.size())),58,461,73,Paper);
    font.Draw("RECORDS / INDEX",61,552,12,Muted);
    Surface({321, 109, 405, 518}, Ink);
    SectionHeading(font,{347,122,353,49},"记录检索","RECORD DIRECTORY");
    const int start = std::clamp(archiveSelection_ - 3, 0, std::max(0, int(entries.size()) - 7));
    for (int i = start; i < std::min(start + 7, int(entries.size())); ++i) {
        const auto& e = entries[i]; const float y = 180.0F + (i - start) * 55;
        const bool selected = i == archiveSelection_;
        DrawRectangleRec({340, y, 367, 45}, selected ? Paper : Color{49, 55, 60, 255});
        if (selected) DrawRectangle(340, int(y), 4, 45, Blue);
        font.Draw(TextFormat("%02d", i + 1), 351, y + 15, 14, selected ? Ink : Muted);
        Fit(font, e.unlocked ? e.name : "???? / 无法检索", {386, y, 307, 45}, 19, selected ? Ink : e.unlocked ? Paper : Muted);
        PressedVeil({340, y, 367, 45});
    }
    font.Draw("滚轮 / ↑↓ 翻阅", 350, 592, 14, Muted);
    Surface({747, 109, 477, 518}, Paper);
    DrawRectangle(747, 109, 477, 49, Ink);
    font.Draw("RHODES ISLAND / DOCUMENT", 775, 126, 13, Paper);
    if (!entries.empty()) {
        const auto& e = entries[archiveSelection_];
        font.Draw(TextFormat("FILE / %03d", archiveSelection_ + 1), 779, 185, 16, OnPaper);
        DrawRectangle(1149,185,38,4,e.unlocked?Blue:OnPaper);
        Wrap(font, e.unlocked ? e.name : "记录损坏", {778, 228, 406, 84}, 30, Ink);
        Rule(778, 327, 402, Ink);
        Wrap(font, e.unlocked ? e.description : "????\n记录尚未恢复，无法检索。", {778, 353, 405, 170}, 21, Ink);
        DrawLine(778,539,1186,539,Fade(Ink,.23F));
        if (e.unlocked) font.Draw((std::string("来源 / ") + SourceName(e.source)).c_str(), 778, 555, 16, OnPaper);
        font.Draw(TextFormat("%02d / %02d", archiveSelection_ + 1, int(entries.size())), 1083, 593, 16, Ink);
    }
    Footer(font, "点击分类与条目    ←→ / A D 切换分类    ↑↓ / W S / 滚轮选择记录    返回箭头 / Esc 主页");
}

void MainMenu::DrawEquipment(const UiFont& font) const {
    DrawBackground(font, "装备 / 战术配置");
    font.Draw("LOADOUT / SELECT SLOT", 63, 113, 12, Muted);
    font.Draw("任意类型可放入任一装备栏", 930, 112, 16, Paper);
    for (int i = 0; i < 3; ++i) {
        const auto* item = progress_.FindEquipment(progress_.Slots()[i]);
        const auto r = Rectangle{62.0F + i * 403, 151, 378, 105};
        const Color color = item ? Accent(*item) : Muted;
        Surface(r, i == equipmentSlot_ ? Paper : Panel);
        font.Draw(TextFormat("%02d",i+1),r.x+290,r.y+13,70,
                  i==equipmentSlot_?Fade(Ink,.08F):Fade(Paper,.08F));
        DrawRectangleRec({r.x, r.y + 101, r.width, 4}, i == equipmentSlot_ ? Blue : color);
        font.Draw(TextFormat("SLOT %02d", i + 1), r.x + 18, r.y + 14, 13, i == equipmentSlot_ ? Ink : Muted);
        Fit(font, item ? item->name : "未装备", {r.x + 18, r.y + 44, 326, 35}, 27, i == equipmentSlot_ ? Ink : Paper);
        PressedVeil(r);
    }
    Surface({62, 281, 548, 348}, Ink);
    Surface({631, 281, 587, 348}, Paper);
    font.Draw("持有装备", 83, 299, 23, Paper);
    font.Draw("INVENTORY",477,309,11,Muted);
    const auto entries = progress_.Equipment();
    const EquipmentEntry* selected = nullptr; int row = 0;
    const int start = std::max(0, equipmentSelection_ - 3);
    for (const auto& item : entries) if (item.owned) {
        if (row == equipmentSelection_) selected = &item;
        if (row >= start && row < start + 6) {
            const float y = 343.0F + (row - start) * 43;
            const bool active = row == equipmentSelection_;
            DrawRectangleRec({78, y, 516, 39}, active ? Paper : Panel);
            DrawRectangle(78, int(y), 4, 39, Accent(item));
            font.Draw(KindName(item.kind), 93, y + 10, 15, active?OnPaper:Muted);
            Fit(font, item.name, {158, y, 418, 39}, 20, active?Ink:Paper);
            PressedVeil({78, y, 516, 39});
        }
        ++row;
    }
    if (!row) { font.Draw("暂无装备", 98, 383, 30, Muted); Wrap(font, "通过当前地图出口探索，完成行动后获得协议、藏品与残响。", {98, 448, 445, 107}, 19, Muted); }
    if (selected) {
        const Color color = Accent(*selected);
        DrawRectangle(631, 281, 587, 6, color);
        Fit(font, selected->name, {658, 306, 529, 44}, 33, Ink);
        font.Draw((std::string(KindName(selected->kind)) + " / " + SourceName(selected->source)).c_str(), 660, 364, 20, Ink);
        Rule(658, 405, 526, Ink);
        Wrap(font, selected->description, {658, 423, 526, 67}, 19, Ink);
        Fit(font, selected->effect, {658, 494, 526, 30}, 18, Ink);
        font.Draw(progress_.IsProtocolSuppressed() && selected->kind == EquipmentKind::Protocol ? "权限已撤销（仅此战）" : "权限有效 / 效果开发占位", 658, 528, 14, Ink);
    } else { Emblem({924, 386}, 47, 1, Muted); font.Draw("等待配置", 860, 459, 28, Ink); }
    Button(font, Unequip, "卸下当前栏", pointer_.Hit(Unequip), Ink, Paper);
    Button(font, Equip, "确认装备", pointer_.Hit(Equip), Blue, Paper);
    Footer(font, "点击装备栏与物品，再确认装备    ←→ 选栏    ↑↓ / 滚轮选物品    Enter 装备    X 卸下    Esc 返回");
}

void MainMenu::DrawSettings(const UiFont& font) const {
    DrawBackground(font, "设置 / 系统配置");
    font.Draw("SYSTEM",54,151,47,Paper);
    font.Draw("PREFERENCES",57,210,13,Muted);
    DrawRectangle(57,247,54,3,Blue);
    font.Draw("系统配置",57,285,27,Paper);
    Wrap(font,"音量与操作设置",{57,335,208,60},18,Muted);
    font.Draw("01 / AUDIO",57,490,12,Muted);
    font.Draw("02 / CONTROLS",57,518,12,Muted);
    font.Draw("03 / SAVE DATA",57,546,12,Muted);
    font.Draw("PREFERENCES / CONTROL PANEL",316,127,12,Muted);
    const char* labels[] = {"总音量", "音效音量", "自定义键位", "重置游戏进度", "返回主页"};
    const char* hints[] = {"MASTER VOLUME","SOUND EFFECTS","KEY BINDINGS","RESET PROGRESS","RETURN TO HOME"};
    for (int i = 0; i < 5; ++i) {
        const float y = 166.0F + i * 88;
        const bool selected = i == settingsSelection_;
        Surface(SettingsRow(i), selected ? Paper : Panel);
        const Color text = selected ? Ink : Paper;
        if (selected) DrawRectangle(304, int(y), 4, 71, Blue);
        font.Draw(TextFormat("%02d",i+1),324,y+23,22,selected?OnPaper:Muted);
        font.Draw(labels[i], 374, y + 12, 25, text);
        font.Draw(hints[i],376,y+47,10,selected?OnPaper:Muted);
        if (i < 2) {
            const int value = i == 0 ? settings_.MasterVolume() : settings_.SoundVolume();
            DrawRectangle(690, int(y + 33), 345, 4, selected?Fade(Ink,.3F):Muted);
            DrawRectangle(690, int(y + 33), 345 * value / 100, 4, selected?Blue:Paper);
            DrawRectangle(685 + 345 * value / 100, int(y + 23), 10, 25, selected ? Ink : Paper);
            font.Draw(TextFormat("%d%%", value), 1060, y + 21, 25, text);
        } else {
            font.Draw(i == 3 ? "需再次确认" : "打开", 1000, y + 25, 17,
                      i == 3 ? (selected ? Orange : Color{230,157,97,255}) : text);
            font.Draw(">",1157,y+21,25,text);
        }
        PressedVeil({304, y, 895, 71});
    }
    Footer(font, "点击条目或拖动音量滑块    ↑↓ 选择    ←→ 调整音量    Enter 打开    返回箭头 / Esc 主页");
}
void MainMenu::DrawKeyBindings(const UiFont& font) const {
    DrawBackground(font, "设置 / 自定义键位");
    font.Draw("战斗操作", 83, 109, 28, Paper);
    font.Draw("菜单固定使用方向键 / WASD、Enter 与 Esc", 628, 118, 18, Paper);
    Surface({81, 146, 1115, 483}, Ink);
    DrawRectangle(775,146,421,483,Panel);
    for (int i = 0; i < GameSettings::kActionCount; ++i) {
        const float y = 152.0F + i * 43;
        const bool selected = i == keySelection_;
        if (selected) {DrawRectangle(81, int(y - 3), 1115, 40, Paper);DrawRectangle(81,int(y-3),4,40,Blue);}
        else DrawLine(105,int(y+37),1175,int(y+37),Fade(Paper,.12F));
        font.Draw(TextFormat("%02d",i+1),113,y+7,13,selected?OnPaper:Muted);
        font.Draw(GameSettings::ActionName(static_cast<GameAction>(i)), 164, y + 3, 21, selected ? Ink : Paper);
        const auto value = capturingKey_ && selected ? "按下新键…" : GameSettings::KeyName(settings_.Key(static_cast<GameAction>(i)));
        DrawRectangle(831,int(y+1),310,31,selected?Ink:Canvas);
        font.Draw(value.c_str(), 849, y + 6, 19, Paper);
        PressedVeil({81, 149.0F + i * 43, 1115, 40});
    }
    Footer(font, capturingKey_ ? "按任意可用键设置    Esc / 返回箭头取消    重复键位不可使用" : "点击操作修改键位    ↑↓ 选择    Enter 修改    返回箭头 / Esc 返回设置");
}

void MainMenu::DrawNodeDetail(const UiFont& font) const {
    DrawBackground(font, "行动准备");
    const auto* node = progress_.CurrentNode(); if (!node) return;
    const bool memory = node->type == NodeType::Hidden || node->source == EquipmentSource::Theresa;
    Surface({65, 113, 354, 506}, Ink);
    font.Draw("OPERATION", 90, 141, 19, Muted);
    Emblem({240, 292}, 75, node->type == NodeType::Boss ? 1 : 2, memory ? Warm : Paper);
    Wrap(font, node->name, {90, 411, 300, 105}, 32, Paper);
    font.Draw(NodeTypeName(node->type), 90, 571, 22, memory ? Warm : Paper);
    Surface({441, 113, 774, 506}, Paper);
    Fit(font, node->location, {471, 140, 710, 39}, 30, Ink);
    Rule(471, 194, 710, Ink);
    Wrap(font, "任务 / " + node->objective, {471, 216, 710, 69}, 23, Ink);
    Fit(font, "关联角色 / " + node->relatedCharacters, {471, 284, 710, 31}, 19, Ink);
    Wrap(font, node->description, {471, 331, 710, 63}, 19, Ink);
    Fit(font, "入口条件 / " + progress_.UnlockConditionText(node->id), {471, 401, 710, 27}, 17, Ink);
    std::string reward = "奖励 / ";
    for (const auto& id : node->reward.archiveIds) if (const auto* e = StoryData::FindArchive(id)) reward += e->name + "  ";
    for (const auto& id : node->reward.equipmentIds) if (const auto* e = StoryData::FindEquipment(id)) reward += e->name + "  ";
    if (node->reward.archiveIds.empty() && node->reward.equipmentIds.empty()) reward += "推进任务 / 开放出口";
    Fit(font, reward, {471, 439, 710, 28}, 18, Ink);
    font.Draw(TextFormat("PRTS +%d    真相 +%d    记忆 +%d", node->reward.compliance, node->reward.truth, node->reward.memory), 471, 482, 17, Ink);
    font.Draw(node->requiresNoProtocol ? "完成要求：卸下全部协议" : "完成要求：到达当前地点", 471, 514, 17, Ink);
    Button(font, Confirm, progress_.IsCompleted(node->id) ? "已完成 · 返回地图" : node->id == "ending_prts_core" ? "进入 PRTS 战斗占位" : "完成行动（开发占位）", pointer_.Hit(Confirm), memory ? Color{137, 80, 76, 255} : Blue, Paper);
    Footer(font, "开发占位：关卡战斗、机关与剧情演出待接入    Enter 确认    返回箭头 / Esc 返回当前地图");
}

void MainMenu::DrawTransition(const UiFont& font) const {
    if (page_ == Page::ConfirmReset) {
        DrawBackground(font, "设置 / 重置进度");
        Surface({165, 175, 950, 355}, Paper, Orange);
        font.Draw("确认重置游戏进度？", 219, 222, 39, Ink);
        font.Draw("地图、档案、装备及结局记录将恢复初始状态。", 221, 309, 24, Ink);
        font.Draw("音量与自定义键位会保留。", 221, 363, 21, Ink);
        Button(font, ResetConfirm, "确认重置", pointer_.Hit(ResetConfirm), {161, 63, 54, 255}, Paper);
        Footer(font, "点击确认重置 / Enter 确认    返回箭头 / Esc 取消");
    } else if (page_ == Page::FirstReset) {
        DrawBackground(font, "序章 / 第一次重置");
        Surface({110, 137, 1060, 473}, Ink, Orange);
        font.Draw("记录无法保存", 158, 183, 49, Paper);
        font.Draw("第一次重置 / PRTS 接入", 161, 267, 25, Orange);
        font.Draw("特蕾西娅的声音仍在。终端已重新接入。", 161, 350, 25, Paper);
        font.Draw("此前发生的事件：未检索到记录。", 161, 404, 22, Muted);
        Button(font, Confirm, "返回地图", pointer_.Hit(Confirm), Blue, Paper);
        Footer(font, "剧情演出开发占位：第一章出口已开放    Enter / 点击按钮继续");
    } else {
        DrawBackground(font, "PRTS / 权限撤销");
        Surface({110, 135, 1060, 475}, Ink, Orange);
        font.Draw("击败 PRTS", 157, 173, 47, Paper);
        font.Draw("BOSS ENCOUNTER / AUTHORIZATION REVOKED", 160, 237, 15, Orange);
        font.Draw("协议：权限已撤销（仅此战）", 160, 292, 28, Orange);
        font.Draw("离开此战后恢复；藏品与残响保持有效。", 160, 337, 21, Paper);
        int y = 392;
        for (const auto& id : progress_.Slots()) if (const auto* item = progress_.FindEquipment(id)) {
            font.Draw((item->name + (item->kind == EquipmentKind::Protocol ? " / 已失效（仅此战）" : " / 有效")).c_str(), 162, float(y), 21, Accent(*item)); y += 38;
        }
        Button(font, Confirm, "确认击败 PRTS（占位）", pointer_.Hit(Confirm), {137, 80, 76, 255}, Paper);
        Footer(font, "开发占位：Boss 攻击与关卡战斗逻辑待接入    Enter 确认胜利    返回箭头 / Esc 离开战斗");
    }
}

void MainMenu::DrawEnding(const UiFont& font) const {
    // TODO: Replace these distinct ending presentations with authored scenes and transitions.
    const bool hidden = ending_ == EndingType::Hidden;
    if (hidden) {
        ClearBackground({199, 190, 178, 255});
        DrawRectangleGradientV(0, 0, 1280, 720, {226, 218, 204, 255}, {120, 107, 102, 255});
        DrawRectangle(102, 85, 308, 493, {186, 177, 166, 255});
        DrawRectangle(127, 108, 255, 317, {223, 216, 201, 255});
        DrawRectangle(248, 108, 10, 317, {158, 148, 139, 255});
        DrawRectangle(127, 260, 255, 10, {158, 148, 139, 255});
        DrawLineEx({545, 587}, {601, 501}, 2, {126, 81, 78, 180});
        DrawLineEx({601, 501}, {577, 454}, 2, {126, 81, 78, 180});
        font.Draw("拥抱特蕾西娅", 481, 203, 51, Ink);
        Wrap(font, "PRTS 核心已停止。诊疗所重新安静下来。\n特蕾西娅已化为原石。\n留下的是储存在游戏中的信息；她仍在被记得。", {487, 316, 650, 207}, 24, Ink);
        font.Draw("结局演出开发占位", 488, 558, 16, Ink);
        Button(font, Confirm, "返回探索地图", pointer_.Hit(Confirm), {224, 215, 202, 255}, Ink);
    } else {
        DrawBackground(font, "行动结束 / 数据归档");
        Surface({95, 137, 1090, 473}, Ink);
        DrawRectangle(95, 137, 344, 473, Paper);
        font.Draw("MISSION", 126, 192, 43, Ink);
        font.Draw("COMPLETE", 126, 248, 40, Ink);
        font.Skin().Draw("rhodes", {171, 345, 158, 168});
        font.Draw("标准处置", 481, 201, 48, Paper);
        font.Draw("PRTS：任务成功。塔露拉行动已结束。", 486, 303, 24, Paper);
        font.Draw("世界看似恢复正常，随后被稳定封存。", 486, 367, 22, Muted);
        font.Draw("任务完成 / 数据归档 / 世界稳定", 486, 448, 19, Paper);
        font.Draw("结局演出开发占位", 486, 493, 16, Muted);
        Button(font, Confirm, "返回探索地图", pointer_.Hit(Confirm), Paper, Ink);
        Footer(font, "Enter / Esc / 返回按钮继续探索");
    }
}
