#include "main_menu.h"
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
    switch (page_) {
    case Page::Splash:
        TerminalUi::Background(font, "ARKNIGHTS-GO", "FAN PROJECT / DEVELOPMENT BUILD", false);
        font.Skin().Draw("rhodes", {525, 142, 230, 245}, Paper);
        font.Draw("明日方舟 · 记忆档案", 392, 431, 43, Paper);
        Button(font, {430, 546, 420, 64}, "点击或按 Enter 接入终端", true);
        break;
    case Page::Home: DrawHome(font, art); break;
    case Page::Map: DrawMap(font); break;
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
    TerminalUi::Background(font, "行动终端", "HOME / RHODES ISLAND", false);
    // Existing portrait and client terminal texture carry the home visual identity.
    // TODO: Allow the player to choose an assistant and an authored scene background.
    font.Draw("RHODES", 39, 131, 86, Fade(Paper, .12F));
    font.Draw("ISLAND", 41, 221, 86, Fade(Paper, .12F));
    art.DrawPortrait(OperatorKind::Exusiai, {10, 85, 737, 760});
    DrawRectangleGradientH(546, 90, 182, 560, BLANK, Fade(Ink, .26F));
    const auto* current = progress_.CurrentNode();
    const auto* chapter = current ? StoryData::FindChapter(current->chapterId) : nullptr;
    Surface({43, 397, 163, 42}, Ink);
    font.Draw("行动协助 / 能天使", 54, 410, 17, Paper);
    Surface({43, 457, 607, 174}, Fade(Ink, .87F));
    DrawRectangle(43, 457, 5, 174, Orange);
    font.Draw("CURRENT OPERATION", 65, 472, 12, Muted);
    Fit(font, chapter ? chapter->title : "序章", {65, 493, 560, 30}, 26, Paper);
    Fit(font, current ? current->name : "切尔诺伯格", {65, 535, 554, 29}, 25, Paper);
    Fit(font, current ? current->objective : "继续当前行动", {65, 577, 552, 30}, 18, Muted);

    Surface({720, 105, 492, 37}, Fade(Ink, .82F));
    font.Draw(TextFormat("PRTS  %02d", progress_.Compliance()), 735, 116, 17, Paper);
    font.Draw(TextFormat("真相  %02d", progress_.Truth()), 903, 116, 17, Paper);
    font.Draw(TextFormat("记忆  %02d", progress_.MemoryIntegrity()), 1064, 116, 17, Paper);
    const auto main = HomeButtons[0];
    Surface(main, Paper);
    if (!font.Skin().Draw("home_battle", main)) font.Draw("终端", 750, 176, 47, Ink);
    DrawRectangle(720, 260, 492, 44, Ink);
    font.Draw("探索地图", 744, 270, 25, Paper);
    font.Draw("REGION / ACCESS  >", 1007, 277, 12, Muted);
    if (homeSelection_ == 0 || pointer_.Hit(main)) DrawRectangle(720, 300, 492, 4, Orange);
    const char* names[] = {"探索地图", "档案", "装备", "设置", "继续行动", "退出"};
    const char* subtitles[] = {"", "ARCHIVES", "EQUIPMENT", "SETTINGS", "CONTINUE OPERATION", "EXIT"};
    for (int i = 1; i < 6; ++i) {
        const auto r = HomeButtons[i];
        const bool selected = i == homeSelection_ || pointer_.Hit(r);
        const bool primary = i == 4;
        Surface(r, primary ? Blue : Paper);
        if (i == 1 || i == 2) Emblem({r.x + r.width - 45, r.y + 45}, 33, i - 1, {190, 198, 200, 255});
        font.Draw(names[i], r.x + 22, r.y + (i < 3 ? 17 : 11), i < 3 ? 36 : 27, primary ? Paper : Ink);
        font.Draw(subtitles[i], r.x + 24, r.y + r.height - 23, 11, primary ? Paper : Color{86, 94, 99, 255});
        if (selected) DrawRectangleRec({r.x, r.y + r.height - 4, r.width, 4}, primary ? Paper : Orange);
    }
    int exits = 0;
    if (current) for (const auto& e : current->exits) if (progress_.ExitAvailable(e)) ++exits;
    font.Draw(TextFormat("开放出口  %02d", exits), 724, 608, 16, Paper);
    std::string slots = "装备  ";
    for (const auto& id : progress_.Slots()) {
        const auto* item = progress_.FindEquipment(id);
        slots += item ? std::string(KindName(item->kind)) + " / " : "空 / ";
    }
    Fit(font, slots, {912, 603, 300, 27}, 15, Paper);
    Footer(font, "点击功能块 / ↑↓ 选择 · Enter 确认    B 战斗演示    F6/F7/F8/F10/F11 开发调试");
}

