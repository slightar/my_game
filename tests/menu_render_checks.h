#pragma once
#include "main_menu.h"
#include "nlohmann/json.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

// Manual GPU regression checks, invoked by character_render_smoke.
// Playback uses raylib 5.5 automation events, never OS input or the player's save.
template<class Capture>
void CheckMenuInteraction(const std::filesystem::path& output, Capture capture) {
    const auto directory = output / "isolated-menu-state";
    std::filesystem::create_directories(directory);
    ProgressSystem fixture(directory);
    if (!fixture.ResetProgress()) throw std::runtime_error("Cannot create isolated menu fixture");
    const auto require = [](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); };
    enum : unsigned { MouseUp = 5, MouseDown = 6, MousePosition = 7 };
    const auto event = [](unsigned type, int a, int b = 0) { PlayAutomationEvent({0, type, {a, b, 0, 0}}); };
    const auto click = [&](MainMenu& menu, int x, int y) {
        event(MousePosition, x, y); event(MouseDown, MOUSE_BUTTON_LEFT);
        menu.Update(); event(MouseUp, MOUSE_BUTTON_LEFT);
    };
    MainMenu menu(directory); menu.OpenHome();
    click(menu, 800, 550); // Home -> settings.
    const int initialVolume = menu.Settings().MasterVolume();
    click(menu, 600, 110); // Blank heading must not change audio.
    require(menu.Settings().MasterVolume() == initialVolume, "Blank area changed volume");
    click(menu, 860, 200);
    require(menu.Settings().MasterVolume() >= 48 && menu.Settings().MasterVolume() <= 50, "Volume slider click failed");
    capture("settings-pointer.png", menu);
    click(menu, 70, 44); // Back -> home.
    click(menu, 840, 211); // Home -> map.
    click(menu, 310, 618); // Explicit task button -> node detail.
    capture("node-detail.png", menu);
    click(menu, 960, 570);
    require(ProgressSystem(directory).IsCompleted("prologue_awaken"), "Mouse action completion failed");
    click(menu, 1080, 370); // Walk to the main exit; must not teleport immediately.
    require(ProgressSystem(directory).SelectedNode() == "prologue_awaken", "Map pointer teleported to another node");
    for (int i = 0; i < 110; ++i) menu.Update(1.0F / 60.0F);
    require(ProgressSystem(directory).SelectedNode() == "prologue_defeat", "Walking to exit did not travel");
    capture("map-exit.png", menu);

    // Populate an isolated fixture for long text, equipment and both ending pages.
    fixture.Load(); fixture.DebugHiddenReady();
    const auto savePath = directory / "save/game_progress.json";
    nlohmann::json saved;
    { std::ifstream in(savePath); in >> saved; }
    saved["equipmentIds"] = nlohmann::json::array();
    for (const auto& item : StoryData::Equipment()) saved["equipmentIds"].push_back(item.id);
    saved["archiveIds"] = nlohmann::json::array();
    for (const auto& record : StoryData::Archives()) saved["archiveIds"].push_back(record.id);
    saved["unlockedNodes"] = nlohmann::json::array();
    for (const auto& node : StoryData::Nodes()) saved["unlockedNodes"].push_back(node.id);
    saved["slots"] = {"protocol_tactical", "relic_refusal", "echo_first_call"};
    saved["completedNodes"].push_back("chapter6_first_exit");
    saved["selectedNode"] = "ending_prts_core";
    { std::ofstream out(savePath); out << saved.dump(2); }
    MainMenu hidden(directory);
    hidden.OpenArchive(); capture("archive-unlocked.png", hidden);
    hidden.OpenEquipment();
    click(hidden, 705, 200); // Select slot 2, then unload through its explicit button.
    click(hidden, 770, 575);
    require(ProgressSystem(directory).Slots()[1].empty(), "Unequip pointer action failed");
    click(hidden, 220, 405); // Select low-temperature protocol, do not equip on selection.
    require(ProgressSystem(directory).Slots()[1].empty(), "Selecting an item equipped it without confirmation");
    click(hidden, 1065, 575);
    require(ProgressSystem(directory).Slots()[1] == "protocol_cryo", "Equip confirmation failed");
    capture("equipment-owned.png", hidden);
    hidden.OpenStageSelect(); click(hidden, 310, 618); click(hidden, 960, 570);
    require(hidden.IsProtocolSuppressed(), "Hidden battle did not suppress protocols");
    capture("hidden-boss.png", hidden);
    click(hidden, 70, 44);
    require(!hidden.IsProtocolSuppressed(), "Back button did not restore protocols");
    click(hidden, 310, 618); click(hidden, 960, 570); click(hidden, 960, 570);
    require(!hidden.IsProtocolSuppressed(), "Victory did not restore protocols");
    click(hidden, 960, 570); // Complete ending_hidden from its detail page.
    require(ProgressSystem(directory).EndingComplete(EndingType::Hidden), "Hidden ending pointer flow failed");
    require(ProgressSystem(directory).Slots()[0] == "protocol_tactical", "Protocol was removed from save");
    capture("ending-hidden.png", hidden);

    { std::ifstream in(savePath); in >> saved; }
    saved["selectedNode"] = "ending_normal";
    { std::ofstream out(savePath); out << saved.dump(2); }
    MainMenu normal(directory); normal.OpenStageSelect();
    click(normal, 310, 618); click(normal, 960, 570);
    require(ProgressSystem(directory).EndingComplete(EndingType::Normal), "Normal ending pointer flow failed");
    capture("ending-normal.png", normal);
}
