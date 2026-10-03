#pragma once
#include "raylib.h"
#include <map>
#include <string>

// Own GPU textures for the lifetime of the UI, including missing-asset fallbacks.
class UiAssets {
public:
    UiAssets() {
        for (const char* name : {"home_battle", "home_operator", "rhodes", "back",
             "dots", "tag", "vignette", "sniper", "guard", "stars", "skill_shadow", "prts_avatar",
             "ring", "direction", "panel", "hp", "sp", "pause",
             "map_bg", "map_bkg_normal", "map_bkg_normal_branch", "map_bkg_hilight",
             "map_details_bg", "map_icon_stage_rank_3", "map_sprite_track_point_frame"}) {
            const auto path = std::string(GetApplicationDirectory()) + "assets/ui/client/" + name + ".png";
            if (!FileExists(path.c_str())) continue;
            auto texture = LoadTexture(path.c_str());
            if (texture.id) { SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR); textures_[name] = texture; }
        }
        for(const char* name:{"bg","bkg_normal","bkg_normal_branch","bkg_hilight",
            "sprite_track_point_frame","sprite_track_point_center","icon_stage_rank_3","icon_stage_rank_0",
            "icon_boss_normal","icon_boss_hilight","scrollbar","scrollbar_bg","img_info_bg",
            "btn_close","btn_shadow","confirm_icon","image_stage_preview_back","sprite_background",
            "main_0","main_1","main_2","main_3","main_4","main_5","main_6",
            "scene_main_0","scene_main_1","scene_main_2","scene_main_3","scene_main_4","scene_main_5","scene_main_6"}) {
            const auto path=std::string(GetApplicationDirectory())+"assets/ui/stage/"+name+".png";
            if(!FileExists(path.c_str()))continue;
            auto t=LoadTexture(path.c_str());
            if(t.id){SetTextureFilter(t,TEXTURE_FILTER_BILINEAR);textures_[std::string("stage_")+name]=t;}
        }
    }
    ~UiAssets() { for (const auto& [name, texture] : textures_) UnloadTexture(texture); }
    UiAssets(const UiAssets&) = delete;
    UiAssets& operator=(const UiAssets&) = delete;
    bool Draw(const char* name, Rectangle destination, Color tint = WHITE, bool flip = false) const {
        const auto found = textures_.find(name);
        if (found == textures_.end()) return false;
        const auto& t = found->second;
        DrawTexturePro(t, {0, 0, float(t.width) * (flip ? -1 : 1), float(t.height)}, destination, {}, 0, tint);
        return true;
    }
    bool DrawRegion(const char* name,Rectangle source,Rectangle destination,Color tint=WHITE) const {
        const auto found=textures_.find(name);if(found==textures_.end())return false;
        DrawTexturePro(found->second,source,destination,{},0,tint);return true;
    }
private:
    std::map<std::string, Texture2D> textures_;
};
