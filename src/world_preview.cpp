#include "game.h"
#include "world_layout.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr Color Cyan{85,205,209,255}, Paper{232,240,243,255}, Muted{155,174,187,255};
int NearestPort(const std::vector<WorldLayout::Port>& ports,Vector2 p) {
    int best=-1;float distance=48;
    for(int i=0;i<int(ports.size());++i)if(std::abs(p.y+39-ports[i].feetY)<12&&std::abs(ports[i].x-p.x)<distance){best=i;distance=std::abs(ports[i].x-p.x);}
    return best;
}
const WorldLayout::Port* Forward(const std::vector<WorldLayout::Port>& ports) {
    for(const auto& p:ports)if(p.route->kind==WorldLayout::RouteKind::Main&&!p.returning)return &p;
    return nullptr;
}
float Ease(float t){t=std::clamp(t,0.0F,1.0F);return t*t*(3-2*t);}
// Advance by code point, never through the middle of a Chinese UTF-8 character.
std::string Revealed(const std::string& text,int count) {
    size_t end=0;
    while(end<text.size()&&count-->0) {
        ++end;
        while(end<text.size()&&(static_cast<unsigned char>(text[end])&0xC0)==0x80)++end;
    }
    return text.substr(0,end);
}
void DrawNarration(const UiFont& font,const std::string& text,float time,bool caution,bool upper) {
    const float opacity=Ease(time/.22F), y=(upper?420:104)+(1-opacity)*8;
    const Color ink{32,42,49,255}, accent=caution?Color{191,134,57,255}:Cyan;
    const auto tint=[&](Color c){return Fade(c,opacity);};
    DrawRectangleRec({32,y+5,108,108},tint(Color{0,0,0,95}));
    DrawRectangleRec({28,y,108,108},tint(Color{15,22,29,245}));
    font.Skin().Draw("prts_avatar",{31,y+3,102,102},tint(WHITE));
    DrawRectangleLinesEx({28,y,108,108},1,tint(Color{142,161,171,255}));
    DrawRectangleRec({28,y+105,108,3},tint(accent));
    DrawRectangleRec({158,y+5,540,108},tint(Color{0,0,0,95}));
    DrawRectangleRec({154,y,540,108},tint(Color{236,241,242,248}));
    DrawTriangle({154,y+38},{140,y+48},{154,y+58},tint(Color{236,241,242,248}));
    DrawRectangleRec({154,y,540,29},tint(Color{30,40,48,255}));
    font.Draw("PRTS",170,y+5,19,tint(WHITE));
    font.Draw("RHODES ISLAND / NAVIGATION",242,y+9,11,tint(Color{159,178,186,255}));
    DrawCircle(679,int(y+14),3,tint(accent));
    const auto visible=Revealed(text,int(std::max(0.0F,time-.15F)*30));
    float x=174,row=y+44;
    // Wrap at complete glyph boundaries; leave a stable two-line bubble.
    for(size_t i=0;i<visible.size();) {
        size_t end=i+1;
        while(end<visible.size()&&(static_cast<unsigned char>(visible[end])&0xC0)==0x80)++end;
        const auto glyph=visible.substr(i,end-i);const float w=font.Measure(glyph.c_str(),20);
        if(x+w>672){x=174;row+=27;}
        font.Draw(glyph.c_str(),x,row,20,tint(ink));x+=w;i=end;
    }
    if(visible.size()<text.size())DrawRectangleRec({x+3,row+3,2,17},tint(accent));
}
}

