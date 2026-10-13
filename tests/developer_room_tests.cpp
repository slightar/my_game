#include "developer_room.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
void Check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try {
    DeveloperRoom room;room.Enter(2600);const Vector2 player{1100,605};
    Check(room.CatalogSize()==35&&room.VisibleCatalog().size()==35,"Catalog incomplete");
    room.SetFilter(DeveloperFilter::Mobs);Check(room.VisibleCatalog().size()==31,"Mob filter");
    room.SetFilter(DeveloperFilter::Bosses);Check(room.VisibleCatalog().size()==4&&room.SelectedBoss(),"Boss filter");
    room.SetFilter(DeveloperFilter::All);room.AdjustCount(100);Check(room.Count()==20,"Count ceiling");
    room.AdjustCount(-100);Check(room.Count()==1,"Count floor");
    room.Select(32);Check(room.ListStart()==30&&room.Count()==1,"Boss pagination/count");
    room.Select(0);room.SetSpawnMode(1);Check(room.SpawnX(player)==760,"Left spawn");
    room.SetSpawnMode(0);Check(room.SpawnX(player)==1440,"Right spawn");
    room.Mark(-100);Check(room.SpawnX(player)==115,"Marker bounds");
    room.Mark(1600);
    for(int i=0;i<31;++i){room.Select(i);Check(room.SpawnSelected(player),"Mob spawn failed");}
    Check(room.Mobs().Remaining()==31&&room.Mobs().Pending()==0,"Unexpected queued enemies");
    Check(room.Mobs().Units()[int(EnemyKind::StealthCrossbow)].cloaked,"Manual spawn lost initial stealth");
    room.Select(0);const float originalHealth=room.Mobs().Units().front().maxHealth;
    room.CycleDifficulty();room.SpawnSelected(player);
    Check(room.Mobs().Units().back().maxHealth>originalHealth,"Manual profile ignored");
    Check(room.Mobs().Units().front().difficulty.movement==1,"Existing difficulty changed");
    room.Select(31);room.SpawnSelected(player);
    Check(room.Boss().Kind()==StoryBossKind::Crownslayer&&room.Boss().Targetable(),"Crown sandbox inactive");
    room.Select(32);room.SpawnSelected(player);
    Check(room.Boss().Kind()==StoryBossKind::W&&room.Mobs().Remaining()==32,"Boss replacement lost mobs");
    room.Select(33);room.SpawnSelected(player);
    Check(room.Boss().Kind()==StoryBossKind::FrostNova&&room.Boss().TwoLives(),"Frost sandbox missing");
    room.Select(34);room.SpawnSelected(player);
    Check(room.Boss().Kind()==StoryBossKind::Patriot&&room.Boss().TwoLives()&&room.Mobs().Remaining()==32,"Patriot sandbox missing");
    room.ToggleFrozen();const auto before=room.Boss().Feet();const float time=room.Seconds();
    Check(!room.UpdateEnemies(1,player)&&room.Seconds()==time&&room.Boss().Feet().x==before.x,"Freeze leaks simulation");
    room.RequestStep();Check(room.UpdateEnemies(1,player),"Step ignored");
    Check(std::abs(room.Seconds()-time-1.0F/60)<.00001F,"Step duration wrong");
    Check(!room.UpdateEnemies(1,player),"Step keeps running");
    room.ToggleFrozen();room.UpdateEnemies(.1F,player);Check(room.Seconds()>time+.1F,"Resume failed");
    room.Clear();Check(room.Mobs().Remaining()==0&&!room.Boss().Present()&&room.Mobs().Bolts().empty(),"Clear retained enemies/hazards");
    room.Select(0);room.AdjustCount(100);
    for(int i=0;i<3;++i)Check(room.SpawnSelected(player),"Cap rejected valid count");
    Check(!room.SpawnSelected(player)&&room.Mobs().Remaining()==60,"Cap partially spawned");
    room.AdjustCount(-16);Check(room.SpawnSelected(player)&&room.Mobs().Remaining()==64,"Cap off by one");
    room.AdjustCount(-3);Check(!room.SpawnSelected(player),"Cap overflow");
    for(const auto& u:room.Mobs().Units())room.Mobs().Damage(u.id,10000,DamageType::Arts,player);
    for(int i=0;i<60;++i)room.UpdateEnemies(1.0F/60,player);
    Check(room.Mobs().Units().empty(),"Dead bodies never pruned");
    Check(room.God(),"Default should allow safe testing");room.ToggleGod();Check(!room.God(),"God toggle");
    room.CycleSpeed();Check(room.Speed()==2,"Speed 2x");room.CycleSpeed();Check(room.Speed()==.5F,"Speed .5x");
    room.Enter(2600);Check(room.Speed()==1&&room.God()&&!room.Frozen(),"New room retained old settings");
    std::cout<<"Developer room catalog, spawning, cap, profiles, pause, step and reset passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
