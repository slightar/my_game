#pragma once
#include "raylib.h"
#include "world_layout.h"
#include <map>
#include <string>
class WorldScenery {
public:
    ~WorldScenery();
    WorldScenery()=default;
    WorldScenery(const WorldScenery&)=delete;
    WorldScenery& operator=(const WorldScenery&)=delete;
    bool Load(const WorldLayout::Region& region,bool wardShortcutOpen=false);
    void Background(const WorldLayout::Region& region,float camera,float time) const;
    static std::vector<Rectangle> Platforms(const WorldLayout::Region& region);
private:
    Texture2D background_{};
    std::string loaded_;
};
