#pragma once
#include "audio_system.h"
#include "game_settings.h"
#include "player.h"
#include <cmath>
#include <stdexcept>

template<class Render>
void CheckColdCombat(const CharacterArt& art,const UiFont& font,Render render) {
    AudioSystem audio;
    GameSettings settings(std::filesystem::path(GetApplicationDirectory())/"cold-fixture");
    const auto key=[](int code,bool down){PlayAutomationEvent({0,down?2U:1U,{code,0,0,0}});};
    const auto check=[](bool ok,const char* message){if(!ok)throw std::runtime_error(message);};
    std::vector<Bullet> bullets;
    Player normal,cold;normal.Reset();cold.Reset();
    normal.PlaceAt({300,605},1);cold.PlaceAt({300,605},1);cold.ApplyCold();
    key(KEY_D,true);key(KEY_J,true);
    int normalShots=0,coldShots=0;
    for(int frame=0;frame<120;++frame) {
        normal.Update(1.0F/60,{2300,605},20,bullets,audio,settings);normalShots+=normal.Actions().ranged;
        cold.Update(1.0F/60,{2300,605},20,bullets,audio,settings);coldShots+=cold.Actions().ranged;
    }
    key(KEY_D,false);key(KEY_J,false);
    check(std::abs((cold.Position().x-300)/(normal.Position().x-300)-.7F)<.01F,"Cold did not slow movement");
    check(coldShots<normalShots*.8F,"Cold did not slow basic attacks");
    check(cold.IsCold()&&!cold.IsFrozen(),"A single cold stack incorrectly froze operator");
    render("cold-status.png",[&]{cold.Draw(art);cold.DrawHud(font,art,"能天使",&settings);});
    cold.ApplyCold();const auto x=cold.Position().x;
    check(cold.IsFrozen(),"Second cold stack did not freeze");
    const int freezeKeys[]{settings.Key(GameAction::MoveRight),settings.Key(GameAction::Attack),
        settings.Key(GameAction::Jump),settings.Key(GameAction::Dodge),settings.Key(GameAction::SkillOne),settings.Key(GameAction::SkillTwo)};
    for(int code:freezeKeys)key(code,true);
    bullets.clear();
    for(int frame=0;frame<60;++frame)cold.Update(1.0F/60,{2300,605},20,bullets,audio,settings);
    for(int code:freezeKeys)key(code,false);
    check(std::abs(cold.Position().x-x)<.01F&&bullets.empty(),"Frozen operator could move or attack");
    check(cold.Actions().skills==0&&cold.Actions().dodges==0&&cold.Actions().jumps==0,"Frozen operator could use actions");
    render("frozen-status.png",[&]{cold.Draw(art);cold.DrawHud(font,art,"能天使",&settings);});
    for(int frame=0;frame<65;++frame)cold.Update(1.0F/60,{2300,605},20,bullets,audio,settings);
    check(!cold.IsFrozen()&&cold.IsCold(),"Freeze failed to expire before cold");
    check(cold.TakeDamage({x+10,605},false,2)&&cold.Health()==1,"Frozen-target bonus damage ignored");
    cold.Reset();check(!cold.IsCold()&&!cold.IsFrozen(),"New action retained cold");
}
