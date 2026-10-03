#include "main_menu.h"
#include "world_layout.h"
#include "ui_font.h"
#include "terminal_ui.h"
#include <algorithm>
#include <cmath>

namespace {
using namespace WorldLayout;
constexpr Rectangle View{132,194,794,402}, Enter{957,574,280,46};
constexpr Color Background{15,25,34,255}, Panel{23,36,47,255};
constexpr Color Cyan{85,205,209,255}, Gold{212,168,107,255}, Pink{225,169,192,255};
constexpr Color Red{237,120,110,255}, Text{232,240,243,255}, Muted{155,174,187,255};
constexpr float MaxScroll=1300;
float Y(int layer){return 227+layer*66.0F;}
Rectangle Card(const Region& r,float scroll){return {r.mapX-scroll-76,Y(r.layer)-24,152,48};}
Color Accent(const Region& r) {
    return r.kind==Kind::Boss?Red:r.kind==Kind::Exploration?Gold:r.kind==Kind::Memory?Pink:Cyan;
}
Color RouteColor(RouteKind kind){return kind==RouteKind::Main?Cyan:kind==RouteKind::Exploration?Gold:Pink;}
constexpr std::array<const char*,5> JumpIds{"clinic","bridge","pipes","core","prts"};
constexpr std::array<const char*,5> JumpLabels{"起点","高架段","地下段","城市核心","系统空间"};
Rectangle Jump(int i){return {132+i*159.0F,152,150,28};}
void Segment(Vector2 a,Vector2 b,Color color,bool dashed) {
    const float len=std::hypot(b.x-a.x,b.y-a.y);
    if(len<.01F)return;
    if(!dashed){DrawLineEx(a,b,2,color);return;}
    for(float t=0;t<len;t+=14) {
        const float end=std::min(t+7,len);
        DrawLineEx({a.x+(b.x-a.x)*t/len,a.y+(b.y-a.y)*t/len},
                   {a.x+(b.x-a.x)*end/len,a.y+(b.y-a.y)*end/len},2,color);
    }
}
void Arrow(Vector2 a,Vector2 b,Color color) {
    const float len=std::hypot(b.x-a.x,b.y-a.y);if(len<.01F)return;
    const Vector2 u{(b.x-a.x)/len,(b.y-a.y)/len};
    DrawLineEx(b,{b.x-u.x*7-u.y*4,b.y-u.y*7+u.x*4},2,color);
    DrawLineEx(b,{b.x-u.x*7+u.y*4,b.y-u.y*7-u.x*4},2,color);
}
void Connection(const Route& route,float scroll) {
    const auto& f=*Find(route.from);const auto& t=*Find(route.to);
    const auto fr=Card(f,scroll),tr=Card(t,scroll);
    Vector2 a{fr.x+76,fr.y+24},b{tr.x+76,tr.y+24};
    const Color color=Fade(RouteColor(route.kind),.78F);
    const bool dashed=route.kind!=RouteKind::Main;
    std::vector<Vector2> path;
    // The final shaft skirts the burning-city card; system-space lines stay below
    // physical layers. The ward loop routes west of the clinic, as in the reference.
    if(std::string_view(route.from)=="core"&&std::string_view(route.to)=="root") {
        a={fr.x+fr.width,fr.y+24};b={tr.x+tr.width,tr.y+24};
        path={a,{a.x+28,a.y},{a.x+28,b.y},b};
    } else if(std::string_view(route.from)=="ward"&&std::string_view(route.to)=="clinic") {
        a={fr.x,fr.y+24};b={tr.x+76,tr.y+48};
        path={a,{b.x,a.y},b};
    } else if(f.layer==t.layer) {
        const float sign=t.mapX>f.mapX?1.0F:-1.0F;
        a.x+=sign*76;b.x-=sign*76;path={a,b};
    } else if(std::abs(f.mapX-t.mapX)<5) {
        const float sign=t.layer>f.layer?1.0F:-1.0F;
        a.y+=sign*24;b.y-=sign*24;path={a,b};
    } else {
        const float sign=t.layer>f.layer?1.0F:-1.0F;
        a.y+=sign*24;b.y-=sign*24;
        const float mid=(a.y+b.y)*.5F;path={a,{a.x,mid},{b.x,mid},b};
    }
    for(std::size_t i=1;i<path.size();++i)Segment(path[i-1],path[i],color,dashed);
    Arrow(path[path.size()-2],path.back(),color);
}
}

void MainMenu::OpenWorldMap(const std::string& region) {
    if(WorldLayout::Find(region))worldRegion_=region;
    if(!WorldLayout::Find(worldRegion_))worldRegion_="clinic";
    page_=Page::WorldMap;worldDrag_={};status_.clear();
    worldScroll_=std::clamp(WorldLayout::Find(worldRegion_)->mapX-365,0.0F,MaxScroll);
}