void MainMenu::DrawMap(const UiFont& font) const {
    const auto* node = progress_.CurrentNode(); if (!node) return;
    const auto* chapter = StoryData::FindChapter(node->chapterId);
    DrawBackground(font, "终端 / 区域观测");
    Fit(font, chapter ? chapter->title : "当前区域", {64, 106, 660, 31}, 28, Paper);
    font.Draw(("当前位置 / " + node->name).c_str(), 67, 147, 19, Paper);
    font.Draw("LOCAL AREA / EXIT CONNECTIONS", 861, 119, 13, Muted);
    Surface(MapArea, {38, 44, 49, 244});
    // TODO: Replace the traversable schematic with authored terrain and physical exit triggers.
    for (int x = 109; x < 1195; x += 40) DrawLine(x, 184, x, 596, {71, 78, 82, 85});
    for (int y = 196; y < 596; y += 40) DrawLine(85, y, 1195, y, {71, 78, 82, 85});
    const Rectangle blocks[] = {{120, 251, 142, 102}, {145, 411, 179, 95}, {442, 279, 96, 79}, {481, 456, 151, 76}, {851, 226, 130, 105}, {944, 465, 169, 82}};
    for (auto r : blocks) { DrawRectangleRec(r, {56, 63, 68, 255}); DrawRectangleLinesEx(r, 1, {84, 92, 96, 255}); }
    font.Draw("N", 116, 213, 15, Muted);
    DrawLine(120, 245, 120, 233, Paper);
    for (const auto& exit : node->exits) {
        const bool visible = progress_.ExitVisible(exit), available = progress_.ExitAvailable(exit);
        const Color color = exit.hidden ? Warm : Paper;
        if (!visible) {
            // Undiscovered exits show only a faint environmental hint, never the target name.
            DrawLineEx({exit.x - 16, exit.y + 8}, {exit.x + 8, exit.y - 8}, 1, Fade(Warm, .36F));
            Fit(font, exit.hint, {exit.x - 115, exit.y + 15, 220, 22}, 12, Fade(Paper, .45F));
            continue;
        }
        const Vector2 bend{exit.x, 390};
        DrawLineEx({640, 390}, bend, available ? 3 : 1, Fade(color, available ? .8F : .3F));
        DrawLineEx(bend, {exit.x, exit.y}, available ? 3 : 1, Fade(color, available ? .8F : .3F));
        DrawPoly({exit.x, exit.y}, 4, 24, 0, color);
        DrawPoly({exit.x, exit.y}, 4, 19, 0, Ink);
        DrawPoly({exit.x, exit.y}, 4, 8, 0, available ? color : Muted);
        const float labelY = exit.y < 240 ? exit.y + 30 : exit.y - 64;
        Surface({exit.x - 104, labelY, 208, 29}, color);
        Fit(font, exit.label, {exit.x - 96, labelY, 192, 29}, 17, Ink);
        font.Draw(available ? (exit.hidden ? "记忆裂隙 / 可进入" : "主线出口 / 可进入") : "任务完成后开放",
                  exit.x - 90, labelY + 33, 12, available ? color : Muted);
    }
    DrawPoly({640, 390}, 4, 36, 0, Paper);
    DrawPoly({640, 390}, 4, 29, 0, Ink);
    font.Draw(progress_.IsCompleted(node->id) ? "已完成" : "行动中", 613, 435, 15, Paper);
    DrawCircleV(explorer_, 11, Blue);
    DrawCircleLines(int(explorer_.x), int(explorer_.y), 15, Paper);
    if (walkingToPointer_) DrawCircleLines(int(walkTarget_.x), int(walkTarget_.y), 10, Fade(Paper, .65F));
    Button(font, MapTask, "当前行动详情  /  完成任务  >", pointer_.Hit(MapTask), Paper, Ink);
    const auto* exit = NearbyExit();
    Fit(font, exit ? "Enter 通过当前出口" : "点击地图移动 · 点击出口前往", {575, 605, 601, 27}, 18, Paper);
    Footer(font, "WASD / 方向键移动    Enter 查看任务或通过出口    鼠标点地面移动、点出口前往    返回箭头 / Esc 主页");
}

