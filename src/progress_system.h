#pragma once

#include "story_data.h"
#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

using EquipmentEntry = EquipmentItem;

struct GameProgress {
    int version = 2;
    std::set<std::string> visitedRegions;
    bool wardShortcutOpen = false;
    std::string selectedRegion = "clinic";
    std::set<std::string> unlockedNodes{"prologue_awaken"};
    std::set<std::string> completedNodes;
    std::string currentNode = "prologue_awaken";
    std::set<std::string> archiveIds;
    std::set<std::string> equipmentIds;
    std::array<std::string, 3> slots{};
    int prtsCompliance = 0;
    int truthDiscovery = 0;
    int memoryIntegrity = 3;
    bool normalComplete = false;
    bool hiddenComplete = false;
};

struct EquipmentModifiers {
    // TODO: Replace description-only effects with typed battle modifiers.
    std::vector<std::string> effectDescriptions;
};

class ProgressSystem {
public:
    explicit ProgressSystem(std::filesystem::path applicationDirectory);
    const std::vector<StoryChapter>& Chapters() const { return StoryData::Chapters(); }
    const std::vector<MapNode>& Nodes() const { return StoryData::Nodes(); }
    std::vector<ArchiveEntry> Archives(ArchiveCategory category) const;
    std::vector<EquipmentItem> Equipment() const;
    const MapNode* FindNode(const std::string& id) const { return StoryData::FindNode(id); }
    const EquipmentItem* FindEquipment(const std::string& id) const { return StoryData::FindEquipment(id); }
    const MapNode* CurrentNode() const { return FindNode(progress_.currentNode); }
    const std::string& SelectedNode() const { return progress_.currentNode; }
    bool SelectNode(const std::string& id);
    bool RegionVisited(const std::string& id) const { return progress_.visitedRegions.contains(id); }
    bool RegionVisible(const std::string& id) const;
    bool RegionAvailable(const std::string& id) const;
    bool VisitRegion(const std::string& id, const std::string& from = "");
    bool WardShortcutOpen() const { return progress_.wardShortcutOpen; }
    bool SelectRegion(const std::string& id);
    const std::string& SelectedRegion() const { return progress_.selectedRegion; }
    const std::array<std::string, 3>& Slots() const { return progress_.slots; }
    NodeState State(const std::string& id) const;
    bool IsUnlocked(const std::string& id) const { return progress_.unlockedNodes.contains(id); }
    bool IsCompleted(const std::string& id) const { return progress_.completedNodes.contains(id); }
    bool HasArchive(const std::string& id) const { return progress_.archiveIds.contains(id); }
    bool HasEquipment(const std::string& id) const { return progress_.equipmentIds.contains(id); }
    std::string UnlockConditionText(const std::string& id) const;
    bool ExitVisible(const MapExit& exit) const;
    bool ExitAvailable(const MapExit& exit) const;
    bool Travel(const std::string& exitId);
    bool CanCompleteCurrent() const;
    bool CompleteNode(const std::string& id);
    bool Equip(int slot, const std::string& id);
    bool IsProtocolSuppressed() const { return protocolSuppressed_; }
    void SetProtocolSuppressed(bool value) { protocolSuppressed_ = value; }
    EquipmentModifiers CalculateEquipmentModifiers() const;
    int Compliance() const { return progress_.prtsCompliance; }
    int Truth() const { return progress_.truthDiscovery; }
    int MemoryIntegrity() const { return progress_.memoryIntegrity; }
    int OwnedEchoCount() const;
    bool HiddenRequirementsMet() const;
    bool EndingComplete(EndingType type) const;
    void AddCompliance();
    void AddTruth();
    void UnlockAllNodes();
    void DebugNormalReady();
    void DebugHiddenReady();
    bool Save();
    bool Load();
    bool ResetProgress();
    const std::string& Error() const { return error_; }
    static constexpr int kHiddenTruthThreshold = 6;

private:
    void RevealAvailable();
    bool ConditionMet(const MapNode& source, const MapExit& exit) const;
    bool HasEquippedProtocol() const;
    bool Validate(const GameProgress& candidate) const;
    void LoadLegacy(const std::string& serialized);
    std::filesystem::path savePath_;
    GameProgress progress_;
    bool protocolSuppressed_ = false;
    bool legacyPending_ = false;
    std::string error_;
};