void Game::EnterWorldRegion(const std::string& id,const std::string& from) {
    const auto* region=WorldLayout::Find(id);if(!region)return;
    if(!worldDesignPreview_ && !mainMenu_.RecordRegionVisit(id,from))return;
    worldScenery_.Load(*region,worldDesignPreview_||mainMenu_.WardShortcutOpen());worldTime_=0;worldNarrationTime_=0;
    worldInvestigate_=0;worldInvestigatePort_=-1;worldDeviation_=0;
    worldRegion_=id;worldPreview_=true;paused_=false;bullets_.clear();
    float x=300,feetY=GameConfig::kFloorY;int facing=1;
    for(const auto& port:WorldLayout::Ports(id))if(port.target->id==from) {
        // Enter at the beginning of the route, retaining the full panorama.
        if(port.route->kind==WorldLayout::RouteKind::Main) {
            if(!port.returning){x=region->width-280;facing=-1;}
        } else {facing=port.x>region->width*.65F?-1:1;x=port.x+facing*110;feetY=port.feetY;}
        break;
    }
    x=std::clamp(x,80.0F,region->width-80);
    for(auto& op:operators_) {
        op.SetHorizontalBounds(80,region->width-80);
        op.PlaceAt({x,feetY-39},facing);
    }
    worldTraversal_.Enter(id);
    cameraX_=std::clamp(x,640.0F,region->width-640);
    if(from.empty()){worldTransition_=-1;worldDestination_.clear();}
}

void Game::UpdateWorldPreview(float dt) {
    dt=std::clamp(dt,0.0F,.05F);
    if(worldTransition_>=0) {
        worldTransition_+=dt;
        if(worldTransition_>=.36F&&!worldTransitionLoaded_) {
            const std::string from=worldRegion_;
            EnterWorldRegion(worldDestination_,from);
            worldTransitionLoaded_=true;
        }
        if(worldTransition_>=1.1F){worldTransition_=-1;worldDestination_.clear();}
        return;
    }
    if(IsKeyPressed(KEY_TAB)||IsKeyPressed(KEY_M)||IsKeyPressed(KEY_ESCAPE)) {
        worldPreview_=false;inBattle_=false;
        if(worldDesignPreview_)mainMenu_.OpenWorldMap(worldRegion_);else mainMenu_.OpenStageSelect();
        return;
    }
    const auto* region=WorldLayout::Find(worldRegion_);if(!region)return;
    if(mainMenu_.Settings().Pressed(GameAction::OperatorOne))SwitchOperator(0);
    if(mainMenu_.Settings().Pressed(GameAction::OperatorTwo))SwitchOperator(1);
    worldTime_+=dt;
    player_->Update(dt,{player_->Position().x+500,player_->Position().y},0,
                    bullets_,audio_,mainMenu_.Settings(),false,true,worldTraversal_.Platforms(),worldTraversal_.Ramp());
    const float x=player_->Position().x;
    const float desired=std::clamp(x,640.0F,region->width-640);
    cameraX_+=(desired-cameraX_)*(1-std::exp(-7*dt));
    const auto ports=WorldLayout::Ports(worldRegion_);
    const auto* forward=Forward(ports);
    const int nearby=NearestPort(ports,player_->Position());
    const int previousDeviation=worldDeviation_;
    if(forward) {
        worldDeviation_=x<230?1:0;
        if(nearby>=0&&ports[nearby].route->kind!=WorldLayout::RouteKind::Main)
            worldDeviation_=2;
    }
    worldNarrationTime_=previousDeviation==worldDeviation_?worldNarrationTime_+dt:0;
    const auto start=[&](const WorldLayout::Port& p){
        worldDestination_=p.target->id;worldTransition_=0;worldTransitionLoaded_=false;
        worldInvestigate_=0;worldInvestigatePort_=-1;
    };
    // Main path continues naturally. All side doors require an intentional hold;
    // mere proximity never changes discovery, scene geometry, or stage visibility.
    if(forward&&worldTime_>.65F&&x>=forward->x&&player_->Position().y>=GameConfig::kFloorY-80){start(*forward);return;}
    if(nearby<0||nearby!=worldInvestigatePort_||!IsKeyDown(KEY_F))worldInvestigate_=0;
    worldInvestigatePort_=nearby;
    if(nearby>=0&&IsKeyDown(KEY_F)) {
        const auto& p=ports[nearby];
        if(!worldDesignPreview_&&worldRegion_=="clinic"&&std::string_view(p.target->id)=="ward"&&!mainMenu_.WardShortcutOpen()) {
            worldInvestigate_=0;return;
        }
        if(p.route->kind==WorldLayout::RouteKind::Main) {
            if(IsKeyPressed(KEY_F))start(p);
        } else {
            worldInvestigate_+=dt;
            if(worldInvestigate_>=1.15F)start(p);
        }
    }
}

