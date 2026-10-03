#include "progress_system.h"
#include "world_layout.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

using Json = nlohmann::json;

ProgressSystem::ProgressSystem(std::filesystem::path applicationDirectory)
    : savePath_(std::move(applicationDirectory) / "save" / "game_progress.json") { Load(); }

bool ProgressSystem::RegionVisible(const std::string& id) const {
    return !WorldLayout::Internal(id) && WorldLayout::Find(id) && (!WorldLayout::Hidden(id) || RegionVisited(id));
}
bool ProgressSystem::RegionAvailable(const std::string& id) const {
    if(!RegionVisible(id))return false;
    if(id=="clinic" || RegionVisited(id))return true;
    for(const auto& r:WorldLayout::Routes)
        if(r.kind==WorldLayout::RouteKind::Main && r.to==id && RegionVisited(r.from))return true;
    return false;
}
bool ProgressSystem::SelectRegion(const std::string& id) {
    if(!RegionAvailable(id))return false;
    const auto before=progress_;progress_.selectedRegion=id;
    if(Save())return true;progress_=before;return false;
}
bool ProgressSystem::VisitRegion(const std::string& id,const std::string& from) {
    if(!WorldLayout::Find(id)||WorldLayout::Internal(id))return false;
    if(from=="clinic"&&id=="ward"&&!progress_.wardShortcutOpen)return false;
    const bool openShortcut=from=="ward"&&id=="clinic"&&
        progress_.selectedRegion=="ward"&&RegionVisited("ward");
    if(RegionVisited(id)&&progress_.selectedRegion==id&&!openShortcut)return true;
    const auto before=progress_;progress_.visitedRegions.insert(id);progress_.selectedRegion=id;
    if(openShortcut)progress_.wardShortcutOpen=true;
    if(Save())return true;progress_=before;return false;
}

std::vector<ArchiveEntry> ProgressSystem::Archives(ArchiveCategory category) const {
    std::vector<ArchiveEntry> result;
    for (auto entry : StoryData::Archives()) if (entry.category == category) {
        entry.unlocked = HasArchive(entry.id); result.push_back(std::move(entry));
    }
    return result;
}
std::vector<EquipmentItem> ProgressSystem::Equipment() const {
    auto result = StoryData::Equipment();
    for (auto& item : result) item.owned = HasEquipment(item.id);
    return result;
}
NodeState ProgressSystem::State(const std::string& id) const {
    const auto* node = FindNode(id);
    if (!node) return NodeState::Locked;
    if (IsCompleted(id)) return NodeState::Completed;
    if (IsUnlocked(id)) return NodeState::Available;
    return node->type == NodeType::Hidden ? NodeState::Hidden : NodeState::Locked;
}
bool ProgressSystem::ExitVisible(const MapExit& exit) const {
    return !exit.hidden || IsUnlocked(exit.targetId);
}
std::string ProgressSystem::UnlockConditionText(const std::string& id) const {
    if (id == "prologue_awaken") return "初始可进入";
    for (const auto& source : Nodes()) for (const auto& exit : source.exits) if (exit.targetId == id) {
        std::string text = "从" + source.name + "的出口进入";
        if (exit.condition.sourceCompleted) text += "；完成来源地点";
        if (!exit.condition.requiredArchive.empty())
            if (const auto* archive = StoryData::FindArchive(exit.condition.requiredArchive))
                text += "；关联档案《" + archive->name + "》";
        if (!exit.condition.requiredEquipment.empty())
            if (const auto* item = FindEquipment(exit.condition.requiredEquipment))
                text += "；获得《" + item->name + "》";
        if (exit.condition.minTruth > 0) text += "；真相发现度达到" + std::to_string(exit.condition.minTruth);
        if (exit.condition.hiddenEndingRequirements) text += "；满足隐藏结局调查条件";
        return text;
    }
    return "未记录的入口";
}
bool ProgressSystem::ConditionMet(const MapNode& source, const MapExit& exit) const {
    const auto& condition = exit.condition;
    if (condition.sourceCompleted && !IsCompleted(source.id)) return false;
    if (!condition.requiredArchive.empty() && !HasArchive(condition.requiredArchive)) return false;
    if (!condition.requiredEquipment.empty() && !HasEquipment(condition.requiredEquipment)) return false;
    if (progress_.truthDiscovery < condition.minTruth) return false;
    if (condition.hiddenEndingRequirements && !HiddenRequirementsMet()) return false;
    return true;
}
bool ProgressSystem::ExitAvailable(const MapExit& exit) const {
    const auto* node = CurrentNode();
    if (!node || !IsUnlocked(exit.targetId)) return false;
    for (const auto& own : node->exits) if (own.id == exit.id && own.targetId == exit.targetId)
        return ConditionMet(*node, own);
    return false;
}
bool ProgressSystem::Travel(const std::string& exitId) {
    const auto* node = CurrentNode();
    if (!node) return false;
    for (const auto& exit : node->exits) if (exit.id == exitId && ExitAvailable(exit)) {
        progress_.currentNode = exit.targetId; return Save();
    }
    return false;
}