void MainMenu::DrawArchive(const UiFont& font) const {
    DrawBackground(font, "档案 / 行动记录");
    const auto entries = progress_.Archives(archiveCategory_ == 0 ? ArchiveCategory::Story : ArchiveCategory::Item);
    font.Draw("DATABASE", 58, 112, 15, Paper);
    for (int i = 0; i < 2; ++i) Button(font, Category(i), i == 0 ? "关键剧情" : "道具记录", i == archiveCategory_, i == archiveCategory_ ? Paper : Ink, i == archiveCategory_ ? Ink : Paper);
    font.Skin().Draw("rhodes", {100, 363, 148, 163}, Fade(Paper, .15F));
    font.Draw("RECORD / INDEX", 79, 572, 14, Paper);
    Surface({321, 109, 405, 518}, {34, 39, 44, 245});
    font.Draw("记录检索", 347, 127, 23, Paper);
    const int start = std::clamp(archiveSelection_ - 3, 0, std::max(0, int(entries.size()) - 7));
    for (int i = start; i < std::min(start + 7, int(entries.size())); ++i) {
        const auto& e = entries[i]; const float y = 180.0F + (i - start) * 55;
        const bool selected = i == archiveSelection_;
        DrawRectangleRec({340, y, 367, 45}, selected ? Paper : Color{49, 55, 60, 255});
        if (selected) DrawRectangle(340, int(y), 4, 45, Blue);
        font.Draw(TextFormat("%02d", i + 1), 351, y + 15, 14, selected ? Ink : Muted);
        Fit(font, e.unlocked ? e.name : "???? / 无法检索", {386, y, 307, 45}, 19, selected ? Ink : e.unlocked ? Paper : Muted);
    }
    font.Draw("滚轮 / ↑↓ 翻阅", 350, 592, 14, Muted);
    Surface({747, 109, 477, 518}, Paper);
    DrawRectangle(747, 109, 477, 49, Ink);
    font.Draw("ARCHIVE / DOCUMENT", 775, 126, 15, Paper);
    if (!entries.empty()) {
        const auto& e = entries[archiveSelection_];
        font.Draw(TextFormat("FILE  %03d", archiveSelection_ + 1), 779, 190, 14, {105, 113, 118, 255});
        Wrap(font, e.unlocked ? e.name : "记录损坏", {778, 228, 406, 84}, 30, Ink);
        Rule(778, 327, 402, Ink);
        Wrap(font, e.unlocked ? e.description : "????\n记录尚未恢复，无法检索。", {778, 353, 405, 170}, 21, Ink);
        if (e.unlocked) font.Draw((std::string("来源 / ") + SourceName(e.source)).c_str(), 778, 555, 18, Ink);
        font.Draw(TextFormat("%02d / %02d", archiveSelection_ + 1, int(entries.size())), 1083, 593, 16, Ink);
    }
    Footer(font, "点击分类与条目    ←→ / A D 切换分类    ↑↓ / W S / 滚轮选择记录    返回箭头 / Esc 主页");
}

