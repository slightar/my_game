#include "ui_input.h"
#include "action_map.h"
#include "story_data.h"
#include <stdexcept>
#include <set>
#include <iostream>

void Require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){try {
    UiPointerState state;const Rectangle button{100,100,120,60};
    auto p=state.Sample({130,120},true,1280,720);
    Require(p.Held(button)&&!p.Clicked(button),"Press must depress without activating");
    for(int i=0;i<240;++i) {
        p=state.Sample({130,120},true,1280,720);
        Require(p.down&&!p.released&&!p.Clicked(button),"Hold repeats click");
    }
    p=state.Sample({130,120},false,1280,720);Require(p.Clicked(button),"Release did not activate");
    Require(!state.Sample({130,120},false,1280,720).Clicked(button),"Release was delivered twice");
    state.Sample({10,10},true,1280,720);
    Require(!state.Sample({130,120},false,1280,720).Clicked(button),"Drag-in activated a control");
    state.Sample({130,120},true,1280,720);
    Require(!state.Sample({10,10},false,1280,720).Clicked(button),"Drag-out activated a control");
    // Wide-window letterboxing preserves the same logical hit target.
    state.Sample({770,240},true,2560,1440);
    Require(!state.Sample({770,240},false,2560,1440).Clicked(button),"Scaling hit a different control");
    state.Sample({260,240},true,2560,1440);
    Require(state.Sample({260,240},false,2560,1440).Clicked(button),"Scaled release missed control");
    ActionMap::DragGesture drag;float scroll=100;
    p=state.Sample({500,300},true,1280,720);
    Require(!drag.Update(p,ActionMap::View,1000,scroll),"Mouse down immediately became a drag");
    p=state.Sample({350,300},true,1280,720);
    Require(drag.Update(p,ActionMap::View,1000,scroll)&&scroll==250,"Drag did not track mouse delta");
    p=state.Sample({350,300},false,1280,720);
    Require(drag.Update(p,ActionMap::View,1000,scroll),"Drag release was not consumed");
    Require(!drag.tracking,"Drag capture survived release");
    drag.Update(state.Sample({500,300},true,1280,720),ActionMap::View,1000,scroll);
    Require(drag.Update(state.Sample({1500,300},true,1280,720),ActionMap::View,1000,scroll)&&scroll==0,"Drag past edge not clamped");
    drag.Update(state.Sample({1500,300},false,1280,720),ActionMap::View,1000,scroll);
    drag.Update(state.Sample({500,300},true,1280,720),ActionMap::View,1000,scroll);
    Require(!drag.Update(state.Sample({503,300},false,1280,720),ActionMap::View,1000,scroll),"Small click jitter prevented selection");
    std::set<std::string> ids;
    for(const auto& p:ActionMap::points) {
        Require(StoryData::FindNode(std::string(p.id))!=nullptr,"Map references unknown node");
        Require(ids.insert(std::string(p.id)).second,"Duplicate map node");
    }
    Require(ids.size()==StoryData::Nodes().size(),"Map omits story node");
    for(const auto& chapter:StoryData::Chapters()) {
        float x=-1;
        for(const auto& id:chapter.nodeIds) {
            const auto* p=ActionMap::Find(id);
            if(p->y==370){Require(p->x>x,"Mainline not left-to-right");x=p->x;}
            if(StoryData::FindNode(id)->type==NodeType::Hidden)Require(p->y!=370,"Hidden node lies on mainline");
        }
    }
    std::cout<<"Pointer capture, single release, scaling and map layout passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