bool ProgressSystem::SelectNode(const std::string& id) {
    if (!StoryData::FindNode(id) || !progress_.unlockedNodes.contains(id)) return false;
    progress_.currentNode = id;
    Save();
    return true;
}
bool ProgressSystem::HasEquippedProtocol() const {
    for (const auto& id : progress_.slots) if (const auto* item = FindEquipment(id))
        if (item->kind == EquipmentKind::Protocol) return true;
    return false;
}
bool ProgressSystem::CanCompleteCurrent() const {
    const auto* node = CurrentNode();
    if (!node || !IsUnlocked(node->id) || IsCompleted(node->id)) return false;
    if (node->requiresNoProtocol && HasEquippedProtocol()) return false;
    if (node->id == "ending_normal" && !IsCompleted("chapter5_talulah")) return false;
    if (node->id == "ending_prts_core" && !IsCompleted("chapter6_first_exit")) return false;
    if (node->id == "ending_hidden" && !IsCompleted("ending_prts_core")) return false;
    return true;
}
void ProgressSystem::RevealAvailable() {
    bool changed;
    do {
        changed = false;
        for (const auto& node : Nodes()) if (IsUnlocked(node.id))
            for (const auto& exit : node.exits)
                if (ConditionMet(node, exit) && !IsUnlocked(exit.targetId))
                    changed |= progress_.unlockedNodes.insert(exit.targetId).second;
    } while (changed);
}
bool ProgressSystem::CompleteNode(const std::string& id) {
    if (id != progress_.currentNode || !CanCompleteCurrent()) return false;
    const auto* node = CurrentNode();
    progress_.completedNodes.insert(id);
    for (const auto& archive : node->reward.archiveIds) progress_.archiveIds.insert(archive);
    for (const auto& item : node->reward.equipmentIds) progress_.equipmentIds.insert(item);
    progress_.truthDiscovery += node->reward.truth;
    progress_.prtsCompliance += node->reward.compliance;
    progress_.memoryIntegrity = std::max(0, progress_.memoryIntegrity + node->reward.memory);
    if (id == "ending_normal") progress_.normalComplete = true;
    if (id == "ending_hidden") progress_.hiddenComplete = true;
    RevealAvailable(); return Save();
}
bool ProgressSystem::Equip(int slot, const std::string& id) {
    if (slot < 0 || slot >= static_cast<int>(progress_.slots.size()) ||
        (!id.empty() && (!HasEquipment(id) || !FindEquipment(id)))) return false;
    if (!id.empty()) for (int i = 0; i < static_cast<int>(progress_.slots.size()); ++i)
        if (i != slot && progress_.slots[i] == id) progress_.slots[i].clear();
    progress_.slots[slot] = id; return Save();
}
EquipmentModifiers ProgressSystem::CalculateEquipmentModifiers() const {
    EquipmentModifiers result;
    for (const auto& id : progress_.slots) if (const auto* item = FindEquipment(id))
        if (!(protocolSuppressed_ && item->kind == EquipmentKind::Protocol))
            result.effectDescriptions.push_back(item->effect);
    return result;
}
int ProgressSystem::OwnedEchoCount() const {
    int count = 0;
    for (const auto& id : progress_.equipmentIds) if (const auto* item = FindEquipment(id))
        if (item->kind == EquipmentKind::Echo) ++count;
    return count;
}
bool ProgressSystem::HiddenRequirementsMet() const {
    return progress_.truthDiscovery >= kHiddenTruthThreshold && OwnedEchoCount() >= 3 &&
           (HasEquipment("relic_refusal") || HasArchive("observer_not_list")) &&
           IsCompleted("chapter6_lost_archive");
}
bool ProgressSystem::EndingComplete(EndingType type) const {
    return type == EndingType::Normal ? progress_.normalComplete :
           type == EndingType::Hidden && progress_.hiddenComplete;
}
void ProgressSystem::AddCompliance() { ++progress_.prtsCompliance; Save(); }
void ProgressSystem::AddTruth() { ++progress_.truthDiscovery; RevealAvailable(); Save(); }
void ProgressSystem::UnlockAllNodes() {
    for (const auto& node : Nodes()) progress_.unlockedNodes.insert(node.id);
    Save();
}
void ProgressSystem::DebugNormalReady() {
    // Development shortcut: maintain the fixed boss order while exposing the ending flow.
    const char* chain[] = {"prologue_awaken", "prologue_defeat", "chapter1_reconnect", "chapter1_gray_snow",
        "chapter1_crownslayer", "chapter2_white_refuge", "chapter2_frostnova", "chapter3_silent_district",
        "chapter3_theater", "chapter4_relay_station", "chapter4_mon3tr", "chapter5_burning_city", "chapter5_talulah"};
    for (const char* id : chain) {
        progress_.unlockedNodes.insert(id); progress_.completedNodes.insert(id);
    }
    progress_.unlockedNodes.insert("ending_normal");
    progress_.currentNode = "chapter5_talulah";
    RevealAvailable(); Save();
}
void ProgressSystem::DebugHiddenReady() {
    DebugNormalReady();
    const char* extras[] = {"chapter1_clocktower", "chapter2_whitefield", "chapter3_rooftop_garden",
        "chapter5_refusal", "chapter6_lost_archive"};
    for (const char* id : extras) {
        progress_.unlockedNodes.insert(id); progress_.completedNodes.insert(id);
    }
    progress_.equipmentIds.insert({"echo_first_call", "echo_burning_snow", "echo_every_time", "relic_refusal"});
    progress_.archiveIds.insert("refusal_record");
    progress_.truthDiscovery = std::max(progress_.truthDiscovery, kHiddenTruthThreshold);
    progress_.unlockedNodes.insert("chapter6_first_exit");
    progress_.currentNode = "chapter6_lost_archive";
    RevealAvailable(); Save();
}
bool ProgressSystem::Validate(const GameProgress& candidate) const {
    if (candidate.version != 2 || !candidate.unlockedNodes.contains("prologue_awaken") ||
        !candidate.unlockedNodes.contains(candidate.currentNode) || !FindNode(candidate.currentNode) ||
        candidate.prtsCompliance < 0 || candidate.truthDiscovery < 0 || candidate.memoryIntegrity < 0) return false;
    for (const auto& id : candidate.unlockedNodes) if (!FindNode(id)) return false;
    for (const auto& id : candidate.completedNodes) if (!FindNode(id)) return false;
    for (const auto& id : candidate.archiveIds) if (!StoryData::FindArchive(id)) return false;
    for (const auto& id : candidate.equipmentIds) if (!FindEquipment(id)) return false;
    for (const auto& id : candidate.slots) if (!id.empty() && !candidate.equipmentIds.contains(id)) return false;
    for(const auto& id:candidate.visitedRegions)if(!WorldLayout::Find(id))return false;
    if(!WorldLayout::Find(candidate.selectedRegion))return false;
    if(candidate.selectedRegion!="clinic" && WorldLayout::Hidden(candidate.selectedRegion) && !candidate.visitedRegions.contains(candidate.selectedRegion))return false;
    return true;
}
bool ProgressSystem::Save() {
    try {
        std::filesystem::create_directories(savePath_.parent_path());
        if (legacyPending_ && std::filesystem::exists(savePath_)) {
            const auto backup = savePath_.parent_path() / "game_progress.v1.bak";
            if (!std::filesystem::exists(backup)) std::filesystem::copy_file(savePath_, backup);
        }
        Json data = {{"visitedRegions", progress_.visitedRegions}, {"selectedRegion", progress_.selectedRegion}, {"version", progress_.version}, {"unlockedNodes", progress_.unlockedNodes},
            {"wardShortcutOpen", progress_.wardShortcutOpen},
            {"completedNodes", progress_.completedNodes}, {"selectedNode", progress_.currentNode},
            {"archiveIds", progress_.archiveIds}, {"equipmentIds", progress_.equipmentIds},
            {"slots", progress_.slots}, {"prtsCompliance", progress_.prtsCompliance},
            {"truthDiscovery", progress_.truthDiscovery}, {"memoryIntegrity", progress_.memoryIntegrity},
            {"normalComplete", progress_.normalComplete}, {"hiddenComplete", progress_.hiddenComplete}};
        const auto temporary = savePath_.string() + ".tmp";
        { std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
          if (!out) throw std::runtime_error("无法写入存档"); out << data.dump(2);
          if (!out) throw std::runtime_error("存档写入中断"); }
        std::filesystem::copy_file(temporary, savePath_, std::filesystem::copy_options::overwrite_existing);
        std::filesystem::remove(temporary);
        legacyPending_ = false; error_.clear(); return true;
    } catch (const std::exception& e) {
        progress_ = GameProgress{}; protocolSuppressed_ = false;
        error_ = std::string("存档失败，已回退默认进度: ") + e.what(); return false;
    }
}
void ProgressSystem::LoadLegacy(const std::string& serialized) {
    const auto data = Json::parse(serialized);
    progress_ = GameProgress{};
    progress_.prtsCompliance = std::max(0, data.value("prtsCompliance", 0));
    progress_.truthDiscovery = std::max(0, data.value("truthDiscovery", 0));
    progress_.normalComplete = data.value("normalComplete", false);
    progress_.hiddenComplete = data.value("hiddenComplete", false);
    if (progress_.normalComplete) progress_.archiveIds.insert("standard_disposal_record");
    if (progress_.hiddenComplete) progress_.archiveIds.insert("theresa_remembered_record");
    const std::pair<const char*, const char*> archiveMap[] = {
        {"first_reset", "first_reset"}, {"theresa_trace", "kaltsit_theresa_record"},
        {"broken_terminal", "unregistered_patient"}, {"originite_fragment", "still_crowd"}};
    const auto oldStories = data.value("storyRecords", std::set<std::string>{});
    const auto oldItems = data.value("itemRecords", std::set<std::string>{});
    for (const auto& [oldId, newId] : archiveMap)
        if (oldStories.contains(oldId) || oldItems.contains(oldId)) progress_.archiveIds.insert(newId);
    const std::pair<const char*, const char*> equipmentMap[] = {
        {"protocol_damage", "protocol_tactical"}, {"protocol_vitality", "protocol_preservation"},
        {"relic_lantern", "relic_diagnostic"}, {"relic_shard", "relic_unsealed_lamp"},
        {"echo_memory", "echo_first_call"}, {"echo_whisper", "echo_every_time"}};
    const auto oldOwned = data.value("ownedEquipment", std::set<std::string>{});
    for (const auto& [oldId, newId] : equipmentMap) if (oldOwned.contains(oldId)) progress_.equipmentIds.insert(newId);
    if (data.contains("slots")) {
        const auto oldSlots = data.at("slots").get<std::array<std::string, 3>>();
        for (int i = 0; i < 3; ++i) for (const auto& [oldId, newId] : equipmentMap)
            if (oldSlots[i] == oldId && progress_.equipmentIds.contains(newId)) progress_.slots[i] = newId;
    }
    legacyPending_ = true;
    error_ = "旧版进度已迁移；原文件将在首次保存时备份";
}
bool ProgressSystem::Load() {
    try {
        if (!std::filesystem::exists(savePath_)) return true;
        std::ifstream in(savePath_, std::ios::binary);
        if (!in) throw std::runtime_error("无法读取存档");
        const std::string serialized((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto data = Json::parse(serialized);
        const int version = data.at("version").get<int>();
        if (version == 1) { LoadLegacy(serialized); return true; }
        if (version != 2) throw std::runtime_error("不支持的存档版本");
        GameProgress loaded;
        loaded.version = version;
        loaded.visitedRegions = data.value("visitedRegions", std::set<std::string>{});
        loaded.wardShortcutOpen = data.value("wardShortcutOpen", false);
        loaded.selectedRegion = data.value("selectedRegion", std::string("clinic"));
        loaded.unlockedNodes = data.at("unlockedNodes").get<std::set<std::string>>();
        loaded.completedNodes = data.at("completedNodes").get<std::set<std::string>>();
        loaded.currentNode = data.at("selectedNode").get<std::string>();
        loaded.archiveIds = data.at("archiveIds").get<std::set<std::string>>();
        loaded.equipmentIds = data.at("equipmentIds").get<std::set<std::string>>();
        loaded.slots = data.at("slots").get<std::array<std::string, 3>>();
        loaded.prtsCompliance = data.at("prtsCompliance").get<int>();
        loaded.truthDiscovery = data.at("truthDiscovery").get<int>();
        loaded.memoryIntegrity = data.at("memoryIntegrity").get<int>();
        loaded.normalComplete = data.at("normalComplete").get<bool>();
        loaded.hiddenComplete = data.at("hiddenComplete").get<bool>();
        if (!Validate(loaded)) throw std::runtime_error("存档进度无效");
        progress_ = std::move(loaded); legacyPending_ = false; error_.clear(); return true;
    } catch (const std::exception& e) {
        progress_ = GameProgress{}; legacyPending_ = false; protocolSuppressed_ = false;
        error_ = std::string("存档读取失败，已使用默认进度: ") + e.what(); return false;
    }
}
bool ProgressSystem::ResetProgress() {
    progress_ = GameProgress{}; protocolSuppressed_ = false;
    return Save();
}
