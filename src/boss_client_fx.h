#pragma once
#include "story_boss.h"
#include <map>
#include <memory>
#include <string>
class UiFont;
// Original client texture layers, projected into the side-view battle lane.
class BossClientFx {
public:
    BossClientFx();
    ~BossClientFx();
    BossClientFx(const BossClientFx&)=delete;
    BossClientFx& operator=(const BossClientFx&)=delete;
    bool Ready() const;
    bool ReadyFor(StoryBossKind kind) const;
    void Draw(const StoryBoss& boss,const UiFont& font) const;
    void DrawStory(const StoryBoss& boss,const UiFont& font) const;
    void DrawCrown(const Boss& boss,const UiFont& font,Texture2D sprite) const;
    void DrawForeground(const StoryBoss& boss) const;
    void Atmosphere(const StoryBoss& boss,float camera,const UiFont& font) const;
    void Weather(const StoryBoss& boss,float camera,bool foreground) const;
private:
    friend struct BossClientFxTestAccess;
    struct FxFrame {float size=1;Color color=WHITE;int tile=0;};
    struct FxLayer {std::string texture;float life=0,delay=0;int columns=1,rows=1;std::vector<FxFrame> frames;};
    using TimelineMap = std::map<std::string,std::map<std::string,FxLayer,std::less<>>,std::less<>>;
    using TextureMap = std::map<std::string,Texture2D,std::less<>>;
    struct Resources {
        Resources();
        ~Resources();
        TimelineMap timelines_;
        TextureMap textures_;
    };
    static std::shared_ptr<Resources> AcquireResources();
    // Both Boss renderers use the same immutable client assets. A weak cache
    // releases GPU resources when the last renderer dies, before CloseWindow.
    std::shared_ptr<Resources> resources_;
    const TimelineMap& timelines_;
    bool TimelineLayer(const char* clip,const char* layer,float seconds,Vector2 at,float width,float height,
        float rotation=0,float opacity=1,bool additive=false,float exposure=1,float anchorY=.5F,float anchorX=.5F) const;
    void Quad(const char* id,Vector2 at,float width,float height,float rotation,Color color,int frame=0,int columns=1,int rows=1) const;
    void GroundWarning(Vector2 at,float radius,float pulse,Color color) const;
    void DrawSpear(Vector2 at,float height,float rotation,Color color) const;
    void Explosion(Vector2 at,float radius,float seconds,bool c4) const;
    void ExplosionSmoke(Vector2 at,float radius,float seconds,bool c4) const;
    void GrenadeTrail(const BossGrenade& grenade) const;
    void FrostAttackRing(Vector2 at,float radius,float time,float opacity) const;
    void Revival(const StoryBoss& boss,float time,float opacity) const;
    void DrawExpansion(const StoryBoss& boss,const UiFont& font) const;
    void DrawExpansionForeground(const StoryBoss& boss) const;
    const TextureMap& textures_;
};