MenuAction MainMenu::UpdateWorldMap() {
    using namespace WorldLayout;
    if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_BACKSPACE)){OpenHome();return MenuAction::None;}
    if(IsKeyPressed(KEY_F9)){OpenStageSelect();return MenuAction::None;}
    if(worldDrag_.Update(pointer_,View,MaxScroll,worldScroll_)) {
        pointer_.captured=false;pointerState_.captured=false;return MenuAction::None;
    }
    if(pointer_.Hit(View))worldScroll_=std::clamp(worldScroll_-GetMouseWheelMove()*100,0.0F,MaxScroll);
    for(int i=0;i<5;++i)if(pointer_.Clicked(Jump(i))){OpenWorldMap(JumpIds[i]);return MenuAction::None;}
    if(pointer_.Clicked({132,612,44,28}))worldScroll_=std::max(0.0F,worldScroll_-350);
    if(pointer_.Clicked({184,612,44,28}))worldScroll_=std::min(MaxScroll,worldScroll_+350);
    if(pointer_.Hit(View))for(const auto& r:Regions)
        if(pointer_.Clicked(Card(r,worldScroll_))){worldRegion_=r.id;return MenuAction::None;}
    const auto ports=Ports(worldRegion_);
    for(int i=0;i<int(ports.size());++i)
        if(pointer_.Clicked({957,369+i*36.0F,280,30})){OpenWorldMap(ports[i].target->id);return MenuAction::None;}
    if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_RIGHT)) {
        int i=0;for(;i<int(Regions.size());++i)if(Regions[i].id==worldRegion_)break;
        i=(i+(IsKeyPressed(KEY_RIGHT)?1:int(Regions.size())-1))%int(Regions.size());
        OpenWorldMap(Regions[i].id);
    }
    if(IsKeyPressed(KEY_ENTER)||pointer_.Clicked(Enter))return MenuAction::StartWorldPreview;
    return MenuAction::None;
}

void MainMenu::DrawWorldMap(const UiFont& font) const {
    using namespace WorldLayout;
    using TerminalUi::Fit;
    DrawRectangle(0,0,1280,720,Background);
    TerminalUi::Button(font,TerminalUi::Back,"< 返回",pointer_.Hit(TerminalUi::Back),Panel,Text);
    font.Draw("切尔诺伯格 / 城区横截面",146,27,28,Text);
    font.Draw("区域规划 · 20 个区域 · 6 个层级",827,39,16,Muted);
    DrawRectangleRec({30,88,1208,49},Color{42,37,53,255});
    font.Draw("序章异常接入：诊疗所 → 魔王王座 → 茧笼断线 → 诊疗所重生",45,97,17,Pink);
    font.Draw("王座属于非物理空间；终章由数据根系接入。",737,116,13,Muted);
    for(int i=0;i<5;++i)TerminalUi::Button(font,Jump(i),JumpLabels[i],pointer_.Hit(Jump(i)),Panel,Text);
    for(int layer=0;layer<6;++layer) {
        DrawRectangleRec({30,Y(layer)-31,896,62},layer%2?Color{19,32,43,255}:Panel);
        Fit(font,Layers[layer],{37,Y(layer)-10,90,24},14,layer==5?Pink:Muted);
    }
    // Clip in actual screen pixels, including letterboxing for non-1280 windows.
    const float scale=std::min(GetScreenWidth()/1280.0F,GetScreenHeight()/720.0F);
    const float ox=(GetScreenWidth()-1280*scale)/2,oy=(GetScreenHeight()-720*scale)/2;
    BeginScissorMode(int(ox+View.x*scale),int(oy+View.y*scale),int(View.width*scale),int(View.height*scale));
    for(const auto& route:Routes)Connection(route,worldScroll_);
    for(const auto& r:Regions) {
        const auto box=Card(r,worldScroll_);const bool selected=worldRegion_==r.id;
        if(box.x+box.width<View.x||box.x>View.x+View.width)continue;
        const Color accent=Accent(r);
        DrawRectangleRec(box,selected?Color{48,66,76,255}:Color{27,43,54,255});
        DrawRectangleLinesEx(box,selected?2:1,Fade(accent,selected?1:.70F));
        DrawRectangleRec({box.x,box.y,4,box.height},accent);
        Fit(font,r.name,{box.x+12,box.y+7,132,21},18,Text);
        Fit(font,r.subtitle,{box.x+12,box.y+30,132,15},12,accent);
        TerminalUi::PressedVeil(box);
    }
    EndScissorMode();
    const auto& selected=*Find(worldRegion_);
    DrawRectangleRec({943,152,307,490},Panel);
    const Color accent=Accent(selected);
    DrawRectangleRec({943,152,307,3},accent);
    font.Draw("区域信息",958,172,13,Muted);
    Fit(font,selected.name,{957,198,280,36},29,Text);
    font.Draw(Layers[selected.layer],958,244,16,accent);
    Fit(font,selected.subtitle,{957,270,280,23},17,Muted);
    TerminalUi::Wrap(font,selected.note,{957,303,280,56},15,Text);
    const auto ports=Ports(worldRegion_);
    for(int i=0;i<int(ports.size());++i) {
        const auto& port=ports[i];const Rectangle box{957,369+i*36.0F,280,30};
        DrawRectangleRec(box,Color{32,49,60,255});
        Fit(font,std::string(port.returning?"返回 / ":"前往 / ")+port.target->name,{box.x+9,box.y+5,260,21},15,RouteColor(port.route->kind));
        TerminalUi::PressedVeil(box);
    }
    if(ports.empty())font.Draw("已抵达终点",958,380,18,Pink);
    font.Draw("内部细节待建设 · 当前可自由预览",957,548,13,Muted);
    TerminalUi::Button(font,Enter,"进入空白区域",pointer_.Hit(Enter),Color{38,102,112,255},Text);
    TerminalUi::Button(font,{132,612,44,28},"<",false,Panel,Text);
    TerminalUi::Button(font,{184,612,44,28},">",false,Panel,Text);
    DrawRectangle(248,624,193,3,Color{42,58,70,255});
    DrawRectangle(int(248+worldScroll_/MaxScroll*153),622,40,7,Cyan);
    font.Draw("主线",476,618,14,Cyan);font.Draw("探索支路",560,618,14,Gold);font.Draw("系统接入",686,618,14,Pink);
    font.Draw("箭头标记流程方向；普通通道在空白区域中可双向回访。",132,642,11,Muted);
    TerminalUi::Footer(font,"拖动 / 滚轮横移 · 点击区域查看 · 左右键选择 · Enter 进入 · Esc 返回");
}
