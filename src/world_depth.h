#pragma once
#include <string_view>

// Decorative depth is independent of walkable geometry and passage artwork.
namespace WorldDepth {
struct Layer { const char* suffix; float speed, height, y, opacity; };
inline constexpr Layer Back{"_back", .62F, 540, -25, .65F};
inline constexpr Layer Front{"_front", 1.22F, 80, 640, .92F};
inline constexpr float Offset(float camera,float speed) { return (640-camera)*speed; }
inline constexpr float FrontPosition(float worldWidth,float fraction,float camera) {
    return 640+(worldWidth*fraction-camera)*Front.speed;
}
inline constexpr float FrontGroups[]{.07F,.48F,.90F};
inline constexpr std::string_view Theme(std::string_view id) {
    if(id=="root"||id=="prts"||id=="throne"||id=="approach_prts")return "system";
    if(id=="industry"||id=="burn"||id=="core"||id=="normal"||id=="approach_industry"||id=="approach_core")return "industrial";
    if(id=="gray"||id=="entry"||id=="bridge"||id=="wtower"||id=="ice"||id=="industry"||
       id=="burn"||id=="core"||id=="normal"||id=="approach_bridge"||id=="approach_wtower"||
       id=="approach_ice"||id=="approach_industry"||id=="approach_core")return "street";
    return "interior";
}
inline constexpr std::string_view BackTheme(std::string_view theme) {
    return theme=="industrial"?"street":theme;
}
}
