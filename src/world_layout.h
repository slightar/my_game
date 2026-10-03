#pragma once
#include "world_art.h"
#include <array>
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

// The user's city cross-section. Scene content and progression gates are deliberately
// separate: every room is inspectable while this is a traversal prototype.
namespace WorldLayout {
enum class Kind { Main, Boss, Exploration, Ending, Memory };
enum class RouteKind { Main, Exploration, System };
struct Region {
    const char* id;
    const char* name;
    const char* subtitle;
    int layer; // 0..4 = +2,+1,0,-1,-2; 5 is non-physical system space.
    float mapX;
    float width;
    Kind kind;
    const char* note;
};
inline constexpr std::array<const char*,6> Layers{
    "高架层 +2","建筑上层 +1","地表 0","地下 -1","设施深层 -2","系统空间"};
inline constexpr std::array Regions{
    Region{"clinic","地下诊疗所","起点 / 重生",3,220,WorldArt::Width("clinic"),Kind::Main,"城区探索的起点与重生位置。序章异常接入结束后回到这里。"},
    Region{"gray","灰雪街区","PRTS 接管",2,220,WorldArt::Width("gray"),Kind::Main,"地表主路向东接入高架入口；向上可探索停摆钟楼。"},
    Region{"clock","停摆钟楼","残响 / 隐藏",0,220,WorldArt::Width("clock"),Kind::Exploration,"从灰雪街区登楼，探索后由上层通道汇入高架桥。"},
    Region{"entry","高架入口","楼梯上行",2,510,WorldArt::Width("entry"),Kind::Main,"主线在这里向上进入高架桥，地下侧路连接旧车站。"},
    Region{"station","旧车站","地下通道",3,510,WorldArt::Width("station"),Kind::Main,"连接高架入口和维护井，向下的旧电梯通往零号病房。"},
    Region{"ward","零号病房","旧电梯回访",4,510,WorldArt::Width("ward"),Kind::Exploration,"从旧车站下行调查，并可通过回访通道返回地下诊疗所。"},
    Region{"bridge","高架桥","01 / 弑君者",0,600,WorldArt::Width("bridge"),Kind::Boss,"主线第一处 Boss 区域；战斗暂不实现。东侧继续前往通讯塔上层。"},
    Region{"wtower","通讯塔上层","02 / W",0,900,WorldArt::Width("wtower"),Kind::Boss,"主线第二处 Boss 区域；战斗暂不实现。由升降机下降至废弃通讯站。"},
    Region{"comm","废弃通讯站","升降机出口",2,900,WorldArt::Width("comm"),Kind::Main,"地表向东通往冰封居住区，向下可进入 W 爆破后留下的维护井。"},
    Region{"well","维护井","W 的爆破缺口",3,900,WorldArt::Width("well"),Kind::Exploration,"连接旧车站、废弃通讯站与热能管线。爆破开路条件后续接入。"},
    Region{"ice","冰封居住区","03 / 霜星",2,1220,WorldArt::Width("ice"),Kind::Boss,"主线第三处 Boss 区域；战斗暂不实现。调查路线向下进入热能管线。"},
    Region{"pipes","热能管线","向下调查",3,1220,WorldArt::Width("pipes"),Kind::Main,"地下主线继续向东前往工业区关口，支路向下进入冰下避难所。"},
    Region{"shelter","冰下避难所","藏品 / 隐藏",4,1220,WorldArt::Width("shelter"),Kind::Exploration,"探索深层设施后，可沿地下通路抵达 PRTS 数据根系。"},
    Region{"industry","工业区关口","04 / 爱国者",3,1520,WorldArt::Width("industry"),Kind::Boss,"主线第四处 Boss 区域；战斗暂不实现。离开关口后上行至燃烧街区。"},
    Region{"burn","燃烧街区","地表主线",2,1830,WorldArt::Width("burn"),Kind::Main,"从地下返回地表，继续向上进入城市核心。"},
    Region{"core","城市核心","05 / 塔露拉",1,1830,WorldArt::Width("core"),Kind::Boss,"主线第五处 Boss 区域。普通路线通向任务结算，烧开的竖井通向数据根系。"},
    Region{"normal","任务结算","普通结局",1,2130,WorldArt::Width("normal"),Kind::Ending,"普通路线终点。当前只提供空白区域，结局演出与奖励尚未接入。"},
    Region{"root","PRTS 数据根系","塔露拉烧开的竖井",4,1830,WorldArt::Width("root"),Kind::Exploration,"城市最深处的物理设施；从这里异常接入 PRTS 核心，而非继续走到一间物理房间。"},
    Region{"prts","PRTS 核心","协议失效",5,1520,WorldArt::Width("prts"),Kind::Boss,"独立系统空间。协议失效与核心战斗暂不实现，后续出口通往魔王王座。"},
    Region{"throne","魔王王座","特蕾西娅",5,1830,WorldArt::Width("throne"),Kind::Memory,"终章系统空间终点。序章只发生异常接入，王座不属于切城物理建筑。"}
};
struct Route {
    const char* from;
    const char* to;
    RouteKind kind;
    const char* passage;
    bool returnable;
};
inline constexpr std::array Routes{
    Route{"clinic","gray",RouteKind::Main,"上行出口",true},
    Route{"gray","entry",RouteKind::Main,"街区主道",true},
    Route{"entry","bridge",RouteKind::Main,"上行楼梯",true},
    Route{"bridge","wtower",RouteKind::Main,"高架通道",true},
    Route{"wtower","comm",RouteKind::Main,"下降升降机",true},
    Route{"comm","ice",RouteKind::Main,"地表通道",true},
    Route{"ice","pipes",RouteKind::Main,"下行通道",true},
    Route{"pipes","industry",RouteKind::Main,"地下主道",true},
    Route{"industry","burn",RouteKind::Main,"上行出口",true},
    Route{"burn","core",RouteKind::Main,"核心上行",true},
    Route{"core","normal",RouteKind::Main,"任务结算出口",true},
    Route{"gray","clock",RouteKind::Exploration,"钟楼楼梯",true},
    Route{"clock","bridge",RouteKind::Exploration,"高架汇入口",true},
    Route{"station","entry",RouteKind::Exploration,"地下出入口",true},
    Route{"station","ward",RouteKind::Exploration,"旧电梯",true},
    Route{"ward","clinic",RouteKind::Exploration,"诊疗所回访通道",true},
    Route{"station","well",RouteKind::Exploration,"旧地下通道",true},
    Route{"comm","well",RouteKind::Exploration,"爆破缺口",true},
    Route{"well","pipes",RouteKind::Exploration,"维护通道",true},
    Route{"pipes","shelter",RouteKind::Exploration,"避难所下行",true},
    Route{"shelter","root",RouteKind::Exploration,"深层通道",true},
    Route{"core","root",RouteKind::System,"烧开的竖井",false},
    Route{"root","prts",RouteKind::System,"系统接入",false},
    Route{"prts","throne",RouteKind::System,"王座接入",false}
};
inline const Region* Find(std::string_view id) {
    for(const auto& r:Regions) if(r.id==id)return &r;
    return nullptr;
}
inline bool Hidden(std::string_view id) {
    return id=="clock" || id=="station" || id=="ward" || id=="well" || id=="shelter" || id=="root" || id=="prts" || id=="throne";
}
inline constexpr std::array Prologue{"地下诊疗所","魔王王座","茧笼断线","地下诊疗所重生"};
struct Port { const Route* route; const Region* target; float x; bool returning; float feetY=644; };
inline std::vector<Port> Ports(std::string_view id) {
    std::vector<Port> result;
    const auto* room=Find(id);if(!room)return result;
    for(const auto& route:Routes)
        if(route.to==id && route.returnable)result.push_back({&route,Find(route.from),0,true});
    for(const auto& route:Routes)
        if(route.from==id)result.push_back({&route,Find(route.to),0,false});
    // Side passages match permanent recesses painted into the panorama.
    int recess=0;
    const auto* art=WorldArt::Find(id);if(!art)return {};
    for(auto& port:result) {
        if(port.route->kind==RouteKind::Main)
            port.x=port.returning?110:room->width-110;
        else port.x=room->width*art->passages[recess++];
        if(const auto* stairs=WorldArt::Stairs(id);stairs&&std::string_view(port.target->id)==stairs->destination)port.feetY=WorldArt::UpperFloor(id);
    }
    std::sort(result.begin(),result.end(),[](const Port& a,const Port& b){return a.x<b.x;});
    return result;
}
inline std::string Glyphs() {
    const std::string narration="请沿街道向右前进前往路线已确认无需回头那扇旧门不在规划路线内请继续向右此处无可用导航由你决定去向任务路线已完成门后传来气流长按推开松开取消正在推门原路返回通行向右继续移动区域连接中信号中断发现记录失败请重试";
    std::string result="通行检修平台切城行动已发现支路尚未抵达已探索可进入沿已开放的主线出口抵达此区域沿主路前进靠近通道进行探索关卡选择区域探索内部场景待建设调查通道开始行动当前区域级原路返回场景内容预留战斗未实现靠近出口双向回访箭头标记流程方向普通通道当前自由信息按键切尔诺伯格城区横截面区域规划全部区域开放预览内部待建设进入空白区域返回城区地图相邻出口已抵达终点非物理空间楼层连通方向序章异常接入茧笼断线地表主线探索支路系统接入滚轮横移拖动地图按住移动跳跃附近按切换区域主线出口返回出口上行下行水平通道区域长度米未实现存档保持区域导航定位诊疗所终章空间预览连接线路单向出口编号";
    for(const auto& r:Regions)result+=std::string(r.name)+r.subtitle+r.note;
    for(const auto& l:Layers)result+=l;
    for(const auto& r:Routes)result+=r.passage;
    for(const auto& scene:WorldArt::Scenes)result+=scene.clue;
    return result+narration+"门从另一侧锁住了天桥楼梯上楼下楼到达上层沿天桥探索";
}
}
