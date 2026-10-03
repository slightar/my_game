#include "world_scenery.h"
#include "file_path.h"
#include "game_types.h"

using WorldLayout::Region;
WorldScenery::~WorldScenery(){if(background_.id)UnloadTexture(background_);}
bool WorldScenery::Load(const Region& r,bool wardShortcutOpen){
    const auto folder=Utf8Path(GetApplicationDirectory())/"assets/environment"/
        (WorldArt::Repainted(r.id)?"scaled":"regions");
    const auto* scene=WorldArt::Find(r.id);
    const std::string art=std::string(r.id)=="clinic"&&!wardShortcutOpen?"clinic_closed":scene&&scene->imageOverride?scene->imageOverride:r.id;
    if(art!=loaded_||!background_.id){
        if(background_.id)UnloadTexture(background_);
        background_=LoadTexture((folder/(art+".png")).string().c_str());loaded_=art;
        if(background_.id)SetTextureFilter(background_,TEXTURE_FILTER_BILINEAR);
    }
    return background_.id!=0;
}
std::vector<Rectangle> WorldScenery::Platforms(const Region&){return {};}
void WorldScenery::Background(const Region& r,float camera,float) const {
    if(!background_.id)return;
    // One authored panorama: no mirrored seams, overlaid doors, or visible
    // collision geometry. The floor and recessed passages are part of the art.
    const auto* scene=WorldArt::Find(r.id);if(!scene)return;
    const float ground=scene->ground;
    const float height=GameConfig::kFloorY/ground;
    const float width=height*float(background_.width)/float(background_.height);
    const float y=GameConfig::kFloorY-WorldArt::PaintedGround(r.id)*height;
    const float x=640-camera;
    DrawTexturePro(background_,{0,0,float(background_.width),float(background_.height)},
                   {x,y,width,height},{},0,WHITE);
}
