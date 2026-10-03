#pragma once
#include "world_art.h"
#include "raylib.h"
#include <vector>

// Converts authored geometry into always-present one-way platforms.
// No input handling, teleporting, or street/upper-floor mode switches.
class SceneTraversal {
public:
    void Enter(std::string_view id) {
        platforms_.clear();hasRamp_=false;
        const auto* stairs=WorldArt::Stairs(id);if(!stairs)return;
        const float width=WorldArt::Width(id),top=WorldArt::UpperFloor(id);
        const float bottom=644+(stairs->bottom-WorldArt::PaintedGround(id))*(644/WorldArt::Find(id)->ground);
        ramp_={stairs->left*width,top,(stairs->right-stairs->left)*width,bottom-top};hasRamp_=true;
        platforms_.push_back({stairs->landingLeft*width,top,(stairs->landingRight-stairs->landingLeft)*width,12});
    }
    const Rectangle* Ramp() const {return hasRamp_?&ramp_:nullptr;}
    const std::vector<Rectangle>* Platforms() const {return &platforms_;}
private:
    bool hasRamp_=false;
    Rectangle ramp_{};
    std::vector<Rectangle> platforms_;
};
