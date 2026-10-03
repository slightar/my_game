#include "game.h"
#include "world_layout.h"
#include "file_path.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <cmath>
struct WorldPreviewTestAccess {
    static void Place(Game& game,Vector2 point){game.player_->PlaceAt(point,1);}
    static bool ArtReady(Game& game){return game.worldScenery_.Load(*WorldLayout::Find(game.worldRegion_));}
    static void Open(Game& game) { game.mainMenu_.OpenWorldMap("clinic"); }
};
int main(){
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1280,720,"City traversal smoke");InitAudioDevice();
    int result=0;
    try {
        ProgressSystem fixture(Utf8Path(GetApplicationDirectory()));
        if(!fixture.ResetProgress())throw std::runtime_error("Fixture reset failed");
        Game game;
        const auto key=[](int k,bool down){PlayAutomationEvent({0,down?2U:1U,{k,0,0,0}});};
        const auto press=[&](int k){key(k,true);game.Update(1.0F/60);key(k,false);game.Update(1.0F/60);};
        const auto check=[](bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);};
        const auto output=Utf8Path(GetApplicationDirectory())/"world-qa";std::filesystem::create_directories(output);
        MainMenu map(output/"menu-fixture");map.OpenWorldMap();
        const auto mouse=[](unsigned type,int a,int b=0){PlayAutomationEvent({0,type,{a,b,0,0}});};
        const auto click=[&](int x,int y){mouse(7,x,y);mouse(6,MOUSE_BUTTON_LEFT);map.Update();mouse(5,MOUSE_BUTTON_LEFT);return map.Update();};
        click(600,227);check(map.SelectedWorldRegion()=="bridge","Node click failed");
        mouse(7,600,227);mouse(6,MOUSE_BUTTON_LEFT);map.Update();mouse(7,200,227);map.Update();
        mouse(5,MOUSE_BUTTON_LEFT);check(map.Update()==MenuAction::None&&map.SelectedWorldRegion()=="bridge","Drag activated node");
        click(840,166);check(map.SelectedWorldRegion()=="prts","System navigation failed");
        check(click(1090,594)==MenuAction::StartWorldPreview,"Room entry button failed");
        const auto capture=[&](const std::string& name){auto t=LoadRenderTexture(1280,720);BeginTextureMode(t);game.Draw(1,{});EndTextureMode();
            auto im=LoadImageFromTexture(t.texture);ImageFlipVertical(&im);check(ExportImage(im,(output/(name+".png")).string().c_str()),"Screenshot failed");UnloadImage(im);UnloadRenderTexture(t);};
        // Real player flow: existing skinned menu -> action -> room -> discovery -> menu.
        press(KEY_ENTER);press(KEY_ENTER);capture("player-stages-before");
        mouse(7,1060,583);mouse(6,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);mouse(5,MOUSE_BUTTON_LEFT);game.Update(1.0F/60);
        check(game.InWorldPreview()&&game.WorldRegion()=="clinic","Start action click did not enter clinic");
        check(game.WorldPlayerPosition().x<WorldLayout::Find("clinic")->width*.2F,"Entry skips the first half of the scene");
        const auto settle=[&](){for(int i=0;i<80;++i)game.Update(1.0F/60);};
        bool checkedBridgeLayers=false;
        const auto walkExit=[&](const std::string& target){
            WorldLayout::Port port{};for(const auto& p:WorldLayout::Ports(game.WorldRegion()))if(p.target->id==target)port=p;
            check(port.target!=nullptr,"Missing physical exit");int count=0;
            const auto moveTo=[&](float x){
                int steps=0;
                while(std::abs(game.WorldPlayerPosition().x-x)>5&&steps++<1800) {
                    int k=game.WorldPlayerPosition().x<x?KEY_D:KEY_A;
                    key(k,true);game.Update(1.0F/60);key(k,false);
                }
                check(steps<1800,"Stair approach unreachable");
            };
            if(const auto* stairs=WorldArt::Stairs(game.WorldRegion())) {
                const float width=WorldLayout::Find(game.WorldRegion())->width;
                if(port.feetY<643&&game.WorldPlayerPosition().y>400) {
                    if(!checkedBridgeLayers) {
                        moveTo(width*.245F);key(KEY_F,true);for(int i=0;i<80;++i)game.Update(1.0F/60);key(KEY_F,false);
                        check(game.WorldRegion()=="entry","Old ground door still active");
                        moveTo(port.x);key(KEY_F,true);for(int i=0;i<80;++i)game.Update(1.0F/60);key(KEY_F,false);
                        check(game.WorldRegion()=="entry","Upper door activated from street");
                        capture("entry-under-bridge");
                    }
                    moveTo(stairs->right*width+30);
                    moveTo(stairs->right*width-22);settle();
                    check(std::abs(game.WorldPlayerPosition().y+39-644)<2,"Broken stairs can be entered without jumping");
                    capture("entry-broken-stair-ground");
                    key(KEY_SPACE,true);for(int i=0;i<20;++i)game.Update(1.0F/60);key(KEY_SPACE,false);
                    settle();
                    const float stairBottom=644+(stairs->bottom-WorldArt::PaintedGround("entry"))*(644/WorldArt::Find("entry")->ground);
                    check(game.WorldPlayerPosition().y+39<stairBottom+2&&game.WorldPlayerPosition().y+39>port.feetY+40,"Jump did not land on broken stair");
                    capture("entry-broken-stair-landed");
                    moveTo((stairs->left+stairs->right)*width/2);capture("entry-stair-ascent");
                    moveTo(stairs->left*width-12);settle();
                    check(std::abs(game.WorldPlayerPosition().y+39-port.feetY)<2,"Background stairs did not reach landing");
                    if(!checkedBridgeLayers) {
                        press(KEY_SPACE);settle();
                        check(std::abs(game.WorldPlayerPosition().y+39-port.feetY)<2,"Landing jump fell through bridge");
                        press(KEY_TWO);settle();
                        check(std::abs(game.WorldPlayerPosition().y+39-port.feetY)<2,"Operator switch fell through bridge");
                        press(KEY_ONE);checkedBridgeLayers=true;
                    }
                    capture("entry-upper-landing");
                } else if(port.feetY>643&&game.WorldPlayerPosition().y<400) {
                    moveTo(stairs->landingRight*width+40);settle();
                    check(std::abs(game.WorldPlayerPosition().y+39-644)<2,"Stair descent failed");
                }
            }
            while(std::abs(game.WorldPlayerPosition().x-port.x)>12 && count++<1800){int k=game.WorldPlayerPosition().x<port.x?KEY_D:KEY_A;key(k,true);game.Update(1.0F/60);key(k,false);}
            check(count<1800,"Exit unreachable");
            if(port.feetY<643)capture("entry-station-door");
            if(port.route->kind==WorldLayout::RouteKind::Main&&!port.returning){
                key(KEY_D,true);for(int i=0;i<15;++i)game.Update(1.0F/60);key(KEY_D,false);
            } else {key(KEY_F,true);for(int i=0;i<75;++i)game.Update(1.0F/60);key(KEY_F,false);}
            settle();check(game.WorldRegion()==target,"Player traversal failed");
        };
        float wardDoorX=0;for(const auto& p:WorldLayout::Ports("clinic"))if(std::string_view(p.target->id)=="ward")wardDoorX=p.x;
        WorldPreviewTestAccess::Place(game,{wardDoorX,GameConfig::kFloorY-39});settle();capture("ward-door-closed");
        key(KEY_F,true);for(int i=0;i<100;++i)game.Update(1.0F/60);key(KEY_F,false);settle();
        check(game.WorldRegion()=="clinic"&&!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisited("ward"),"Locked ward shortcut admitted player");
        walkExit("gray");
        check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Hidden region revealed by adjacency");
        press(KEY_TAB);capture("player-stages-undiscovered");press(KEY_ENTER);
        // Proximity and a cancelled short investigation must not reveal a stage.
        float secretX=0;for(const auto& p:WorldLayout::Ports("gray"))if(std::string_view(p.target->id)=="clock")secretX=p.x;
        WorldPreviewTestAccess::Place(game,{secretX,GameConfig::kFloorY-39});settle();
        capture("secret-before-investigation");
        check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Proximity revealed secret");
        key(KEY_F,true);for(int i=0;i<30;++i)game.Update(1.0F/60);key(KEY_F,false);settle();
        check(game.WorldRegion()=="gray"&&!ProgressSystem(Utf8Path(GetApplicationDirectory())).RegionVisible("clock"),"Cancelled hold entered secret");
        walkExit("clock");press(KEY_TAB);capture("player-stages-discovered");
        ProgressSystem saved(Utf8Path(GetApplicationDirectory()));
        check(saved.RegionVisited("clock")&&saved.RegionAvailable("clock")&&!saved.RegionVisible("ward"),"Discovery save failed");
        press(KEY_ENTER);check(game.WorldRegion()=="clock"&&game.InWorldPreview(),"Discovered stage replay failed");
        capture("player-clock-room");press(KEY_TAB);
        { Game reloaded;const auto tick=[&](int k){key(k,true);reloaded.Update(1.0F/60);key(k,false);reloaded.Update(1.0F/60);};
          tick(KEY_ENTER);tick(KEY_ENTER);tick(KEY_ENTER);
          check(reloaded.InWorldPreview()&&reloaded.WorldRegion()=="clock","Restart lost selected discovery"); }
        press(KEY_ENTER);walkExit("bridge");walkExit("entry");walkExit("station");walkExit("ward");
        check(!ProgressSystem(Utf8Path(GetApplicationDirectory())).WardShortcutOpen(),"Visiting ward prematurely opened shortcut");
        walkExit("clinic");
        check(ProgressSystem(Utf8Path(GetApplicationDirectory())).WardShortcutOpen(),"Return through ward did not persist shortcut");
        WorldPreviewTestAccess::Place(game,{wardDoorX,GameConfig::kFloorY-39});settle();capture("ward-door-open");
        walkExit("ward");walkExit("clinic");press(KEY_TAB);
        WorldPreviewTestAccess::Open(game);capture("city-west");press(KEY_ENTER);
        check(game.InWorldPreview()&&game.WorldRegion()=="clinic","Prototype test entry failed");
        capture("clinic");
        // Traverse every outgoing and reversible connection using real movement and F.
        const auto enter=[&](const std::string& id){
            if(game.InWorldPreview())press(KEY_TAB);
            int start=0,target=0;
            for(int i=0;i<int(WorldLayout::Regions.size());++i){if(WorldLayout::Regions[i].id==game.WorldRegion())start=i;if(WorldLayout::Regions[i].id==id)target=i;}
            for(int i=0;i<(target-start+20)%20;++i)press(KEY_RIGHT);
            press(KEY_ENTER);check(game.WorldRegion()==id&&game.InWorldPreview(),"Map selection failed");
        };
        for(const auto& region:WorldLayout::Regions)for(const auto& port:WorldLayout::Ports(region.id)) {
            enter(region.id);
            walkExit(port.target->id);
            for(int i=0;i<5;++i)game.Update(1.0F/60);
            check(game.WorldRegion()==port.target->id,"Arrival bounced back");
        }
        for(const auto& region:WorldLayout::Regions){enter(region.id);check(WorldPreviewTestAccess::ArtReady(game),"Missing scene artwork");capture(std::string("region-")+region.id);}
        enter("gray");settle();capture("gray-street");capture("dialogue-main");
        WorldPreviewTestAccess::Place(game,{100,GameConfig::kFloorY-39});settle();capture("dialogue-return-warning");
        WorldPreviewTestAccess::Place(game,{secretX,GameConfig::kFloorY-39});settle();capture("dialogue-investigation");
        for(const auto* id:{"clinic","ward","shelter"}) {
            enter(id);WorldPreviewTestAccess::Place(game,{WorldLayout::Find(id)->width*.52F,GameConfig::kFloorY-39});
            settle();capture(std::string("scale-")+id);
            press(KEY_TWO);capture(std::string("scale-texas-")+id);
            press(KEY_SPACE);capture(std::string("scale-jump-")+id);settle();press(KEY_ONE);
        }
        enter("gray");
        check(WorldScenery::Platforms(*WorldLayout::Find("gray")).empty(),"Floating platforms remain");
        const float floor=GameConfig::kFloorY-39;
        WorldPreviewTestAccess::Place(game,{1100,floor-120});settle();
        check(std::abs(game.WorldPlayerPosition().y-floor)<1,"Continuous floor landing failed");
        press(KEY_SPACE);check(game.WorldPlayerPosition().y<floor-1,"Floor jump failed");settle();
        // Capture the actual fade, then ensure no movement/key input leaks across it.
        float end=WorldLayout::Find("gray")->width-108;
        WorldPreviewTestAccess::Place(game,{end,floor});game.Update(1.0F/60);
        for(int i=0;i<12;++i)game.Update(1.0F/60);capture("transition-fade");
        check(game.WorldRegion()=="gray","Transition skipped outgoing fade");
        press(KEY_TAB);check(game.InWorldPreview(),"Menu interrupted transition");
        settle();check(game.WorldRegion()=="entry","Automatic main transition failed");
        enter("ice");capture("ice-street");enter("station");capture("station");enter("pipes");capture("pipes");
        enter("core");press(KEY_TAB);capture("city-east");press(KEY_ENTER);capture("core");
        enter("prts");press(KEY_TAB);capture("city-system");press(KEY_ENTER);capture("system-room");
        enter("throne");capture("throne");press(KEY_ESCAPE);check(!game.InWorldPreview(),"Map return failed");
        std::cout<<"Mouse selection/drag, all 45 directed exits, automatic main travel, held investigations, cancellation, fades and return passed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    CloseAudioDevice();CloseWindow();return result;
}
