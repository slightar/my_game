#pragma once
#include "enemy_renderer.h"
#include "terminal_ui.h"
#include "ui_font.h"
#include <stdexcept>

template<class Capture>
void CheckEnemyRendering(const UiFont& font,Capture capture){
    EnemyRenderer renderer;
    for(const auto& d:EnemyDefinitions())if(!renderer.HasSprite(d.kind))throw std::runtime_error("Missing enemy sprite sheet");
    capture("enemy-directions.png",[&]{
        ClearBackground({188,198,203,255});
        for(int r=0;r<4;++r)for(int c=0;c<6;++c){
            EnemyUnit u;u.kind=r<2?EnemyKind::Shield:EnemyKind::Drone;
            u.health=EnemyData(u.kind).health;u.feet={110+c*210.0F,170+r*175.0F};
            u.facing=r%2==0?1:-1;u.moving=true;u.animationTime=c*.2F;
            renderer.DrawUnit(u,font);
            font.Draw(u.facing>0?"RIGHT ->":"<- LEFT",u.feet.x-40,u.feet.y+12,16,TerminalUi::Ink);
        }
    });
    capture("enemy-roster.png",[&]{
        ClearBackground({188,198,203,255});
        DrawRectangle(0,0,1280,86,TerminalUi::Ink);
        font.Draw("敌方单位 / 战斗设计",42,24,34,TerminalUi::Paper);
        for(unsigned i=0;i<6;++i){
            const auto& d=EnemyDefinitions()[i];const float x=108+i*212.0F;
            DrawRectangle(int(x-99),111,198,535,Color{221,227,226,255});
            EnemyUnit unit;unit.id=i+1;unit.kind=d.kind;unit.health=d.health;unit.feet={x,366};unit.facing=-1;unit.animationTime=.25F;
            renderer.DrawUnit(unit,font);
            font.Draw(TextFormat("HP %.0f / SPEED %.0f",d.health,d.speed),x-86,405,14,TerminalUi::Ink);
            TerminalUi::Wrap(font,d.tactic,{x-86,449,172,161},17,TerminalUi::Ink);
        }
        font.Draw("主页 N / 小怪试炼 · 客户端模型离线烘焙 · 战斗数值为本项目设计",43,675,20,TerminalUi::Ink);
    });
    capture("enemy-telegraphs.png",[&]{
        ClearBackground({188,198,203,255});
        EnemyUnit bomb;bomb.kind=EnemyKind::Exploder;bomb.health=7;bomb.feet={320,530};bomb.state=EnemyState::Fuse;bomb.timer=.4F;
        EnemyUnit archer;archer.kind=EnemyKind::Crossbow;archer.health=9;archer.feet={810,530};archer.state=EnemyState::Windup;archer.timer=.5F;archer.aim={1120,450};archer.facing=1;
        renderer.DrawUnit(bomb,font);renderer.DrawUnit(archer,font);
        font.Draw("范围预警 / 瞄准锁定",51,54,36,TerminalUi::Ink);
    });
}
