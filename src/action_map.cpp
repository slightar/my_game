#include "action_map.h"
#include "main_menu.h"
#include "terminal_ui.h"
#include "ui_font.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace {
using namespace TerminalUi;
bool ChapterVisible(const ProgressSystem& progress,const StoryChapter& chapter) {
    for(const auto& id:chapter.nodeIds) if(progress.IsUnlocked(id))return true;
    return false;
}
std::string Rewards(const MapNode& node) {
    std::string result;
    for(const auto& id:node.reward.archiveIds) if(const auto* a=StoryData::FindArchive(id)) result+=a->name+" / ";
    for(const auto& id:node.reward.equipmentIds) if(const auto* e=StoryData::FindEquipment(id)) result+=e->name+" / ";
    return result.empty()?"区域记录与后续路线":result.substr(0,result.size()-3);
}
}

void MainMenu::OpenStageSelect() {
    page_=Page::Map; mapPreview_.clear();mapDrag_={};mapStartReady_=false;
    if(const auto* p=ActionMap::Find(progress_.SelectedNode()))mapScroll_=std::max(0.0F,p->x-220);
}

void MainMenu::UpdateMap(float) {
    using namespace ActionMap;
    float lastX=180;
    for(const auto& p:points)if(progress_.IsUnlocked(std::string(p.id)))lastX=std::max(lastX,p.x);
    // Keep at least one screen of travel even before subsequent regions are discovered.
    // Also accommodates chapter shortcuts (x - 140), keyboard focus and reopening.
    const float maxScroll=std::max(160.0F,lastX-140);
    if(mapDrag_.Update(pointer_,View,maxScroll,mapScroll_)) {
        pointer_.captured=false;pointerState_.captured=false;return;
    }
    // TODO: Retire this development-only legacy flow when real scenes are integrated.
    if(IsKeyPressed(KEY_F9)) {page_=Page::NodeDetail;return;}
    if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_BACKSPACE)) {
        if(!mapPreview_.empty())mapPreview_.clear();else page_=Page::Home;
        return;
    }
    if(pointer_.Clicked(Close)) {mapPreview_.clear();return;}
    for(int i=0;i<int(progress_.Chapters().size());++i) {
        const auto& c=progress_.Chapters()[i];
        if(pointer_.Clicked(Chapter(i)) && ChapterVisible(progress_,c)) {
            if(const auto* p=Find(c.nodeIds.front()))mapScroll_=std::max(0.0F,p->x-140);
            mapPreview_.clear();mapStartReady_=false;return;
        }
    }
    float shift=pointer_.Hit(View)?-GetMouseWheelMove()*110:0;
    if(pointer_.Clicked(Prev))shift-=310;
    if(pointer_.Clicked(Next))shift+=310;
    if(shift!=0)mapScroll_=std::clamp(mapScroll_+shift,0.0F,maxScroll);
    const int direction=(IsKeyPressed(KEY_RIGHT)||IsKeyPressed(KEY_D)?1:0)-(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_A)?1:0);
    if(direction) {
        std::vector<const Point*> available;
        for(const auto& p:points)if(progress_.IsUnlocked(std::string(p.id)))available.push_back(&p);
        std::sort(available.begin(),available.end(),[](auto a,auto b){return a->x<b->x;});
        int index=0;
        const auto current=mapPreview_.empty()?progress_.SelectedNode():mapPreview_;
        for(int i=0;i<int(available.size());++i)if(available[i]->id==current)index=i;
        if(!available.empty()) {
            index=std::clamp(index+direction,0,int(available.size())-1);
            mapPreview_=std::string(available[index]->id);mapStartReady_=false;mapScroll_=std::max(0.0F,available[index]->x-380);
        }
    }
    if(pointer_.Hit(View))for(const auto& p:points) {
        const auto r=Bounds(p,mapScroll_);
        if(r.x<View.x || r.x+r.width>View.x+View.width)continue;
        if(progress_.IsUnlocked(std::string(p.id)) && pointer_.Clicked(r)) {
            mapPreview_=std::string(p.id);mapStartReady_=false;return; // Selection never starts/completes a stage.
        }
    }
    if(pointer_.Clicked(MapTask)) {
        mapPreview_=progress_.SelectedNode();
        if(const auto* p=Find(mapPreview_))mapScroll_=std::clamp(p->x-220,0.0F,maxScroll);
        return;
    }
    if(IsKeyPressed(KEY_ENTER)||(!mapPreview_.empty()&&pointer_.Clicked(Start))) {
        if(mapPreview_.empty())mapPreview_=progress_.SelectedNode();
        else if(progress_.SelectNode(mapPreview_)) {
            // TODO: Dispatch the selected starting point to a continuous playable world.
            mapStartReady_=true;status_="起点已保存；地图场景待接入";
        }
    }
}

