#include "main_menu.h"
#include "world_layout.h"
#include "terminal_ui.h"
#include "ui_font.h"
#include <algorithm>

namespace {
using namespace TerminalUi;
struct CityPoint {const char* id;float x,y;};
constexpr CityPoint cityCityPoints[]{
    {"clinic",180,370},{"gray",420,370},{"entry",660,370},{"bridge",900,370},
    {"wtower",1140,370},{"comm",1380,370},{"ice",1620,370},{"pipes",1860,370},
    {"industry",2100,370},{"burn",2340,370},{"core",2580,370},{"normal",2820,370},
    {"clock",540,215},{"station",780,510},{"ward",420,510},{"well",1500,510},
    {"shelter",1980,510},{"root",2700,510},{"prts",2940,510},{"throne",3180,510}};
constexpr const char* tabs[]{"城区起点","高架桥","通讯塔","冰封街区","工业区","城市核心"};
constexpr int tabIndices[]{0,2,4,6,8,10};
const CityPoint& CityPosition(std::string_view id){for(const auto& p:cityCityPoints)if(p.id==id)return p;return cityCityPoints[0];}
Rectangle CityBounds(const CityPoint& p,float scroll){return {p.x-scroll-77,p.y-26,154,68};}
Rectangle Tab(int i){return {52+i*200.0F,104,192,49};}
int Chapter(float x){int result=0;for(int i=0;i<6;++i)if(x>=cityCityPoints[tabIndices[i]].x)result=i;return result;}
const char* Type(const WorldLayout::Region& r){return r.kind==WorldLayout::Kind::Boss?"Boss":WorldLayout::Hidden(r.id)?"探索":r.kind==WorldLayout::Kind::Ending?"终点":"主线";}
}

