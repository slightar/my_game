#pragma once
#include "raylib.h"
#include "ui_input.h"
#include <cmath>
#include <array>
#include <string_view>

// Presentation coordinates only. Actual traversal/rewards remain in StoryData.
// TODO: Replace these schematic coordinates with authored world-map locations.
namespace ActionMap {
struct DragGesture {
    bool tracking=false, dragging=false;
    float startX=0, startY=0, startScroll=0;
    bool Update(const UiPointer& p,Rectangle view,float maxScroll,float& scroll) {
        if(p.pressed && p.Hit(view)) {tracking=true;dragging=false;startX=p.position.x;startY=p.position.y;startScroll=scroll;}
        if(!tracking)return false;
        if(std::hypot(p.position.x-startX,p.position.y-startY)>6)dragging=true;
        const bool consumed=dragging;
        if(dragging)scroll=std::clamp(startScroll+startX-p.position.x,0.0F,maxScroll);
        if(p.released || !p.down) {tracking=false;dragging=false;}
        return consumed;
    }
};
struct Point { std::string_view id; float x, y; };
inline constexpr std::array points{
    Point{"prologue_awaken",180,370}, Point{"prologue_hidden_vent",310,215}, Point{"prologue_defeat",480,370},
    Point{"chapter1_reconnect",860,370}, Point{"chapter1_gray_snow",1160,370},
    Point{"chapter1_clocktower",1290,215}, Point{"chapter1_train",1390,510}, Point{"chapter1_crownslayer",1500,370},
    Point{"chapter2_white_refuge",1960,370}, Point{"chapter2_snow_lamp",2090,510},
    Point{"chapter2_whitefield",2240,215}, Point{"chapter2_frostnova",2380,370},
    Point{"chapter3_silent_district",2760,370}, Point{"chapter3_theater",3100,370},
    Point{"chapter3_rooftop_garden",2870,215}, Point{"chapter3_ad_screen",3260,510}, Point{"chapter3_amiya_trial",3320,215},
    Point{"chapter4_relay_station",3680,370}, Point{"chapter4_zero_ward",3810,215},
    Point{"chapter4_black_archive",3900,510}, Point{"chapter4_mon3tr",4100,370},
    Point{"chapter5_burning_city",4480,370}, Point{"chapter5_melted_command",4610,215},
    Point{"chapter5_originite_river",4670,510}, Point{"chapter5_talulah",4910,370}, Point{"chapter5_refusal",5060,510},
    Point{"chapter6_lost_archive",5450,370}, Point{"chapter6_nameless_throne",5570,215},
    Point{"chapter6_deleted_dialogue",5730,510}, Point{"chapter6_first_exit",6000,510},
    Point{"ending_normal",5240,370}, Point{"ending_prts_core",6300,510}, Point{"ending_hidden",6600,510}
};
inline constexpr Rectangle View{42,180,815,414};
inline constexpr Rectangle Start{909,559,300,52}, Close{1181,184,30,30};
inline constexpr Rectangle Prev{58,606,56,32}, Next{128,606,56,32};
inline constexpr Rectangle Chapter(int i) { return {52.0F+i*149,104,141,49}; }
inline const Point* Find(std::string_view id) { for(const auto& p:points) if(p.id==id)return &p; return nullptr; }
inline Vector2 Position(const Point& p,float scroll) { return {p.x-scroll,p.y}; }
inline Rectangle Bounds(const Point& p,float scroll) { return {p.x-scroll-77,p.y-26,154,68}; }
}
