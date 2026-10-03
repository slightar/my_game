#include "progress_system.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>

void Step(ProgressSystem& p, const char* exitId, const char* target) {
    assert(p.Travel(exitId)); assert(p.SelectedNode() == target);
}
void Complete(ProgressSystem& p, const char* node) {
    assert(p.SelectedNode() == node); assert(p.CompleteNode(node));
}

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("my_game_progress_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    {
        const auto city=root/"city";
        ProgressSystem p(city);
        assert(p.RegionAvailable("clinic") && !p.RegionAvailable("gray"));
        assert(!p.RegionVisible("clock") && !p.SelectRegion("clock"));
        assert(p.VisitRegion("clinic"));
        assert(p.RegionAvailable("gray"));
        assert(p.SelectRegion("gray") && !p.RegionVisited("gray"));
        assert(p.VisitRegion("gray"));
        assert(!p.RegionVisible("clock") && !p.RegionVisible("ward"));
        p.UnlockAllNodes(); // Legacy/debug unlocks must never reveal city secrets.
        assert(!p.RegionVisible("clock"));
        assert(p.VisitRegion("clock") && p.RegionVisible("clock"));
        assert(p.RegionAvailable("clock") && !p.VisitRegion("bogus"));
        ProgressSystem loaded(city);
        assert(loaded.RegionVisited("clock") && loaded.SelectedRegion()=="clock");
        assert(!loaded.RegionVisible("ward"));
        assert(!loaded.WardShortcutOpen());
        assert(!loaded.VisitRegion("ward","clinic"));
        assert(!loaded.RegionVisited("ward"));
        assert(loaded.VisitRegion("ward","station"));
        assert(!loaded.WardShortcutOpen());
        assert(loaded.VisitRegion("clinic","ward"));
        assert(loaded.WardShortcutOpen());
        ProgressSystem shortcutReloaded(city);
        assert(shortcutReloaded.WardShortcutOpen());
        assert(shortcutReloaded.VisitRegion("ward","clinic"));
        assert(loaded.ResetProgress());
        assert(!loaded.WardShortcutOpen());
        assert(!loaded.RegionVisible("clock") && loaded.SelectedRegion()=="clinic");
    }
    {
        std::set<std::string> ids;
        for (const auto& node : StoryData::Nodes()) {
            assert(ids.insert(node.id).second);
            assert(StoryData::FindChapter(node.chapterId));
            for (const auto& exit : node.exits) assert(StoryData::FindNode(exit.targetId));
            for (const auto& archive : node.reward.archiveIds) assert(StoryData::FindArchive(archive));
            for (const auto& item : node.reward.equipmentIds) assert(StoryData::FindEquipment(item));
        }
        assert(ids.size() == 33);
    }
    {
        ProgressSystem p(root);
        assert(p.SelectedNode() == "prologue_awaken");
        assert(p.State("prologue_hidden_vent") == NodeState::Hidden);
        assert(!p.Travel("awakening_route"));
        Complete(p, "prologue_awaken");
        Step(p, "broken_vent", "prologue_hidden_vent"); Complete(p, "prologue_hidden_vent");
        assert(p.HasArchive("unregistered_patient"));
        Step(p, "vent_return", "prologue_awaken");
        Step(p, "awakening_route", "prologue_defeat"); Complete(p, "prologue_defeat");
        Step(p, "reset_signal", "chapter1_reconnect"); Complete(p, "chapter1_reconnect");
        assert(p.HasEquipment("protocol_tactical"));
        Step(p, "to_gray_snow", "chapter1_gray_snow"); Complete(p, "chapter1_gray_snow");
        Step(p, "clock_roof", "chapter1_clocktower"); Complete(p, "chapter1_clocktower");
        Step(p, "clock_merge", "chapter1_gray_snow");
        Step(p, "mall_basement", "chapter1_train"); Complete(p, "chapter1_train");
        Step(p, "train_merge", "chapter1_gray_snow");
        Step(p, "bridge_exit", "chapter1_crownslayer"); Complete(p, "chapter1_crownslayer");
        Step(p, "to_refuge", "chapter2_white_refuge"); Complete(p, "chapter2_white_refuge");
        Step(p, "heat_pipe", "chapter2_snow_lamp"); Complete(p, "chapter2_snow_lamp");
        Step(p, "lamp_merge", "chapter2_white_refuge");
        Step(p, "blizzard_crack", "chapter2_whitefield"); Complete(p, "chapter2_whitefield");
        Step(p, "whitefield_merge", "chapter2_white_refuge");
        Step(p, "frozen_main", "chapter2_frostnova"); Complete(p, "chapter2_frostnova");
        Step(p, "to_longmen", "chapter3_silent_district"); Complete(p, "chapter3_silent_district");
        Step(p, "garden_ladder", "chapter3_rooftop_garden"); Complete(p, "chapter3_rooftop_garden");
        Step(p, "garden_merge", "chapter3_silent_district");
        Step(p, "theater_street", "chapter3_theater"); Complete(p, "chapter3_theater");
        assert(!p.Travel("screen_alley"));
        Step(p, "trial_stage", "chapter3_amiya_trial");
        assert(p.Equip(0, "protocol_tactical"));
        assert(!p.CompleteNode("chapter3_amiya_trial"));
        assert(p.Equip(0, "")); Complete(p, "chapter3_amiya_trial");
        Step(p, "trial_merge", "chapter3_theater");
        Step(p, "screen_alley", "chapter3_ad_screen"); Complete(p, "chapter3_ad_screen");
        Step(p, "screen_merge", "chapter3_theater");
        Step(p, "relay_road", "chapter4_relay_station"); Complete(p, "chapter4_relay_station");
        Step(p, "zero_ward_door", "chapter4_zero_ward"); Complete(p, "chapter4_zero_ward");
        Step(p, "ward_merge", "chapter4_relay_station");
        Step(p, "mon3tr_gate", "chapter4_mon3tr"); Complete(p, "chapter4_mon3tr");
        Step(p, "medical_backtrack", "chapter4_relay_station");
        Step(p, "black_archive_door", "chapter4_black_archive"); Complete(p, "chapter4_black_archive");
        Step(p, "black_merge", "chapter4_relay_station");
        Step(p, "mon3tr_gate", "chapter4_mon3tr");
        Step(p, "to_core_city", "chapter5_burning_city"); Complete(p, "chapter5_burning_city");
        Step(p, "melted_room", "chapter5_melted_command"); Complete(p, "chapter5_melted_command");
        Step(p, "melted_merge", "chapter5_burning_city");
        Step(p, "river_fault", "chapter5_originite_river"); Complete(p, "chapter5_originite_river");
        Step(p, "river_merge", "chapter5_burning_city");
        Step(p, "talulah_route", "chapter5_talulah"); Complete(p, "chapter5_talulah");
        Step(p, "standard_disposal", "ending_normal"); Complete(p, "ending_normal");
        assert(p.EndingComplete(EndingType::Normal));
        Step(p, "return_to_talulah", "chapter5_talulah");
        Step(p, "evacuation_gap", "chapter5_refusal"); Complete(p, "chapter5_refusal");
        Step(p, "refusal_merge", "chapter6_lost_archive"); Complete(p, "chapter6_lost_archive");
        assert(p.HiddenRequirementsMet());
        assert(p.IsUnlocked("chapter6_first_exit"));
        Step(p, "throne_passage", "chapter6_nameless_throne"); Complete(p, "chapter6_nameless_throne");
        Step(p, "throne_merge", "chapter6_lost_archive");
        Step(p, "deleted_audio", "chapter6_deleted_dialogue"); Complete(p, "chapter6_deleted_dialogue");
        Step(p, "dialogue_merge", "chapter6_lost_archive");
        Step(p, "first_exit_gate", "chapter6_first_exit"); Complete(p, "chapter6_first_exit");
        Step(p, "core_access", "ending_prts_core");
        assert(p.Equip(0, "protocol_tactical"));
        assert(p.Equip(1, "echo_first_call"));
        assert(p.CalculateEquipmentModifiers().effectDescriptions.size() == 2);
        p.SetProtocolSuppressed(true);
        assert(p.CalculateEquipmentModifiers().effectDescriptions.size() == 1);
        Complete(p, "ending_prts_core"); p.SetProtocolSuppressed(false);
        Step(p, "to_hidden_ending", "ending_hidden"); Complete(p, "ending_hidden");
        assert(p.EndingComplete(EndingType::Hidden));
        assert(p.HasArchive("theresa_remembered_record"));
    }
    {
        ProgressSystem restored(root);
        assert(restored.EndingComplete(EndingType::Normal));
        assert(restored.EndingComplete(EndingType::Hidden));
        assert(restored.Slots()[0] == "protocol_tactical");
        assert(!restored.IsProtocolSuppressed());
        std::ofstream(root / "save" / "game_progress.json") << "{invalid";
        assert(!restored.Load());
        assert(restored.SelectedNode() == "prologue_awaken");
        assert(restored.ResetProgress());
    }
    {
        std::ofstream(root / "save" / "game_progress.json") <<
            R"({"version":1,"prtsCompliance":2,"truthDiscovery":3,"normalComplete":true,"hiddenComplete":false,"storyRecords":["first_reset"],"itemRecords":["broken_terminal"],"ownedEquipment":["protocol_damage","echo_memory"],"slots":["protocol_damage","echo_memory",""]})";
        ProgressSystem migrated(root);
        assert(migrated.HasArchive("first_reset"));
        assert(migrated.HasArchive("unregistered_patient"));
        assert(migrated.HasEquipment("protocol_tactical"));
        assert(migrated.Slots()[0] == "protocol_tactical");
        migrated.AddTruth();
        assert(std::filesystem::exists(root / "save" / "game_progress.v1.bak"));
    }
    std::filesystem::remove(root / "save" / "game_progress.json");
    std::filesystem::remove(root / "save" / "game_progress.v1.bak");
    std::filesystem::create_directory(root / "save" / "game_progress.json");
    {
        ProgressSystem blocked(root);
        assert(!blocked.CompleteNode("prologue_awaken"));
        assert(!blocked.IsCompleted("prologue_awaken"));
        assert(!blocked.Error().empty());
    }
    std::filesystem::remove(root / "save" / "game_progress.json.tmp");
    std::filesystem::remove(root / "save" / "game_progress.json");
    std::filesystem::remove(root / "save");
    std::filesystem::remove(root / "city" / "save" / "game_progress.json");
    std::filesystem::remove(root / "city" / "save");
    std::filesystem::remove(root / "city");
    std::filesystem::remove(root);
}
