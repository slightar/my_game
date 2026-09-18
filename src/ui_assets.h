#pragma once
#include "raylib.h"
#include <map>
#include <string>

// Own GPU textures for the lifetime of the UI, including missing-asset fallbacks.
class UiAssets {
public:
    UiAssets() {
        for (const char* name : {"home_battle", "home_operator", "rhodes", "back",
             "dots", "tag", "vignette", "sniper", "guard", "stars", "skill_shadow",
             "ring", "direction", "panel", "hp", "sp", "pause"}) {
            const auto path = std::string(GetApplicationDirectory()) + "assets/ui/client/" + name + ".png";
            if (!FileExists(path.c_str())) continue;
            auto texture = LoadTexture(path.c_str());
            if (texture.id) { SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR); textures_[name] = texture; }
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
private:
    std::map<std::string, Texture2D> textures_;
};
