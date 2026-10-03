#include "world_layout.h"
#include <set>
#include <stdexcept>
#include <iostream>
using namespace WorldLayout;
int main() {
    const auto check=[](bool ok){if(!ok)throw std::runtime_error("Invalid city topology");};
    std::set<std::string> ids, reached{"clinic"};
    std::set<std::string> scenes;
    for(const auto& scene:WorldArt::Scenes)check(scenes.insert(scene.id).second&&Find(scene.id));
    check(scenes.size()==Regions.size());
    for(const auto& r:Regions) {
        check(WorldArt::Find(r.id)!=nullptr);
        check(r.width>2500 && r.width<3200);
        check(ids.insert(r.id).second && r.layer>=0 && r.layer<6);
        float previous=-100;
        for(const auto& p:Ports(r.id)) {check(p.target && p.x>80 && p.x<r.width-80 && p.x-previous>140);previous=p.x;}
    }
    for(int i=0;i<20;++i)for(const auto& r:Regions)if(reached.contains(r.id))
        for(const auto& p:Ports(r.id))reached.insert(p.target->id);
    check(reached.size()==Regions.size());
    for(const auto& route:Routes) {
        check(Find(route.from)&&Find(route.to));
        bool reverse=false;
        for(const auto& port:Ports(route.to))if(std::string_view(port.target->id)==route.from)reverse=true;
        check(reverse==route.returnable);
        if(Find(route.to)->layer==5)check(route.kind==RouteKind::System && !route.returnable);
    }
    check(Ports("throne").empty());
    const char* mainPath[]{"clinic","gray","entry","bridge","wtower","comm","ice","pipes","industry","burn","core","normal"};
    for(int i=0;i<11;++i){bool found=false;for(const auto& r:Routes)
        if(std::string_view(r.from)==mainPath[i]&&std::string_view(r.to)==mainPath[i+1]&&r.kind==RouteKind::Main)found=true;check(found);}
    std::cout<<"City reachability, main route, safe exits and one-way system routes passed\n";
}
