#include "main_menu.h"
#include "character_art.h"
#include "file_path.h"
#include "game_types.h"
#include "ui_font.h"
#include "ui_theme.h"
#include "terminal_ui.h"

#include <algorithm>
#include <cmath>

namespace {
bool Up() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W); }
bool Down() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S); }
bool Left() { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A); }
bool Right() { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D); }

}

MainMenu::MainMenu() : MainMenu(Utf8Path(GetApplicationDirectory())) {}
MainMenu::MainMenu(const std::filesystem::path& applicationDirectory)
    : progress_(applicationDirectory), settings_(applicationDirectory) { SyncError(); }
void MainMenu::SyncError() {
    const auto& error = !settings_.Error().empty() ? settings_.Error() : progress_.Error();
    if (!error.empty() && status_ != error) { status_ = error; TraceLog(LOG_WARNING, "%s", status_.c_str()); }
}
void MainMenu::OpenHome() { progress_.SetProtocolSuppressed(false); page_ = Page::Home; }

void MainMenu::MoveExplorer(float deltaTime) {
    float x = static_cast<float>(IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) -
              static_cast<float>(IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT));
    float y = static_cast<float>(IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) -
              static_cast<float>(IsKeyDown(KEY_W) || IsKeyDown(KEY_UP));
    const float length = std::sqrt(x * x + y * y);
    if (length > 0.0F) { x /= length; y /= length; walkingToPointer_ = false; pendingExit_.clear(); }
    if (walkingToPointer_) {
        const float dx = walkTarget_.x - explorer_.x, dy = walkTarget_.y - explorer_.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= 350.0F * deltaTime) { explorer_ = walkTarget_; walkingToPointer_ = false; }
        else { x = dx / distance; y = dy / distance; }
    }
    explorer_.x = std::clamp(explorer_.x + x * 350.0F * deltaTime, 195.0F, 1100.0F);
    explorer_.y = std::clamp(explorer_.y + y * 350.0F * deltaTime, 215.0F, 555.0F);
}

const MapExit* MainMenu::NearbyExit() const {
    const auto* node = progress_.CurrentNode(); if (!node) return nullptr;
    for (const auto& exit : node->exits) if (progress_.ExitVisible(exit)) {
        const float dx = explorer_.x - exit.x, dy = explorer_.y - exit.y;
        if (dx * dx + dy * dy < 75.0F * 75.0F) return &exit;
    }
    return nullptr;
}

