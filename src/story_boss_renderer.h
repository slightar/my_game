#pragma once
#include "story_boss.h"
#include "boss_client_fx.h"
#include <array>
#include <vector>
class UiFont;

class StoryBossRenderer {
public:
    StoryBossRenderer();
    ~StoryBossRenderer();
    StoryBossRenderer(const StoryBossRenderer&)=delete;
    StoryBossRenderer& operator=(const StoryBossRenderer&)=delete;
    bool Ready(StoryBossKind kind) const;
    void Draw(const StoryBoss& boss,const UiFont& font) const;
    void DrawHud(const StoryBoss& boss,const UiFont& font,const char* region) const;
    void DrawAtmosphere(const StoryBoss& boss,float camera,const UiFont& font) const {fx_.Atmosphere(boss,camera,font);}
    void DrawWeather(const StoryBoss& boss,float camera,bool foreground=false) const {fx_.Weather(boss,camera,foreground);}
    bool EffectsReady() const {return fx_.Ready();}
private:
    void DrawWinter(const StoryBoss& boss,const UiFont& font) const;
    void DrawFrostSmooth(const StoryBoss& boss,const UiFont& font) const;
    void DrawCrownslayer(const StoryBoss& boss,const UiFont& font) const;
    struct Art {
        Texture2D texture{};
        Vector2 anchor{};
        float size=256;
        int cell=256;
        std::array<float,11> durations{1,1,1,.5F,.5F,1,1,1,1,1,1};
    };
    std::array<Art,kStoryBossCount> art_{};
    Texture2D crownBattle_{};
    struct SmoothFrame {Rectangle source{};Vector2 crop{};};
    struct SmoothClip {float duration=1;std::vector<SmoothFrame> frames;};
    Texture2D frostSmooth_{};
    Vector2 frostSmoothAnchor_{};
    std::array<SmoothClip,7> frostClips_{};
    BossClientFx fx_;
};
