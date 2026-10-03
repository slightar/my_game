#pragma once
#include <array>
#include <string_view>

// One authored panorama per region. Passage coordinates are fractions of the
// image width, in incoming-then-outgoing route order, before Ports sorts them.
namespace WorldArt {
// Normalized scene components. The staircase rises leftward to a landing;
// the ground route remains available beneath it.
struct Stairway { float left, right, top, bottom, landingLeft, landingRight; const char* destination; };
inline constexpr Stairway EntryStair{.705F,.827F,.266F,.525F,.645F,.900F,"station"};
struct Scene {
    const char* id;
    float ground;
    std::array<float,3> passages;
    const char* clue;
    const Stairway* stairway=nullptr;
    const char* imageOverride=nullptr;
    float paintedGround=0;
    const char* closedImageOverride=nullptr;
};
inline constexpr std::array Scenes{
    Scene{"clinic",.75F,{.265F},"帘后的旧门留着一道缝",nullptr,"clinic_scale_v3",0,"clinic_closed_scale_v3"},
    Scene{"gray",.745F,{.28F},"停摆的钟下，楼梯间有风",nullptr,"gray_scale_v3"},
    Scene{"clock",.71F,{.12F,.89F},"齿轮停了，通道仍在",nullptr,"clock_scale_v3"},
    Scene{"entry",.70F,{.815F},"天桥上方的门后传来回声",&EntryStair,"entry_style_v2",.638F},
    Scene{"station",.635F,{.10F,.36F,.88F},"废弃站台后还有通道",nullptr,"station_scale_v3"},
    Scene{"ward",.685F,{.15F,.715F},"隔离区的门没有锁",nullptr,"ward_scale_v3"},
    Scene{"bridge",.64F,{.25F},"检修亭里有一条旧路",nullptr,"bridge_style_v2"},
    Scene{"wtower",.725F,{},"",nullptr,"wtower_style_v2"},
    Scene{"comm",.69F,{.32F},"断线的设备后露出缺口",nullptr,"comm_scale_v3"},
    Scene{"well",.72F,{.16F,.49F,.84F},"井壁的通道传来回声",nullptr,"well_scale_v3"},
    Scene{"ice",.70F,{},"",nullptr,"ice_scale_v3"},
    Scene{"pipes",.69F,{.22F,.405F},"热流从侧面的通道涌出",nullptr,"pipes_scale_v3"},
    Scene{"shelter",.71F,{.185F,.87F},"生活痕迹延伸到通道深处",nullptr,"shelter_scale_v3"},
    Scene{"industry",.67F,{},"",nullptr,"industry_scale_v3"},
    Scene{"burn",.69F,{},"",nullptr,"burn_scale_v3"},
    Scene{"core",.71F,{.145F},"焦黑的井口通向下方",nullptr,"core_scale_v3"},
    Scene{"normal",.71F,{},"",nullptr,"normal_scale_v3"},
    Scene{"root",.62F,{.225F,.79F},"线缆汇入未标记的通道",nullptr,"root_scale_v3"},
    Scene{"prts",.72F,{.695F},"光的边缘留着一道裂隙",nullptr,"prts_scale_v3"},
    Scene{"throne",.70F,{},"",nullptr,"throne_style_v2"}
};
// Internal approach scenes never become stage-selection nodes.
inline constexpr std::array ApproachScenes{
    Scene{"approach_bridge",.74F,{.22F},"收费亭里留有一份巡逻记录",nullptr,"approach_bridge_scale_v3",0.615F},
    Scene{"approach_wtower",.74F,{.22F},"维修台的无线电还留有录音",nullptr,"approach_wtower_scale_v3",0.753F},
    Scene{"approach_ice",.74F,{.22F},"信箱里夹着一张撤离便条",nullptr,"approach_ice_scale_v3",0.774F},
    Scene{"approach_industry",.74F,{.22F},"货运桌上压着一份旧清单",nullptr,"approach_industry_scale_v3",0.696F},
    Scene{"approach_core",.74F,{.22F},"疏散终端仍保留最后一条消息",nullptr,"approach_core_scale_v3",0.703F},
    Scene{"approach_prts",.74F,{.22F},"静默终端残留一段访问日志",nullptr,"approach_prts_scale_v3",0.720F}
};
inline constexpr const Scene* Find(std::string_view id) {
    for(const auto& scene:Scenes)if(id==scene.id)return &scene;
    for(const auto& scene:ApproachScenes)if(id==scene.id)return &scene;
    return nullptr;
}
inline constexpr bool Repainted(std::string_view id) {
    return Find(id)!=nullptr;
}
// Separate painted floor alignment from world width: an art correction must
// not silently shorten an already authored route.
inline constexpr float PaintedGround(std::string_view id) {
    if(const auto* scene=Find(id);scene&&scene->paintedGround>0)return scene->paintedGround;
    if(id=="ward")return .745F;
    if(id=="clinic")return .730F;
    if(id=="shelter")return .665F;
    if(id=="comm")return .645F;
    if(id=="gray")return .735F;
    if(id=="wtower")return .715F;
    if(id=="normal")return .690F;
    if(id=="throne")return .680F;
    return Find(id)->ground;
}
// Match the shipped panorama dimensions. Width follows uniform scale rather
// than stretching the image to an unrelated gameplay rectangle.
inline constexpr float Width(std::string_view id) {
    const auto* scene=Find(id);
    const float aspect=id=="clock"?2171.0F/724.0F:3.0F;
    return scene?644.0F/scene->ground*aspect:1280.0F;
}
inline constexpr const Stairway* Stairs(std::string_view id) {
    const auto* scene=Find(id);return scene?scene->stairway:nullptr;
}
inline constexpr float UpperFloor(std::string_view id) {
    const auto* stairs=Stairs(id);
    return stairs?644+(stairs->top-PaintedGround(id))*(644/Find(id)->ground):644;
}
}