void MainMenu::DrawEquipment(const UiFont& font) const {
    DrawBackground(font, "装备 / 战术配置");
    font.Draw("LOADOUT", 63, 111, 18, Paper);
    font.Draw("任意类型可放入任一装备栏", 826, 113, 19, Paper);
    for (int i = 0; i < 3; ++i) {
        const auto* item = progress_.FindEquipment(progress_.Slots()[i]);
        const auto r = Rectangle{62.0F + i * 403, 151, 378, 105};
        const Color color = item ? Accent(*item) : Muted;
        Surface(r, i == equipmentSlot_ ? Paper : Ink);
        DrawRectangleRec({r.x, r.y + 101, r.width, 4}, i == equipmentSlot_ ? Blue : color);
        font.Draw(TextFormat("SLOT %02d", i + 1), r.x + 18, r.y + 14, 13, i == equipmentSlot_ ? Ink : Muted);
        Fit(font, item ? item->name : "未装备", {r.x + 18, r.y + 41, 332, 35}, 28, i == equipmentSlot_ ? Ink : Paper);
    }
    Surface({62, 281, 548, 348}, Ink);
    Surface({631, 281, 587, 348}, Paper);
    font.Draw("持有装备", 83, 299, 23, Paper);
    const auto entries = progress_.Equipment();
    const EquipmentEntry* selected = nullptr; int row = 0;
    const int start = std::max(0, equipmentSelection_ - 3);
    for (const auto& item : entries) if (item.owned) {
        if (row == equipmentSelection_) selected = &item;
        if (row >= start && row < start + 6) {
            const float y = 343.0F + (row - start) * 43;
            const bool active = row == equipmentSelection_;
            DrawRectangleRec({78, y, 516, 39}, active ? Color{66, 75, 82, 255} : Color{38, 44, 49, 255});
            DrawRectangle(78, int(y), 4, 39, Accent(item));
            font.Draw(KindName(item.kind), 93, y + 9, 17, Accent(item));
            Fit(font, item.name, {158, y, 418, 39}, 20, Paper);
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
    font.Draw("PREFERENCES", 84, 118, 17, Paper);
    const char* labels[] = {"总音量", "音效音量", "自定义键位", "重置游戏进度", "返回主页"};
    for (int i = 0; i < 5; ++i) {
        const float y = 166.0F + i * 88;
        const bool selected = i == settingsSelection_;
        Surface({84, y, 1115, 71}, selected ? Paper : Ink);
        const auto text = selected ? Ink : Paper;
        if (selected) DrawRectangle(84, int(y), 5, 71, Blue);
        font.Draw(labels[i], 112, y + 19, 27, text);
        if (i < 2) {
            const int value = i == 0 ? settings_.MasterVolume() : settings_.SoundVolume();
            DrawRectangle(690, int(y + 33), 345, 4, Muted);
            DrawRectangle(690, int(y + 33), 345 * value / 100, 4, Blue);
            DrawRectangle(685 + 345 * value / 100, int(y + 22), 10, 27, selected ? Ink : Paper);
            font.Draw(TextFormat("%d%%", value), 1060, y + 21, 25, text);
        } else font.Draw(i == 3 ? "需再次确认  >" : "打开  >", 963, y + 24, 19, i == 3 ? Orange : text);
    }
    Footer(font, "点击条目或拖动音量滑块    ↑↓ 选择    ←→ 调整音量    Enter 打开    返回箭头 / Esc 主页");
}
void MainMenu::DrawKeyBindings(const UiFont& font) const {
    DrawBackground(font, "设置 / 自定义键位");
    font.Draw("战斗操作", 83, 109, 28, Paper);
    font.Draw("菜单固定使用方向键 / WASD、Enter 与 Esc", 628, 118, 18, Paper);
    Surface({81, 146, 1115, 483}, Ink);
    for (int i = 0; i < GameSettings::kActionCount; ++i) {
        const float y = 152.0F + i * 43;
        const bool selected = i == keySelection_;
        if (selected) DrawRectangle(81, int(y - 3), 1115, 40, Paper);
        font.Draw(GameSettings::ActionName(static_cast<GameAction>(i)), 113, y + 3, 21, selected ? Ink : Paper);
        const auto value = capturingKey_ && selected ? "按下新键…" : GameSettings::KeyName(settings_.Key(static_cast<GameAction>(i)));
        font.Draw(value.c_str(), 849, y + 3, 22, selected ? Ink : Paper);
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
