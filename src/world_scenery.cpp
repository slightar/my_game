#include "world_scenery.h"
#include "file_path.h"
#include "game_types.h"
#include "world_depth.h"
#include <cmath>
#include <algorithm>

using WorldLayout::Region;
namespace {
Texture2D LoadForeground(const std::string& path,std::array<Rectangle,3>& bounds) {
    bounds={};
    auto image=LoadImage(path.c_str());if(!image.data)return {};
    auto* pixels=LoadImageColors(image);
    if(!pixels){UnloadImage(image);return {};}
    std::vector<int> columns(image.width,0);
    for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x)
        if(pixels[y*image.width+x].a>16)++columns[x];
    std::array<int,4> cuts{0,image.width/3,image.width*2/3,image.width};
    // Find the transparent gutters near each third; do not slice off a wheel
    // or cable just because the artist's groups are not exactly equally wide.
    for(int i=1;i<3;++i) {
        const int ideal=cuts[i];int best=ideal;
        for(int x=std::max(1,ideal-image.width/12);x<std::min(image.width-1,ideal+image.width/12);++x)
            if(columns[x]<columns[best]||(columns[x]==columns[best]&&std::abs(x-ideal)<std::abs(best-ideal)))best=x;
        cuts[i]=best;
    }
    for(int i=0;i<3;++i) {
        int left=cuts[i+1],right=cuts[i]-1,top=image.height,bottom=-1;
        for(int y=0;y<image.height;++y)for(int x=cuts[i];x<cuts[i+1];++x)if(pixels[y*image.width+x].a>16) {
            left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);
        }
        if(bottom>=0) {
            left=std::max(cuts[i],left-2);right=std::min(cuts[i+1]-1,right+2);
            top=std::max(0,top-2);bottom=std::min(image.height-1,bottom+2);
            bounds[i]={float(left),float(top),float(right-left+1),float(bottom-top+1)};
        }
    }
    UnloadImageColors(pixels);
    const auto texture=LoadTextureFromImage(image);UnloadImage(image);return texture;
}
void DrawDepth(Texture2D texture,const WorldDepth::Layer& layer,float camera,std::string_view id) {
    if(!texture.id)return;
    const float width=layer.height*float(texture.width)/float(texture.height);
    unsigned phase=2166136261U;
    for(const unsigned char c:id)phase=(phase^c)*16777619U;
    // World-anchored offsets: revisiting a scene and walking backwards is stable.
    const float shift=WorldDepth::Offset(camera,layer.speed)+float(phase%997)/997*width;
    const float first=shift-std::floor(shift/width)*width-width;
    for(float x=first;x<1280;x+=width)
        DrawTexturePro(texture,{0,0,float(texture.width),float(texture.height)},
                       {x,layer.y,width,layer.height},{},0,Fade(WHITE,layer.opacity));
}
}
WorldScenery::~WorldScenery(){
    if(background_.id)UnloadTexture(background_);
    if(depthBack_.id)UnloadTexture(depthBack_);
    if(depthFront_.id)UnloadTexture(depthFront_);
}
bool WorldScenery::Load(const Region& r,bool wardShortcutOpen){
    const auto folder=Utf8Path(GetApplicationDirectory())/"assets/environment"/
        (WorldArt::Repainted(r.id)?"scaled":"regions");
    const auto* scene=WorldArt::Find(r.id);
    const std::string art=scene&&scene->closedImageOverride&&!wardShortcutOpen?scene->closedImageOverride:scene&&scene->imageOverride?scene->imageOverride:r.id;
    if(art!=loaded_||!background_.id){
        if(background_.id)UnloadTexture(background_);
        background_=LoadTexture((folder/(art+".png")).string().c_str());loaded_=art;
        if(background_.id)SetTextureFilter(background_,TEXTURE_FILTER_BILINEAR);
    }
    const std::string theme(WorldDepth::Theme(r.id));
    const auto layers=Utf8Path(GetApplicationDirectory())/"assets/environment/depth";
    if(theme!=depthTheme_||!depthBack_.id) {
        if(depthBack_.id)UnloadTexture(depthBack_);
        depthBack_=LoadTexture((layers/(std::string(WorldDepth::BackTheme(theme))+WorldDepth::Back.suffix+".png")).string().c_str());
        if(depthBack_.id)SetTextureFilter(depthBack_,TEXTURE_FILTER_BILINEAR);
        depthTheme_=theme;
    }
    // Two hospital rooms can share a distant ceiling while retaining their own
    // foreground objects. Cache the foreground by scene, not by theme.
    if(r.id!=depthScene_||!depthFront_.id) {
        if(depthFront_.id)UnloadTexture(depthFront_);
        depthFront_=LoadForeground((layers/(std::string(r.id)+"_front_v2.png")).string(),frontBounds_);
        if(depthFront_.id)SetTextureFilter(depthFront_,TEXTURE_FILTER_BILINEAR);
        depthScene_=r.id;
    }
    return background_.id!=0;
}
std::vector<Rectangle> WorldScenery::Platforms(const Region&){return {};}
void WorldScenery::Background(const Region& r,float camera,float) const {
    if(!background_.id)return;
    // Passage art and the walkable floor stay at exactly the world camera speed.
    const auto* scene=WorldArt::Find(r.id);if(!scene)return;
    const float ground=scene->ground;
    const float height=GameConfig::kFloorY/ground;
    const float width=height*float(background_.width)/float(background_.height);
    const float y=GameConfig::kFloorY-WorldArt::PaintedGround(r.id)*height;
    const float x=640-camera;
    DrawTexturePro(background_,{0,0,float(background_.width),float(background_.height)},
                   {x,y,width,height},{},0,WHITE);
    DrawDepth(depthBack_,WorldDepth::Back,camera,r.id);
}
void WorldScenery::Foreground(const Region& r,float camera) const {
    if(!depthFront_.id)return;
    // The three authored groups are each placed once. No repeated foreground
    // tile, mirroring, or new collision surface is introduced.
    for(int i=0;i<3;++i) {
        const auto source=frontBounds_[i];if(source.height<=0||source.width<=0)continue;
        const float scale=std::min(WorldDepth::Front.height/source.height,320.0F/source.width);
        const float width=source.width*scale,height=source.height*scale;
        const float x=WorldDepth::FrontPosition(r.width,WorldDepth::FrontGroups[i],camera)-width/2;
        if(x+width<0||x>1280)continue;
        DrawTexturePro(depthFront_,source,
                       {x,720-height,width,height},{},0,Fade(WHITE,WorldDepth::Front.opacity));
    }
}
