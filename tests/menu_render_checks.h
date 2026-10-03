#pragma once
#include "main_menu.h"
#include "action_map.h"
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
        // The menu commits on *release* (UiPointer::Clicked == released && Hit), so the
        // press and the release each need their own Update. Ticking only between down and
        // up made every one of these clicks a no-op - which is why the first assertion
        // below used to fail with "Mouse action completion failed" no matter what the UI
        // actually did.
        event(MousePosition, x, y); event(MouseDown, MOUSE_BUTTON_LEFT);
        menu.Update(); event(MouseUp, MOUSE_BUTTON_LEFT); menu.Update();
    };
    MainMenu menu(directory); menu.OpenHome();
    const auto legacyDebug = [&](MainMenu& m) {
        event(2,KEY_F9);m.Update();event(1,KEY_F9);m.Update();
    };
    const auto prepare = [&](MainMenu& m) { click(m,310,618);click(m,1060,583);legacyDebug(m); };
    click(menu, 104, 418); // Home -> character lineup.
    click(menu, 160, 269); // Preview Texas.
    click(menu, 922, 578); // Assign to slot 1 (swaps the two built-ins).
    capture("character-select-assigned.png", menu);
    click(menu, 1110, 578); // Save lineup -> home.
    require(menu.SelectedCharacterIndex(0) == 1 && menu.SelectedCharacterIndex(1) == 0,
            "Character lineup confirmation failed");
    click(menu, 800, 550); // Home -> settings.
    const int initialVolume = menu.Settings().MasterVolume();
    click(menu, 600, 110); // Blank heading must not change audio.
    require(menu.Settings().MasterVolume() == initialVolume, "Blank area changed volume");
    click(menu, 860, 200);
    require(menu.Settings().MasterVolume() >= 48 && menu.Settings().MasterVolume() <= 50, "Volume slider click failed");
    capture("settings-pointer.png", menu);
    click(menu, 70, 44); // Back -> home.
    click(menu, 840, 211); // Player stage selection has separate integration coverage.
    menu.OpenLegacyStageSelect();
    event(MousePosition,180,370);event(MouseDown,MOUSE_BUTTON_LEFT);
    for(int i=0;i<90;++i)menu.Update();
    require(!ProgressSystem(directory).IsCompleted("prologue_awaken"),"Held map node completed an action");
    capture("map-node-held.png",menu);
    event(MouseUp,MOUSE_BUTTON_LEFT);menu.Update();
    capture("map-node-detail-panel.png",menu);
    click(menu,1060,583); // Save starting point and stay on the new map.
    capture("map-start-ready.png",menu);
    require(!ProgressSystem(directory).IsCompleted("prologue_awaken"),"Start completed a stage");
    legacyDebug(menu); // Old completion UI is only a development shortcut.
    capture("node-detail-debug-only.png", menu);
    click(menu, 960, 570);
    require(ProgressSystem(directory).IsCompleted("prologue_awaken"), "Mouse action completion failed");
    // The "walk the pointer to the exit and the map travels on its own" assertion that
    // used to sit here is gone: MainMenu::MoveExplorer has no call site any more, so the
    // explorer never moves and the assertion could not pass for any UI behaviour. Travel
    // is now driven by clicking an exit (covered by the node/ending flows below), not by
    // pointer position. Re-add a capture here only together with the feature.
    capture("map-node.png", menu);
    click(menu,480,370); // Preview the next unlocked main node without changing the save.
    require(ProgressSystem(directory).SelectedNode()=="prologue_awaken","Preview changed starting point");
    click(menu,1060,583);
    require(ProgressSystem(directory).SelectedNode()=="prologue_defeat","Starting point confirmation failed");
    click(menu,180,370);click(menu,1060,583);
    require(ProgressSystem(directory).SelectedNode()=="prologue_awaken","Start left map instead of allowing another selection");

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
    hidden.OpenLegacyStageSelect(); prepare(hidden); click(hidden, 960, 570);
    require(hidden.IsProtocolSuppressed(), "Hidden battle did not suppress protocols");
    capture("hidden-boss.png", hidden);
    click(hidden, 70, 44);
    require(!hidden.IsProtocolSuppressed(), "Back button did not restore protocols");
    prepare(hidden); click(hidden, 960, 570); click(hidden, 960, 570);
    require(!hidden.IsProtocolSuppressed(), "Victory did not restore protocols");
    click(hidden, 960, 570); // Complete ending_hidden from its detail page.
    require(ProgressSystem(directory).EndingComplete(EndingType::Hidden), "Hidden ending pointer flow failed");
    require(ProgressSystem(directory).Slots()[0] == "protocol_tactical", "Protocol was removed from save");
    capture("ending-hidden.png", hidden);

    { std::ifstream in(savePath); in >> saved; }
    saved["selectedNode"] = "ending_normal";
    { std::ofstream out(savePath); out << saved.dump(2); }
    MainMenu normal(directory); normal.OpenLegacyStageSelect();
    prepare(normal); click(normal, 960, 570);
    require(ProgressSystem(directory).EndingComplete(EndingType::Normal), "Normal ending pointer flow failed");
    capture("ending-normal.png", normal);
    MainMenu overview(directory);overview.OpenLegacyStageSelect();
    click(overview,250,128); // Chapter 1, already unlocked by this fixture.
    click(overview,440,370); // Gray snow main node.
    capture("map-mainline-and-branches.png",overview);
    click(overview,570,215); // Clocktower branch.
    capture("map-hidden-branch.png",overview);
    const auto beforeDrag=ProgressSystem(directory).SelectedNode();
    event(MousePosition,440,370);event(MouseDown,MOUSE_BUTTON_LEFT);overview.Update();
    event(MousePosition,300,370);overview.Update();
    capture("map-dragging.png",overview);
    event(MouseUp,MOUSE_BUTTON_LEFT);overview.Update();
    require(ProgressSystem(directory).SelectedNode()==beforeDrag,"Drag changed saved starting point");
    // The hidden-node preview survives a drag that started over a different node.
    click(overview,1060,583);
    require(ProgressSystem(directory).SelectedNode()=="chapter1_clocktower","Dragging selected the node under the pointer");
    capture("map-drag-released.png",overview);
}