void MainMenu::OpenStageSelect() {
    page_=Page::CityStage;mapDrag_={};mapPreview_=progress_.SelectedRegion();
    mapScroll_=std::max(0.0F,CityPosition(mapPreview_).x-220);
}
MenuAction MainMenu::UpdateCityStage() {
    using namespace ActionMap;
    float last=2820;for(const auto& p:cityCityPoints)if(progress_.RegionVisible(p.id))last=std::max(last,p.x);
    const float maxScroll=last-500;
    if(mapDrag_.Update(pointer_,View,maxScroll,mapScroll_)) {pointer_.captured=false;pointerState_.captured=false;return MenuAction::None;}
    if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_BACKSPACE)){OpenHome();return MenuAction::None;}
    if(pointer_.Clicked(Close)){mapPreview_.clear();return MenuAction::None;}
    for(int i=0;i<6;++i)if(pointer_.Clicked(Tab(i))){mapScroll_=std::clamp(cityCityPoints[tabIndices[i]].x-180,0.0F,maxScroll);mapPreview_.clear();return MenuAction::None;}
    float shift=pointer_.Hit(View)?-GetMouseWheelMove()*110:0;
    if(pointer_.Clicked(Prev))shift-=310;if(pointer_.Clicked(Next))shift+=310;
    if(shift)mapScroll_=std::clamp(mapScroll_+shift,0.0F,maxScroll);
    const int direction=(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D)?1:0)-(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_A)?1:0);
    if(direction){
        std::vector<const CityPoint*> visible;for(const auto& p:cityCityPoints)if(progress_.RegionVisible(p.id))visible.push_back(&p);
        std::sort(visible.begin(),visible.end(),[](auto a,auto b){return a->x<b->x;});
        int index=0;for(int i=0;i<int(visible.size());++i)if(visible[i]->id==mapPreview_)index=i;
        index=std::clamp(index+direction,0,int(visible.size())-1);mapPreview_=visible[index]->id;
        mapScroll_=std::clamp(visible[index]->x-380,0.0F,maxScroll);
    }
    if(pointer_.Hit(View))for(const auto& p:cityCityPoints){const auto r=CityBounds(p,mapScroll_);
        if(r.x<View.x||r.x+r.width>View.x+View.width)continue;
        if(progress_.RegionVisible(p.id)&&pointer_.Clicked(r)){mapPreview_=p.id;return MenuAction::None;}}
    if(pointer_.Clicked(MapTask)){mapPreview_=progress_.SelectedRegion();mapScroll_=std::clamp(CityPosition(mapPreview_).x-220,0.0F,maxScroll);}
    if(IsKeyPressed(KEY_ENTER)||(!mapPreview_.empty()&&pointer_.Clicked(Start))){
        if(mapPreview_.empty())mapPreview_=progress_.SelectedRegion();
        else if(progress_.SelectRegion(mapPreview_)){worldRegion_=mapPreview_;return MenuAction::StartWorldPreview;}
        else { SyncError(); feedback_=MenuFeedback::Denied; }
    }
    return MenuAction::None;
}
void MainMenu::DrawCityStage(const UiFont& font) const {
    using namespace TerminalUi;using namespace ActionMap;
    Background(font,"终端 / 主线行动","MAIN THEME / CHERNOBOG");
    font.Skin().Draw("stage_bg",{0,91,1280,562});
    DrawRectangleGradientH(0,153,880,490,Fade(Ink,.15F),Fade(Ink,.52F));
    const int chapter=Chapter(mapScroll_+220);
    for(int i=0;i<6;++i){const auto r=PressedRect(Tab(i));DrawRectangleRec(r,Fade(Ink,.75F));
        font.Draw(TextFormat("SECTOR  %02d",i),r.x+12,r.y+8,11,Paper);
        Fit(font,tabs[i],{r.x+12,r.y+23,r.width-24,20},14,Paper);PressedVeil(Tab(i));}
    const auto tab=Tab(chapter);DrawRectangle(int(tab.x),int(tab.y+tab.height-3),int(tab.width),3,Orange);
    const std::string art="stage_scene_main_"+std::to_string(chapter);
    font.Skin().Draw(art.c_str(),{55,171,310,247},Fade(WHITE,.17F));
    for(const auto& route:WorldLayout::Routes){
        if(!progress_.RegionVisible(route.from)||!progress_.RegionVisible(route.to))continue;
        const auto& from=CityPosition(route.from);const auto& to=CityPosition(route.to);
        Vector2 a{from.x-mapScroll_,from.y},b{to.x-mapScroll_,to.y};if(a.x>b.x)std::swap(a,b);
        if(b.x<View.x||a.x>View.x+View.width)continue;
        a.x=std::clamp(a.x,View.x,View.x+View.width);b.x=std::clamp(b.x,View.x,View.x+View.width);
        const float bend=std::clamp(a.x+65,View.x,View.x+View.width);
        const Color color=route.kind==WorldLayout::RouteKind::Main?Orange:Warm;
        DrawLineEx(a,{bend,a.y},3,Fade(color,.8F));DrawLineEx({bend,a.y},{bend,b.y},3,Fade(color,.8F));DrawLineEx({bend,b.y},b,3,Fade(color,.8F));
    }
    for(const auto& p:cityCityPoints){
        if(!progress_.RegionVisible(p.id))continue;const auto r=CityBounds(p,mapScroll_);
        if(r.x<View.x||r.x+r.width>View.x+View.width)continue;
        const auto& region=*WorldLayout::Find(p.id);const bool selected=mapPreview_==p.id;
        const bool available=progress_.RegionAvailable(p.id),visited=progress_.RegionVisited(p.id);
        const auto d=PressedRect(r);
        font.Skin().Draw(selected?"stage_bkg_hilight":p.y==370?"stage_bkg_normal":"stage_bkg_normal_branch",d,available?WHITE:Fade(WHITE,.45F));
        Fit(font,Type(region),{d.x+30,d.y+9,102,25},23,selected?Paper:Ink);
        Fit(font,region.name,{d.x+18,d.y+44,128,23},17,available?Paper:Muted);
        if(selected)font.Skin().Draw("stage_sprite_track_point_frame",{d.x-49,d.y+3,39,39});
        font.Draw(visited?"已探索":available?"可进入":"尚未抵达",d.x+38,d.y+75,12,available?Paper:Muted);PressedVeil(r);
    }
    const auto* selected=WorldLayout::Find(mapPreview_);
    font.Skin().Draw("stage_btn_shadow",{876,160,372,484});
    if(selected&&progress_.RegionVisible(selected->id)){
        font.Skin().Draw("stage_sprite_background",{887,171,347,463});
        const std::string picture="stage_main_"+std::to_string(Chapter(CityPosition(selected->id).x));
        font.Skin().Draw(picture.c_str(),{899,181,323,191});font.Skin().Draw("stage_btn_close",PressedRect(Close));PressedVeil(Close);
        Fit(font,selected->name,{909,380,300,34},28,Ink);
        Fit(font,WorldLayout::Layers[selected->layer],{909,415,300,23},14,Ink);
        // Do not use the full design notes here: they disclose undiscovered exits.
        Fit(font,selected->subtitle,{909,448,300,28},19,Ink);
        font.Draw("区域探索 / 切城行动",909,494,16,Ink);
        const bool available=progress_.RegionAvailable(selected->id);
        font.Draw(available?"进入区域，沿出口继续探索":"沿已开放的主线出口抵达此区域",909,530,14,Ink);
        const auto start=PressedRect(Start);font.Skin().DrawRegion("stage_bkg_hilight",{18,10,130,34},start,available?WHITE:Fade(WHITE,.4F));
        font.Skin().Draw("stage_confirm_icon",{start.x+15,start.y+10,32,32},Paper);
        Fit(font,available?"开始行动":"尚未抵达",{start.x+64,start.y,start.width-84,start.height},27,Paper);PressedVeil(Start);
    }else{font.Skin().Draw(art.c_str(),{889,198,343,273});font.Draw("请选择行动节点",970,493,21,Paper);}
    Button(font,Prev,"<",pointer_.Hit(Prev),Ink,Paper);Button(font,Next,">",pointer_.Hit(Next),Ink,Paper);
    Button(font,MapTask,"当前区域",pointer_.Hit(MapTask),Ink,Paper);
    font.Draw("主线路径",604,615,13,Orange);font.Draw("已发现支路",727,615,13,Warm);
    Footer(font,"按住左键拖动 · 点击节点查看简报 · 滚轮横移 · Enter 开始行动 · Esc 返回");
}