MenuAction MainMenu::Update(float deltaTime) {
    pointer_ = ReadUiPointer(touchPreviouslyDown_);
    if (page_ != Page::Splash && page_ != Page::Home && pointer_.Clicked(TerminalUi::Back)) {
        if (page_ == Page::KeyBindings && capturingKey_) { capturingKey_ = false; return MenuAction::None; }
        progress_.SetProtocolSuppressed(false);
        walkingToPointer_ = false; pendingExit_.clear();
        if (page_ == Page::KeyBindings || page_ == Page::ConfirmReset) page_ = Page::Settings;
        else if (page_ == Page::NodeDetail || page_ == Page::FirstReset || page_ == Page::HiddenBoss || page_ == Page::Ending) page_ = Page::Map;
        else page_ = Page::Home;
        return MenuAction::None;
    }
    // TODO: Remove development shortcuts when progression can be earned through full levels.
    if (IsKeyPressed(KEY_F6)) { progress_.AddCompliance(); status_ = "调试：PRTS 遵循度 +1"; }
    if (IsKeyPressed(KEY_F7)) { progress_.AddTruth(); status_ = "调试：真相发现度 +1"; }
    if (IsKeyPressed(KEY_F8)) { progress_.UnlockAllNodes(); status_ = "调试：全部节点已解锁"; }
    if (IsKeyPressed(KEY_F10)) { progress_.DebugNormalReady(); explorer_ = {640, 390}; status_ = "调试：已准备普通结局"; }
    if (IsKeyPressed(KEY_F11)) { progress_.DebugHiddenReady(); explorer_ = {640, 390}; status_ = "调试：已准备隐藏结局"; }
    SyncError();
    switch (page_) {
    case Page::Splash:
        if (IsKeyPressed(KEY_ESCAPE)) return MenuAction::Quit;
        if (IsKeyPressed(KEY_ENTER) || pointer_.pressed) page_ = Page::Home;
        break;
    case Page::Home:
        if (IsKeyPressed(KEY_B)) return MenuAction::StartBattle;
        if (Up() || Down()) {
            for (int i = 0; i < 6; ++i) if (TerminalUi::HomeOrder[i] == homeSelection_) {
                homeSelection_ = TerminalUi::HomeOrder[(i + (Up() ? 5 : 1)) % 6];
                break;
            }
        }
        if (IsKeyPressed(KEY_ESCAPE)) return MenuAction::Quit;
        for (int i = 0; i < 6; ++i) if (pointer_.Clicked(TerminalUi::HomeButtons[i])) homeSelection_ = i;
        if (IsKeyPressed(KEY_ENTER) || pointer_.Clicked(TerminalUi::HomeButtons[homeSelection_])) {
            if (homeSelection_ == 0) page_ = Page::Map;
            else if (homeSelection_ == 1) page_ = Page::Archive;
            else if (homeSelection_ == 2) page_ = Page::Equipment;
            else if (homeSelection_ == 3) page_ = Page::Settings;
            else if (homeSelection_ == 4) page_ = Page::Map;
            else return MenuAction::Quit;
        }
        break;
    case Page::Map:
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Home; walkingToPointer_ = false; pendingExit_.clear(); }
        else {
            if (pointer_.Clicked(TerminalUi::MapTask)) { page_ = Page::NodeDetail; walkingToPointer_ = false; pendingExit_.clear(); break; }
            if (pointer_.Clicked(TerminalUi::MapArea)) {
                walkTarget_ = {std::clamp(pointer_.position.x, 195.0F, 1100.0F), std::clamp(pointer_.position.y, 215.0F, 555.0F)};
                walkingToPointer_ = true; pendingExit_.clear();
                if (const auto* node = progress_.CurrentNode()) for (const auto& exit : node->exits) {
                    if (!progress_.ExitVisible(exit)) continue;
                    const float labelY = exit.y < 240 ? exit.y + 30 : exit.y - 64;
                    if (pointer_.Hit({exit.x - 27, exit.y - 27, 54, 54}) || pointer_.Hit({exit.x - 104, labelY, 208, 29})) {
                        walkTarget_ = {exit.x, exit.y}; pendingExit_ = exit.id; break;
                    }
                }
            }
            MoveExplorer(deltaTime);
            const auto* nearby = NearbyExit();
            const bool autoEnter = nearby && !pendingExit_.empty() && nearby->id == pendingExit_;
            if (IsKeyPressed(KEY_ENTER) || autoEnter) {
                if (nearby) {
                    if (progress_.Travel(nearby->id)) { explorer_ = {640, 390}; status_ = "已通过出口进入下一地图"; }
                    else status_ = "出口尚未开放：请先完成当前任务或调查线索";
                } else page_ = Page::NodeDetail;
                walkingToPointer_ = false; pendingExit_.clear();
            }
        }
        break;
    case Page::Archive: {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Home; break; }
        if (pointer_.Clicked(TerminalUi::Category(0))) { archiveCategory_ = 0; archiveSelection_ = 0; }
        if (pointer_.Clicked(TerminalUi::Category(1))) { archiveCategory_ = 1; archiveSelection_ = 0; }
        if (Left() || Right()) { archiveCategory_ = 1 - archiveCategory_; archiveSelection_ = 0; }
        const auto entries = progress_.Archives(archiveCategory_ == 0 ? ArchiveCategory::Story : ArchiveCategory::Item);
        if (entries.empty()) break;
        const int start = std::clamp(archiveSelection_ - 3, 0, std::max(0, static_cast<int>(entries.size()) - 7));
        for (int i = start; i < std::min(start + 7, static_cast<int>(entries.size())); ++i)
            if (pointer_.Clicked({340, 180.0F + (i - start) * 55.0F, 367, 45})) { archiveSelection_ = i; break; }
        const float wheel = GetMouseWheelMove();
        if (Up() || wheel > 0) archiveSelection_ = (archiveSelection_ + static_cast<int>(entries.size()) - 1) % static_cast<int>(entries.size());
        if (Down() || wheel < 0) archiveSelection_ = (archiveSelection_ + 1) % static_cast<int>(entries.size());
        break;
    }
    case Page::Equipment: {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Home; break; }
        for (int i = 0; i < 3; ++i) if (pointer_.Clicked({62.0F + i * 403, 151, 378, 105})) equipmentSlot_ = i;
        if (Left()) equipmentSlot_ = (equipmentSlot_ + 2) % 3;
        if (Right()) equipmentSlot_ = (equipmentSlot_ + 1) % 3;
        const auto entries = progress_.Equipment();
        int count = 0; for (const auto& entry : entries) if (entry.owned) ++count;
        const int start = std::max(0, equipmentSelection_ - 3);
        int visibleRow = 0;
        for (const auto& entry : entries) if (entry.owned) {
            if (visibleRow >= start && visibleRow < start + 6 && pointer_.Clicked({78, 343.0F + (visibleRow - start) * 43, 516, 39})) equipmentSelection_ = visibleRow;
            ++visibleRow;
        }
        const float wheel = GetMouseWheelMove();
        if (count > 0 && (Up() || wheel > 0)) equipmentSelection_ = (equipmentSelection_ + count - 1) % count;
        if (count > 0 && (Down() || wheel < 0)) equipmentSelection_ = (equipmentSelection_ + 1) % count;
        if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_X) || pointer_.Clicked(TerminalUi::Unequip)) progress_.Equip(equipmentSlot_, "");
        if (IsKeyPressed(KEY_ENTER) || pointer_.Clicked(TerminalUi::Equip)) { int i = 0; for (const auto& entry : entries) if (entry.owned && i++ == equipmentSelection_) { progress_.Equip(equipmentSlot_, entry.id); break; } }
        break;
    }
    case Page::Settings:
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Home; break; }
        for (int i = 0; i < 5; ++i) if (pointer_.Clicked({84, 166.0F + i * 88, 1115, 71})) settingsSelection_ = i;
        if (pointer_.down && pointer_.Hit({680, 181, 365, 43})) settingsSelection_ = 0;
        if (pointer_.down && pointer_.Hit({680, 269, 365, 43})) settingsSelection_ = 1;
        if (Up()) settingsSelection_ = (settingsSelection_ + 4) % 5;
        if (Down()) settingsSelection_ = (settingsSelection_ + 1) % 5;
        if (settingsSelection_ < 2) {
            const int step = Right() ? 5 : Left() ? -5 : 0;
            if (step) { if (settingsSelection_ == 0) settings_.AdjustMaster(step); else settings_.AdjustSound(step); }
        }
        if (settingsSelection_ < 2 && pointer_.down && pointer_.Hit({680, 181.0F + settingsSelection_ * 88, 365, 43})) {
            const int value = std::clamp(static_cast<int>((pointer_.position.x - 690.0F) / 345.0F * 100.0F), 0, 100);
            if (settingsSelection_ == 0 && value != settings_.MasterVolume()) settings_.AdjustMaster(value - settings_.MasterVolume());
            else if (settingsSelection_ == 1 && value != settings_.SoundVolume()) settings_.AdjustSound(value - settings_.SoundVolume());
        }
        if (IsKeyPressed(KEY_ENTER) || pointer_.Clicked({84, 166.0F + settingsSelection_ * 88, 1115, 71})) {
            if (settingsSelection_ == 2) page_ = Page::KeyBindings;
            else if (settingsSelection_ == 3) page_ = Page::ConfirmReset;
            else if (settingsSelection_ == 4) page_ = Page::Home;
        }
        break;
    case Page::KeyBindings:
        if (capturingKey_) {
            const int key = GetKeyPressed();
            if (key == KEY_ESCAPE) { capturingKey_ = false; status_ = "已取消改键"; }
            else if (key != 0) {
                if (settings_.Bind(static_cast<GameAction>(keySelection_), key)) {
                    capturingKey_ = false; status_ = "键位已保存";
                } else status_ = "此键不可用或已被其他操作占用";
            }
            break;
        }
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Settings; break; }
        if (Up()) keySelection_ = (keySelection_ + GameSettings::kActionCount - 1) % GameSettings::kActionCount;
        if (Down()) keySelection_ = (keySelection_ + 1) % GameSettings::kActionCount;
        for (int i = 0; i < GameSettings::kActionCount; ++i)
            if (pointer_.Clicked({81, 149.0F + i * 43, 1115, 40})) keySelection_ = i;
        if (IsKeyPressed(KEY_ENTER) || pointer_.Clicked({81, 149.0F + keySelection_ * 43, 1115, 40})) { capturingKey_ = true; status_ = "请按新键；Esc 取消"; }
        break;
    case Page::ConfirmReset:
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Settings; break; }
        if (IsKeyPressed(KEY_ENTER) || pointer_.Clicked(TerminalUi::ResetConfirm)) {
            const bool saved = progress_.ResetProgress();
            archiveSelection_ = equipmentSelection_ = 0;
            status_ = saved ? "游戏进度已重置" : progress_.Error();
            page_ = Page::Settings;
        }
        break;
    case Page::NodeDetail: {
        // TODO: Replace this confirmation with each node's battle and reward flow.
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { page_ = Page::Map; break; }
        if (!IsKeyPressed(KEY_ENTER) && !pointer_.Clicked(TerminalUi::Confirm)) break;
        const auto id = progress_.SelectedNode();
        if (progress_.IsCompleted(id)) { page_ = Page::Map; break; }
        if (id == "ending_prts_core" && !progress_.IsCompleted(id)) {
            if (!progress_.CanCompleteCurrent()) { status_ = "尚未完成隐藏路线入口"; break; }
            progress_.SetProtocolSuppressed(true); page_ = Page::HiddenBoss; break;
        }
        if (!progress_.CompleteNode(id)) {
            status_ = progress_.IsCompleted(id) ? "该地点已完成，请从出口继续" :
                      "尚未满足完成条件；试炼需卸下全部协议";
            break;
        }
        if (id == "ending_normal" || id == "ending_hidden") {
            ending_ = id == "ending_normal" ? EndingType::Normal : EndingType::Hidden;
            page_ = Page::Ending;
        } else if (id == "prologue_defeat") page_ = Page::FirstReset;
        else { status_ = "地点已完成；返回地图寻找新出口"; page_ = Page::Map; }
        break;
    }
    case Page::FirstReset:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || pointer_.Clicked(TerminalUi::Confirm)) page_ = Page::Map;
        break;
    case Page::HiddenBoss:
        // TODO: Connect PRTS boss attacks and victory to the real battle module.
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) { progress_.SetProtocolSuppressed(false); page_ = Page::Map; }
        else if (IsKeyPressed(KEY_ENTER) || pointer_.Clicked(TerminalUi::Confirm)) {
            const bool completed = progress_.CompleteNode("ending_prts_core");
            progress_.SetProtocolSuppressed(false);
            if (!completed || !progress_.Travel("to_hidden_ending")) {
                page_ = Page::Map; status_ = "行动未能完成，请检查当前任务条件或存档提示"; break;
            }
            explorer_ = {640, 390};
            page_ = Page::NodeDetail;
        }
        break;
    case Page::Ending:
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || pointer_.Clicked(TerminalUi::Confirm)) page_ = Page::Map;
        break;
    }
    SyncError(); return MenuAction::None;
}
