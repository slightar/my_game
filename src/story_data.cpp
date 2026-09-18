#include "story_data.h"

namespace {
constexpr unsigned int kCold = 0x53CDDCFF, kWarm = 0xE8A36EFF, kNeutral = 0x98A3ABFF;

MapExit E(const char* id, const char* label, const char* target, float x, float y,
          bool hidden = false, const char* hint = "", const char* archive = "",
          const char* equipment = "", int truth = 0, bool completed = true,
          bool hiddenEnding = false) {
    return {id, label, target, hint, x, y, hidden,
            {archive, equipment, truth, completed, hiddenEnding}};
}
NodeReward R(std::vector<std::string> archives = {}, std::vector<std::string> equipment = {},
             int truth = 0, int compliance = 0, int memory = 0) {
    return {std::move(archives), std::move(equipment), truth, compliance, memory};
}
MapNode N(const char* id, const char* chapter, const char* name, const char* location,
          const char* objective, const char* characters, const char* description,
          NodeType type, EquipmentSource source, NodeReward reward,
          std::vector<MapExit> exits, bool noProtocol = false) {
    return {id, chapter, name, location, objective, characters, description,
            type, source, noProtocol, std::move(reward), std::move(exits)};
}
}

const char* SourceName(EquipmentSource source) {
    switch (source) { case EquipmentSource::Theresa: return "特蕾西娅";
                      case EquipmentSource::PRTS: return "PRTS";
                      case EquipmentSource::World: return "环境记录"; }
    return "";
}
const char* NodeTypeName(NodeType type) {
    switch (type) { case NodeType::Main: return "主线"; case NodeType::Boss: return "Boss";
                    case NodeType::Exploration: return "探索"; case NodeType::Hidden: return "隐藏";
                    case NodeType::Ending: return "结局"; }
    return "";
}

