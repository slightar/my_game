#pragma once
#include "raylib.h"
#include "world_layout.h"
#include <map>
#include <string>
#include <array>
class WorldScenery {
public:
    ~WorldScenery();
    WorldScenery()=default;
    WorldScenery(const WorldScenery&)=delete;
    WorldScenery& operator=(const WorldScenery&)=delete;
    bool Load(const WorldLayout::Region& region,bool wardShortcutOpen=false);
    void Background(const WorldLayout::Region& region,float camera,float time) const;
    void Foreground(const WorldLayout::Region& region,float camera) const;
    bool DepthReady() const { return depthBack_.id&&depthFront_.id&&frontBounds_[0].height>0&&frontBounds_[1].height>0&&frontBounds_[2].height>0; }
    static std::vector<Rectangle> Platforms(const WorldLayout::Region& region);
private:
    Texture2D background_{};
    Texture2D depthBack_{},depthFront_{};
    std::array<Rectangle,3> frontBounds_{};
    std::string depthTheme_;
    std::string depthScene_;
    std::string loaded_;
};
