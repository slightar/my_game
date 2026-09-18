#pragma once

#include <array>
#include <string>
#include <vector>

enum class NodeType { Main, Boss, Exploration, Hidden, Ending };
enum class NodeState { Locked, Available, Completed, Hidden };
enum class ArchiveCategory { Story, Item };
enum class EquipmentKind { Relic, Echo, Protocol };
enum class EquipmentSource { Theresa, PRTS, World };
enum class EndingType { None, Normal, Hidden };

struct StoryChapter {
    std::string id, title, summary;
    int order = 0;
    std::vector<std::string> nodeIds;
};

struct ExitCondition {
    std::string requiredArchive;
    std::string requiredEquipment;
    int minTruth = 0;
    bool sourceCompleted = true;
    bool hiddenEndingRequirements = false;
};

struct MapExit {
    std::string id, label, targetId, hint;
    float x = 0.0F, y = 0.0F;
    bool hidden = false;
    ExitCondition condition;
};

struct NodeReward {
    std::vector<std::string> archiveIds;
    std::vector<std::string> equipmentIds;
    int truth = 0, compliance = 0, memory = 0;
};

struct MapNode {
    std::string id, chapterId, name, location, objective, relatedCharacters, description;
    NodeType type = NodeType::Main;
    EquipmentSource source = EquipmentSource::World;
    bool requiresNoProtocol = false;
    NodeReward reward;
    std::vector<MapExit> exits;
};

struct ArchiveEntry {
    std::string id, name, description;
    ArchiveCategory category = ArchiveCategory::Story;
    EquipmentSource source = EquipmentSource::World;
    unsigned int color = 0;
    bool unlocked = false;
};

struct EquipmentItem {
    std::string id, name, description, effect;
    EquipmentKind kind = EquipmentKind::Relic;
    EquipmentSource source = EquipmentSource::World;
    unsigned int color = 0;
    bool owned = false;
};

const char* SourceName(EquipmentSource source);
const char* NodeTypeName(NodeType type);

namespace StoryData {
const std::vector<StoryChapter>& Chapters();
const std::vector<MapNode>& Nodes();
const std::vector<ArchiveEntry>& Archives();
const std::vector<EquipmentItem>& Equipment();
const MapNode* FindNode(const std::string& id);
const ArchiveEntry* FindArchive(const std::string& id);
const EquipmentItem* FindEquipment(const std::string& id);
const StoryChapter* FindChapter(const std::string& id);
}