namespace StoryData {
const std::vector<StoryChapter>& Chapters() {
    static const std::vector<StoryChapter> chapters = {
        {"prologue", "序章 / 于切尔诺伯格苏醒", "苏醒、失利与第一次重置。", 0,
         {"prologue_awaken", "prologue_hidden_vent", "prologue_defeat"}},
        {"chapter1", "第一章 / 正确的道路", "PRTS 提供最快的撤离与作战路线。", 1,
         {"chapter1_reconnect", "chapter1_gray_snow", "chapter1_clocktower", "chapter1_train", "chapter1_crownslayer"}},
        {"chapter2", "第二章 / 冻原余烬", "霜星的轨迹埋在雪与灯火之下。", 2,
         {"chapter2_white_refuge", "chapter2_snow_lamp", "chapter2_whitefield", "chapter2_frostnova"}},
        {"chapter3", "第三章 / 龙门的回声", "阿米娅与博士调查重复的城市记忆。", 3,
         {"chapter3_silent_district", "chapter3_theater", "chapter3_rooftop_garden", "chapter3_ad_screen", "chapter3_amiya_trial"}},
        {"chapter4", "第四章 / 医生的审讯", "凯尔希留下的权限指向旧记录。", 4,
         {"chapter4_relay_station", "chapter4_zero_ward", "chapter4_black_archive", "chapter4_mon3tr"}},
        {"chapter5", "第五章 / 焚城之王", "追踪塔露拉，抵达切城核心。", 5,
         {"chapter5_burning_city", "chapter5_melted_command", "chapter5_originite_river", "chapter5_talulah", "chapter5_refusal"}},
        {"chapter6", "第六章 / 王庭旧梦", "由旧档案回到第一次死亡的出口。", 6,
         {"chapter6_lost_archive", "chapter6_nameless_throne", "chapter6_deleted_dialogue", "chapter6_first_exit"}},
        {"endings", "结局 / 归档与重逢", "两种处置方式，两个终点。", 7,
         {"ending_normal", "ending_prts_core", "ending_hidden"}}
    };
    return chapters;
}

const std::vector<MapNode>& Nodes() {
    static const std::vector<MapNode> nodes = {
        N("prologue_awaken", "prologue", "于切尔诺伯格苏醒", "切尔诺伯格 / 破损街区", "熟悉移动与终端出口", "特蕾西娅、博士", "特蕾西娅的声音穿过损坏的通讯频道。", NodeType::Main, EquipmentSource::Theresa,
          R({}, {"relic_diagnostic"}, 0, 0, 1),
          {E("awakening_route", "街区主道", "prologue_defeat", 1080, 370), E("broken_vent", "破裂通风道", "prologue_hidden_vent", 365, 205, true, "墙体后有微弱气流")}),
        N("prologue_hidden_vent", "prologue", "破裂通风道", "切尔诺伯格 / 通风夹层", "检查被遗忘的病历", "特蕾西娅", "环境中留下未被登记的患者信息。", NodeType::Hidden, EquipmentSource::Theresa,
          R({"unregistered_patient"}, {}, 1, 0, 1), {E("vent_return", "回到破损街区", "prologue_awaken", 225, 370)}),
        N("prologue_defeat", "prologue", "特蕾西娅剧情战", "切尔诺伯格 / 断裂广场", "经历第一次重置", "特蕾西娅", "剧情战与首次重置均为开发占位。", NodeType::Boss, EquipmentSource::Theresa,
          R({"first_reset"}, {}, 0, 0, -1), {E("reset_signal", "重置后的接入点", "chapter1_reconnect", 1080, 370)}),

        N("chapter1_reconnect", "chapter1", "重新接入", "切尔诺伯格 / 接入站", "完成 PRTS 新手校准", "PRTS", "系统建议优先恢复行动能力。", NodeType::Main, EquipmentSource::PRTS,
          R({}, {"protocol_tactical"}, 0, 1), {E("to_gray_snow", "灰雪街区入口", "chapter1_gray_snow", 1080, 370)}),
        N("chapter1_gray_snow", "chapter1", "灰雪街区", "切尔诺伯格 / 灰雪街区", "寻找高架桥出口", "PRTS、博士", "高架桥是最快路线，街区另有可调查的旧设施。", NodeType::Exploration, EquipmentSource::World,
          R({}, {}, 0, 1), {E("bridge_exit", "高架桥出口", "chapter1_crownslayer", 1080, 370),
             E("clock_roof", "钟楼屋顶", "chapter1_clocktower", 435, 205, true, "钟面仍有一束微光"),
             E("mall_basement", "商场地下层", "chapter1_train", 840, 545, true, "地下有未发送的信号", "unregistered_patient")}),
        N("chapter1_clocktower", "chapter1", "停摆钟楼", "灰雪街区 / 钟楼", "追踪首次呼唤", "特蕾西娅", "钟楼停在重置发生的时刻。", NodeType::Hidden, EquipmentSource::Theresa,
          R({}, {"echo_first_call"}, 1, 0, 1), {E("clock_merge", "通风捷径 / 返回街区", "chapter1_gray_snow", 225, 370)}),
        N("chapter1_train", "chapter1", "永不发车的列车", "灰雪街区 / 废弃站台", "读取未发出的撤离令", "环境记录", "列车广播只留下未发送的命令。", NodeType::Hidden, EquipmentSource::World,
          R({"unsent_evacuation"}, {}, 1), {E("train_merge", "高架桥下层", "chapter1_gray_snow", 225, 370)}),
        N("chapter1_crownslayer", "chapter1", "高架桥的猎手", "切尔诺伯格 / 高架桥", "击败弑君者", "弑君者", "章节 Boss 战斗待接入。", NodeType::Boss, EquipmentSource::PRTS,
          R({}, {}, 0, 1), {E("to_refuge", "冻原方向", "chapter2_white_refuge", 1080, 370)}),

        N("chapter2_white_refuge", "chapter2", "白色避难区", "冻原 / 避难区", "追踪霜星", "霜星、PRTS", "冻原主道可直接通往霜星的所在。", NodeType::Main, EquipmentSource::PRTS,
          R({}, {"protocol_cryo"}, 0, 1), {E("frozen_main", "冻原主道", "chapter2_frostnova", 1080, 370),
             E("heat_pipe", "热能管道", "chapter2_snow_lamp", 390, 540, true, "冰层下似有未熄的灯"),
             E("blizzard_crack", "暴风雪裂隙", "chapter2_whitefield", 805, 205, true, "雪原边缘出现异常回声", "", "relic_unsealed_lamp")}),
        N("chapter2_snow_lamp", "chapter2", "冰层下避难所", "冻原 / 冰下避难所", "保存未封存的灯火", "霜星、特蕾西娅", "灯火仍照着无人领取的物资。", NodeType::Hidden, EquipmentSource::Theresa,
          R({"unsealed_lamp_record"}, {"relic_unsealed_lamp"}, 1, 0, 1), {E("lamp_merge", "回到冻原主道", "chapter2_white_refuge", 225, 370)}),
        N("chapter2_whitefield", "chapter2", "无名雪原", "冻原 / 无名雪原", "追踪雪中的残响", "霜星", "雪没有熄灭所有的记忆。", NodeType::Hidden, EquipmentSource::Theresa,
          R({}, {"echo_burning_snow"}, 1, 0, 1), {E("whitefield_merge", "雪原回访捷径", "chapter2_white_refuge", 225, 370)}),
        N("chapter2_frostnova", "chapter2", "霜星", "冻原 / 终端雪线", "完成霜星 Boss 行动", "霜星", "章节 Boss 战斗待接入。", NodeType::Boss, EquipmentSource::PRTS,
          R({}, {}, 0, 1), {E("to_longmen", "龙门入口", "chapter3_silent_district", 1080, 370)}),

        N("chapter3_silent_district", "chapter3", "静默街区", "龙门 / 静默街区", "与阿米娅调查循环", "阿米娅", "城市重复着几乎相同的一天。", NodeType::Main, EquipmentSource::World,
          R({}, {}, 0, 1), {E("theater_street", "剧场街道", "chapter3_theater", 1080, 370),
             E("garden_ladder", "屋顶花园梯道", "chapter3_rooftop_garden", 430, 205, true, "屋顶传来不属于今日的脚步声")}),
        N("chapter3_theater", "chapter3", "没有观众的剧场", "龙门 / 旧剧场", "调查被重复播放的影像", "阿米娅", "台上仍有人等待一位不存在的观众。", NodeType::Exploration, EquipmentSource::World,
          R({}, {}, 0, 1), {E("relay_road", "中继站方向", "chapter4_relay_station", 1080, 370),
             E("screen_alley", "广告屏后巷", "chapter3_ad_screen", 395, 540, true, "屏幕上有被抹去的名字", "amiya_trial_record"),
             E("trial_stage", "阿米娅试炼场", "chapter3_amiya_trial", 805, 205, false)}),
        N("chapter3_rooftop_garden", "chapter3", "屋顶花园", "龙门 / 屋顶花园", "收集重复的记忆", "特蕾西娅、阿米娅", "她记得每一次已发生的离别。", NodeType::Hidden, EquipmentSource::Theresa,
          R({}, {"echo_every_time"}, 1, 0, 1), {E("garden_merge", "返回静默街区", "chapter3_silent_district", 225, 370)}),
        N("chapter3_ad_screen", "chapter3", "沉默广告屏", "龙门 / 广告屏后巷", "读取观测者名单", "阿米娅", "有一位观测者不在名单中。", NodeType::Hidden, EquipmentSource::World,
          R({"observer_not_list"}, {}, 1), {E("screen_merge", "返回剧场街道", "chapter3_theater", 225, 370)}),
        N("chapter3_amiya_trial", "chapter3", "阿米娅试炼", "龙门 / 临时试炼场", "不依赖协议完成挑战", "阿米娅", "可选挑战：完成时不能装备协议。", NodeType::Exploration, EquipmentSource::Theresa,
          R({"amiya_trial_record"}, {}, 2, 0, 1), {E("trial_merge", "返回旧剧场", "chapter3_theater", 225, 370)}, true),

        N("chapter4_relay_station", "chapter4", "罗德岛中继站", "罗德岛 / 中继站", "与凯尔希确认旧记录", "凯尔希", "新的权限可能打开此前封闭的病房。", NodeType::Main, EquipmentSource::World,
          R({}, {"protocol_preservation"}, 0, 1), {E("mon3tr_gate", "试炼通道", "chapter4_mon3tr", 1080, 370),
             E("zero_ward_door", "零号病房", "chapter4_zero_ward", 415, 205, true, "病房门后有旧医疗日志", "first_reset"),
             E("black_archive_door", "黑名单档案库", "chapter4_black_archive", 800, 540, true, "旧权限可开启此门", "", "relic_medical_permission")}),
        N("chapter4_zero_ward", "chapter4", "零号病房", "罗德岛 / 零号病房", "读取首次重置前日志", "凯尔希", "日志的时间戳早于你记得的第一次苏醒。", NodeType::Hidden, EquipmentSource::World,
          R({"zero_ward_log"}, {}, 1), {E("ward_merge", "返回中继站", "chapter4_relay_station", 225, 370)}),
        N("chapter4_black_archive", "chapter4", "黑名单档案库", "罗德岛 / 黑名单档案库", "查找旧权限留下的警告", "凯尔希、特蕾西娅", "有人告诫你不要把自己交给它。", NodeType::Hidden, EquipmentSource::Theresa,
          R({}, {"echo_dont_surrender"}, 1, 0, 1), {E("black_merge", "返回中继站", "chapter4_relay_station", 225, 370)}),
        N("chapter4_mon3tr", "chapter4", "Mon3tr 试炼", "罗德岛 / 医疗部", "通过 Mon3tr 试炼", "凯尔希、Mon3tr", "章节 Boss 战斗待接入。", NodeType::Boss, EquipmentSource::World,
          R({"medical_old_permission_record"}, {"relic_medical_permission"}, 0, 1), {E("to_core_city", "切城核心方向", "chapter5_burning_city", 1080, 370),
             E("medical_backtrack", "医疗部回访通道", "chapter4_relay_station", 225, 370)}),

        N("chapter5_burning_city", "chapter5", "切城核心", "切尔诺伯格 / 焚城核心", "追踪塔露拉", "塔露拉", "PRTS 将塔露拉所在区域标为最优目标。", NodeType::Main, EquipmentSource::PRTS,
          R({}, {}, 0, 1), {E("talulah_route", "中央指挥塔", "chapter5_talulah", 1080, 370),
             E("melted_room", "熔毁指令室", "chapter5_melted_command", 415, 205, true, "地下线路仍有热源"),
             E("river_fault", "逆流源石河", "chapter5_originite_river", 800, 540, true, "源石河流方向异常", "", "", 3)}),
        N("chapter5_melted_command", "chapter5", "熔毁指令室", "切城核心 / 地下指令室", "调查 PRTS 根系", "塔露拉", "塔露拉真正攻击的是地下的 PRTS 根系。", NodeType::Hidden, EquipmentSource::World,
          R({"melted_command_record"}, {}, 1), {E("melted_merge", "返回切城核心", "chapter5_burning_city", 225, 370)}),
        N("chapter5_originite_river", "chapter5", "逆流源石河", "切城核心 / 源石河", "记录静止的人群", "特蕾西娅", "人群在逆流中保持着同一个动作。", NodeType::Hidden, EquipmentSource::Theresa,
          R({"still_crowd"}, {}, 1), {E("river_merge", "返回切城核心", "chapter5_burning_city", 225, 370)}),
        N("chapter5_talulah", "chapter5", "塔露拉", "切城核心 / 中央指挥塔", "完成塔露拉 Boss 行动", "塔露拉", "普通路线的最终 Boss。后续处置由玩家选择。", NodeType::Boss, EquipmentSource::PRTS,
          R({}, {}, 0, 1), {E("standard_disposal", "接受任务结算", "ending_normal", 1080, 370),
             E("evacuation_gap", "无名撤离通道", "chapter5_refusal", 430, 540, true, "撤离通道仍有未清除的记录", "", "", 3),
             E("observer_archive", "档案回访入口", "chapter6_lost_archive", 805, 205, true, "观测记录指向旧档案", "observer_not_list")}),
        N("chapter5_refusal", "chapter5", "无名撤离通道", "切城核心 / 撤离通道", "保留焚城者的拒绝", "塔露拉、特蕾西娅", "她拒绝的不是撤离，而是一次被安排好的结算。", NodeType::Hidden, EquipmentSource::Theresa,
          R({"refusal_record"}, {"relic_refusal"}, 1, 0, 1), {E("refusal_merge", "旧档案入口", "chapter6_lost_archive", 1080, 370)}),

        N("chapter6_lost_archive", "chapter6", "失落档案室", "王庭旧址 / 档案室", "整理被删去的时间线", "凯尔希、特蕾西娅", "信息不再按 PRTS 的顺序排列。", NodeType::Main, EquipmentSource::Theresa,
          R({"lost_archive_record"}, {}, 1, 0, 1), {E("throne_passage", "无名王座", "chapter6_nameless_throne", 415, 205, true, "旧王庭的地面有细微裂纹"),
             E("deleted_audio", "被删去的对话", "chapter6_deleted_dialogue", 805, 540, true, "终端中有一段损坏音轨"),
             E("first_exit_gate", "第一次死亡的出口", "chapter6_first_exit", 1080, 370, true, "另一处出口需要完整的记忆", "", "", 0, true, true)}),
        N("chapter6_nameless_throne", "chapter6", "无名王座", "王庭旧址 / 王座", "接收未竟之言", "特蕾西娅", "王座上没有王，只有仍未结束的话语。", NodeType::Hidden, EquipmentSource::Theresa,
          R({}, {"echo_unfinished_words"}, 1, 0, 1), {E("throne_merge", "返回失落档案室", "chapter6_lost_archive", 225, 370)}),
        N("chapter6_deleted_dialogue", "chapter6", "被删去的对话", "王庭旧址 / 记录夹层", "恢复凯尔希与特蕾西娅档案", "凯尔希、特蕾西娅", "被删去的对话仍可从残响中复原。", NodeType::Hidden, EquipmentSource::Theresa,
          R({"kaltsit_theresa_record"}, {}, 1, 0, 1), {E("dialogue_merge", "返回失落档案室", "chapter6_lost_archive", 225, 370)}),
        N("chapter6_first_exit", "chapter6", "第一次死亡的出口", "王庭旧址 / 隐藏出口", "进入 PRTS 核心", "特蕾西娅", "这里指向第一次重置前被隐藏的出口。", NodeType::Hidden, EquipmentSource::Theresa,
          R({"first_exit_record"}, {}, 1, 0, 1), {E("core_access", "PRTS 核心入口", "ending_prts_core", 1080, 370)}),

        N("ending_normal", "endings", "标准处置", "PRTS / 任务归档", "接受任务结算", "PRTS", "任务完成，世界恢复正常并被稳定封存。", NodeType::Ending, EquipmentSource::PRTS,
          R({"standard_disposal_record"}), {E("return_to_talulah", "返回中央指挥塔", "chapter5_talulah", 225, 370)}),
        N("ending_prts_core", "endings", "PRTS 核心", "系统 / 核心层", "击败 PRTS", "PRTS", "Boss 战占位：此战中协议权限被临时撤销。", NodeType::Boss, EquipmentSource::PRTS,
          R({"prts_core_record"}), {E("to_hidden_ending", "记忆出口", "ending_hidden", 1080, 370)}),
        N("ending_hidden", "endings", "拥抱特蕾西娅", "记忆 / 原石回声", "保存仍被记得的信息", "特蕾西娅", "她已化为原石，留下的是储存在游戏中的信息。", NodeType::Ending, EquipmentSource::Theresa,
          R({"theresa_remembered_record"}), {})
    };
    return nodes;
}

const std::vector<ArchiveEntry>& Archives() {
    static const std::vector<ArchiveEntry> archives = {
        {"unregistered_patient", "未登记的患者", "破裂通风道中的旧患者记录。", ArchiveCategory::Story, EquipmentSource::World, kNeutral},
        {"first_reset", "第一次重置", "特蕾西娅剧情战后，终端恢复到了新的起点。", ArchiveCategory::Story, EquipmentSource::Theresa, kWarm},
        {"unsent_evacuation", "未发送的撤离令", "列车的撤离命令从未被发送。", ArchiveCategory::Story, EquipmentSource::World, kNeutral},
        {"unsealed_lamp_record", "未封存的灯火", "冰层下仍有人维持着微弱的热源。", ArchiveCategory::Item, EquipmentSource::Theresa, kWarm},
        {"observer_not_list", "观测者不在名单中", "名单缺失了某位始终在场的观测者。", ArchiveCategory::Story, EquipmentSource::World, kNeutral},
        {"amiya_trial_record", "阿米娅的试炼", "不依赖协议完成的战术记录。", ArchiveCategory::Story, EquipmentSource::Theresa, kWarm},
        {"zero_ward_log", "零号病房医疗日志", "时间戳早于第一次可见的重置。", ArchiveCategory::Story, EquipmentSource::World, kNeutral},
        {"medical_old_permission_record", "医疗部旧权限", "可开启被封闭的旧医疗档案库。", ArchiveCategory::Item, EquipmentSource::World, kNeutral},
        {"melted_command_record", "熔毁指令", "塔露拉攻击的是地下 PRTS 根系。", ArchiveCategory::Story, EquipmentSource::World, kNeutral},
        {"still_crowd", "静止的人群", "源石河中的人群保持同一个瞬间。", ArchiveCategory::Story, EquipmentSource::Theresa, kWarm},
        {"refusal_record", "焚城者的拒绝", "撤离通道保留了塔露拉未被归档的选择。", ArchiveCategory::Item, EquipmentSource::Theresa, kWarm},
        {"lost_archive_record", "失落档案索引", "王庭旧档案中的缺页目录。", ArchiveCategory::Story, EquipmentSource::World, kNeutral},
        {"kaltsit_theresa_record", "凯尔希与特蕾西娅", "两人的对话从系统记录中被删去。", ArchiveCategory::Story, EquipmentSource::Theresa, kWarm},
        {"first_exit_record", "第一次死亡的出口", "出口藏在重置之前的记录里。", ArchiveCategory::Story, EquipmentSource::Theresa, kWarm},
        {"standard_disposal_record", "标准处置", "任务成功，世界被稳定封存。", ArchiveCategory::Story, EquipmentSource::PRTS, kCold},
        {"prts_core_record", "权限已撤销", "协议仅在 PRTS 核心战中失效。", ArchiveCategory::Story, EquipmentSource::PRTS, kCold},
        {"theresa_remembered_record", "特蕾西娅：仍在被记得", "原石中留下了储存在游戏里的信息。", ArchiveCategory::Story, EquipmentSource::Theresa, kWarm}
    };
    return archives;
}

const std::vector<EquipmentItem>& Equipment() {
    static const std::vector<EquipmentItem> items = {
        {"protocol_tactical", "战术校准协议", "PRTS 提供的直接火力校准。", "造成伤害大幅提升（占位）", EquipmentKind::Protocol, EquipmentSource::PRTS, kCold},
        {"protocol_cryo", "低温净化协议", "PRTS 为冻原行动分配防护参数。", "低温伤害大幅降低（占位）", EquipmentKind::Protocol, EquipmentSource::PRTS, kCold},
        {"protocol_preservation", "绝对保存协议", "PRTS 提供额外生命储备。", "额外生命与容错显著增加（占位）", EquipmentKind::Protocol, EquipmentSource::PRTS, kCold},
        {"relic_diagnostic", "凝滞的诊疗牌", "保留一次环境观察。", "提示旧设施异常（占位）", EquipmentKind::Relic, EquipmentSource::Theresa, kWarm},
        {"relic_unsealed_lamp", "未封存的灯火", "照亮冻原的隐蔽裂隙。", "显示雪原隐藏出口线索（占位）", EquipmentKind::Relic, EquipmentSource::Theresa, kWarm},
        {"relic_medical_permission", "医疗部旧权限", "允许回访封闭档案。", "开启旧权限门（占位）", EquipmentKind::Relic, EquipmentSource::Theresa, kWarm},
        {"relic_refusal", "焚城者的拒绝", "保存一次未被系统选择的决定。", "揭示撤离通道线索（占位）", EquipmentKind::Relic, EquipmentSource::Theresa, kWarm},
        {"echo_first_call", "第一次呼唤", "记住停摆钟楼的声音。", "死亡后保留一段记忆（占位）", EquipmentKind::Echo, EquipmentSource::Theresa, kWarm},
        {"echo_burning_snow", "仍在燃烧的雪", "雪中仍有未熄灭的热度。", "提示冻原回访捷径（占位）", EquipmentKind::Echo, EquipmentSource::Theresa, kWarm},
        {"echo_every_time", "她记得每一次", "重复之中仍有细微差异。", "标出循环中的异常（占位）", EquipmentKind::Echo, EquipmentSource::Theresa, kWarm},
        {"echo_dont_surrender", "不要把自己交给它", "一条没有被归档的警告。", "保留一条关键记录（占位）", EquipmentKind::Echo, EquipmentSource::Theresa, kWarm},
        {"echo_unfinished_words", "王的未竟之言", "王庭旧梦中未说完的话。", "提高记忆完整度提示（占位）", EquipmentKind::Echo, EquipmentSource::Theresa, kWarm}
    };
    return items;
}

const MapNode* FindNode(const std::string& id) { for (const auto& n : Nodes()) if (n.id == id) return &n; return nullptr; }
const ArchiveEntry* FindArchive(const std::string& id) { for (const auto& a : Archives()) if (a.id == id) return &a; return nullptr; }
const EquipmentItem* FindEquipment(const std::string& id) { for (const auto& e : Equipment()) if (e.id == id) return &e; return nullptr; }
const StoryChapter* FindChapter(const std::string& id) { for (const auto& c : Chapters()) if (c.id == id) return &c; return nullptr; }
}