void Game::DrawWorldPreview(float scale,Vector2 offset) const {
    const auto* region=WorldLayout::Find(worldRegion_);if(!region)return;
    const Camera2D ui{offset,{},0,scale};
    const Camera2D camera{{offset.x+640*scale,offset.y+360*scale},{cameraX_,360},0,scale};
    BeginMode2D(ui);worldScenery_.Background(*region,cameraX_,worldTime_);EndMode2D();
    BeginMode2D(camera);player_->Draw(characterArt_);EndMode2D();
    BeginMode2D(ui);
    const auto ports=WorldLayout::Ports(worldRegion_);
    const auto* forward=Forward(ports);
    const int nearby=NearestPort(ports,player_->Position());
    DrawRectangleGradientV(0,0,1280,115,Color{10,16,22,235},Color{10,16,22,0});
    DrawRectangle(28,26,3,48,Cyan);
    uiFont_.Draw(region->name,43,24,27,Paper);
    uiFont_.Draw(WorldLayout::Layers[region->layer],44,60,14,Cyan);
    uiFont_.Draw("CHERNOBOG / EXPLORATION",923,30,14,Muted);
    // Tutorial-style communication. Secret destination names remain undisclosed.
    std::string line;
    if(forward) {
        line="请沿街道向右前进。前往 "+std::string(forward->target->name)+"。";
        if(worldDeviation_==1)line="路线已确认，无需回头。";
        if(worldDeviation_==2)line="那扇旧门不在规划路线内。请继续向右。";
        uiFont_.Draw((std::string(forward->target->name)+"  >").c_str(),960,62,17,Cyan);
    } else line=region->kind==WorldLayout::Kind::Ending?"任务路线已完成。":"此处无可用导航。由你决定去向。";
    const bool upper=WorldArt::Stairs(worldRegion_)&&player_->Position().y<380;
    DrawNarration(uiFont_,line,worldNarrationTime_,worldDeviation_!=0,upper);
    if(nearby>=0&&worldTransition_<0) {
        const auto& p=ports[nearby];
        const bool unknown=!worldDesignPreview_&&WorldLayout::Hidden(p.target->id)&&!mainMenu_.RegionVisited(p.target->id);
        std::string prompt;
        if(p.route->kind==WorldLayout::RouteKind::Main)prompt=p.returning?"F 原路返回":"向右继续移动";
        else if(!worldDesignPreview_&&worldRegion_=="clinic"&&std::string_view(p.target->id)=="ward"&&!mainMenu_.WardShortcutOpen())
            prompt="门从另一侧锁住了";
        else prompt=unknown?std::string(WorldArt::Find(region->id)->clue)+" · 长按 F 调查":std::string(p.target->name)+" · 长按 F 通行";
        const float w=uiFont_.Measure(prompt.c_str(),18);
        const float promptY=upper?548:238;
        DrawRectangleRec({640-w/2-18,promptY,w+36,52},Fade(BLACK,.7F));
        uiFont_.Draw(prompt.c_str(),640-w/2,promptY+14,18,Paper);
        if(worldInvestigate_>0)DrawRectangleRec({640-w/2,promptY+42,w*std::clamp(worldInvestigate_/1.15F,0.0F,1.0F),2},Cyan);
    }
    DrawRectangleGradientV(0,675,1280,45,BLANK,Fade(BLACK,.9F));
    uiFont_.Draw("左右移动 / 跳跃沿用当前按键 · F 调查 · Tab / M / Esc 返回关卡选择",35,692,17,Muted);
    if(worldTransition_>=0) {
        const float alpha=worldTransition_<.36F?Ease(worldTransition_/.36F):1-Ease((worldTransition_-.62F)/.48F);
        DrawRectangle(0,0,1280,720,Fade(Color{7,12,17,255},alpha));
        if(worldTransitionLoaded_) {
            const float w=uiFont_.Measure(region->name,28);
            uiFont_.Draw(region->name,640-w/2,330,28,Fade(Paper,alpha));
            uiFont_.Draw("CHERNOBOG / PRTS",556,379,15,Fade(Cyan,alpha));
        }
    }
    EndMode2D();
}