void MainMenu::DrawMap(const UiFont& font) const {
    using namespace ActionMap;
    Background(font,"终端 / 主线行动","MAIN THEME / CONNECTED REGIONS");
    font.Skin().Draw("stage_bg",{0,91,1280,562});
    DrawRectangleGradientH(0,153,880,490,Fade(Ink,.15F),Fade(Ink,.52F));
    int activeChapter=0;
    for(int i=0;i<int(progress_.Chapters().size());++i) {
        const auto& c=progress_.Chapters()[i];
        if(const auto* p=Find(c.nodeIds.front());p && p->x-mapScroll_<View.x+View.width/2)activeChapter=i;
        const bool known=ChapterVisible(progress_,c);
        const auto r=PressedRect(Chapter(i));
        DrawRectangleRec(r,Fade(Ink,.75F));
        font.Draw(TextFormat("EPISODE  %02d",i),r.x+12,r.y+8,11,known?Paper:Muted);
        Fit(font,known?c.title:"尚未探索",{r.x+12,r.y+23,r.width-24,20},14,known?Paper:Muted);
        PressedVeil(Chapter(i));
    }
    const auto tab=Chapter(activeChapter);DrawRectangle(int(tab.x),int(tab.y+tab.height-3),int(tab.width),3,Orange);
    // Native chapter artwork and stage plates; no generated grid or opaque redraw over the plates.
    const std::string chapterArt="stage_scene_main_"+std::to_string(std::min(activeChapter,6));
    font.Skin().Draw(chapterArt.c_str(),{55,171,310,247},Fade(WHITE,.17F));
    std::set<std::pair<std::string,std::string>> drawn;
    for(const auto& node:progress_.Nodes()) {
        if(!progress_.IsUnlocked(node.id))continue;
        const auto* from=Find(node.id);if(!from)continue;
        for(const auto& exit:node.exits) {
            if(!progress_.IsUnlocked(exit.targetId))continue;
            const auto* to=Find(exit.targetId);if(!to)continue;
            auto key=std::minmax(node.id,exit.targetId);
            if(!drawn.emplace(key.first,key.second).second)continue;
            Vector2 a=Position(*from,mapScroll_),b=Position(*to,mapScroll_);
            if(a.x>b.x)std::swap(a,b);
            if(b.x<View.x || a.x>View.x+View.width)continue;
            const Color color=(from->y!=370||to->y!=370)?Warm:Orange;
            const float bend=std::clamp(a.x+65,View.x,View.x+View.width);
            a.x=std::clamp(a.x,View.x,View.x+View.width);b.x=std::clamp(b.x,View.x,View.x+View.width);
            DrawLineEx(a,{bend,a.y},3,Fade(color,.85F));
            DrawLineEx({bend,a.y},{bend,b.y},3,Fade(color,.85F));
            DrawLineEx({bend,b.y},b,3,Fade(color,.85F));
        }
    }
    for(int i=0;i<int(points.size());++i) {
        const auto& p=points[i];const auto* n=progress_.FindNode(std::string(p.id));
        if(!n || !progress_.IsUnlocked(n->id))continue;
        const auto r=Bounds(p,mapScroll_);
        if(r.x<View.x || r.x+r.width>View.x+View.width)continue;
        const bool selected=mapPreview_==n->id,current=progress_.SelectedNode()==n->id,complete=progress_.IsCompleted(n->id);
        const auto d=PressedRect(r);
        const auto* sprite=selected?"stage_bkg_hilight":p.y==370?"stage_bkg_normal":"stage_bkg_normal_branch";
        font.Skin().Draw(sprite,d);
        const Color label=selected?Paper:Ink;
        font.Draw(TextFormat("%02d",i+1),d.x+34,d.y+9,24,label);
        Fit(font,NodeTypeName(n->type),{d.x+84,d.y+10,48,25},15,label);
        Fit(font,n->name,{d.x+18,d.y+44,128,23},17,Paper);
        if(complete)font.Skin().Draw("stage_icon_stage_rank_3",{d.x-1,d.y+2,30,34});
        else font.Skin().Draw("stage_icon_stage_rank_0",{d.x-1,d.y+2,30,34});
        if(selected)font.Skin().Draw("stage_sprite_track_point_frame",{d.x-49,d.y+3,39,39});
        else font.Skin().Draw("stage_sprite_track_point_center",{d.x-41,d.y+11,23,23});
        if(current)font.Draw("当前起点",d.x+38,d.y+75,12,Paper);
        PressedVeil(r);
    }
    const auto* selected=progress_.FindNode(mapPreview_);
    if(selected && progress_.IsUnlocked(selected->id)) {
        font.Skin().Draw("stage_btn_shadow",{876,160,372,484});
        font.Skin().Draw("stage_sprite_background",{887,171,347,463});
        const auto* chapter=StoryData::FindChapter(selected->chapterId);
        const std::string art="stage_main_"+std::to_string(chapter?std::min(chapter->order,6):0);
        font.Skin().Draw(art.c_str(),{899,181,323,191});
        font.Skin().Draw("stage_btn_close",PressedRect(Close));PressedVeil(Close);
        Fit(font,selected->name,{909,380,300,34},28,Ink);
        Fit(font,selected->location,{909,415,300,23},14,Ink);
        Fit(font,"任务 / "+selected->objective,{909,441,300,28},17,Ink);
        Fit(font,"角色 / "+selected->relatedCharacters,{909,472,300,20},14,Ink);
        Fit(font,"奖励 / "+Rewards(*selected),{909,497,300,25},15,Ink);
        font.Draw(mapStartReady_?"起点已就绪 · 地图场景尚未实现":"开发占位：地图场景尚未接入",909,532,14,Ink);
        const auto start=PressedRect(Start);
        font.Skin().DrawRegion("stage_bkg_hilight",{18,10,130,34},start);
        font.Skin().Draw("stage_confirm_icon",{start.x+15,start.y+10,32,32},Paper);
        Fit(font,mapStartReady_?"起点已就绪":"开始行动",{start.x+64,start.y,start.width-84,start.height},27,Paper);
        PressedVeil(Start);
    } else {
        font.Skin().Draw("stage_btn_shadow",{876,160,372,484});
        font.Skin().Draw(chapterArt.c_str(),{889,198,343,273});
        font.Draw("请选择行动节点",970,493,21,Paper);
        font.Draw("按住左键拖动路线",990,530,14,Muted);
    }
    Button(font,Prev,"<",pointer_.Hit(Prev),Ink,Paper);
    Button(font,Next,">",pointer_.Hit(Next),Ink,Paper);
    Button(font,{219,606,300,32},"当前起点",pointer_.Hit({219,606,300,32}),Ink,Paper);
    font.Draw("主线路径",604,615,13,Orange);font.Draw("隐藏支路",737,615,13,Warm);
    Footer(font,"按住左键拖动 · 点击节点查看简报 · 滚轮横移 · Enter 确认 · Esc 返回    F9 开发流程");
}
