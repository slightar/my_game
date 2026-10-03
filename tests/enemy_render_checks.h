#pragma once
#include "enemy_renderer.h"
#include "terminal_ui.h"
#include "ui_font.h"
#include <stdexcept>

template<class Capture>
void CheckEnemyRendering(const UiFont& font,Capture capture){
    EnemyRenderer renderer;
    for(const auto& d:EnemyDefinitions())if(!renderer.HasSprite(d.kind))throw std::runtime_error("Missing enemy sprite sheet");
    // Derived from the roster rather than hard-coded: the definition table grew from 6 to
    // 11 kinds as the official names landed, and a fixed count silently drops the newest
    // entries from the capture.
    const int kinds=static_cast<int>(EnemyDefinitions().size());
    capture("enemy-directions.png",[&]{
        ClearBackground({188,198,203,255});
        // Four rigs, four directions each - one melee guard, one flying drone, one fast
        // quadruped and one caster, so the mirroring is exercised on rigs whose silhouettes
        // are not symmetric (the hound and the drone are the ones that would show a wrong
        // bake at a glance).
        const EnemyKind sample[4]={EnemyKind::Shield,EnemyKind::Drone,EnemyKind::Hound,EnemyKind::Caster};
        for(int r=0;r<4;++r)for(int c=0;c<6;++c){
            EnemyUnit u;u.kind=sample[r];
            u.health=EnemyData(u.kind).health;u.feet={110+c*210.0F,170+r*160.0F};
            u.facing=r%2==0?1:-1;u.moving=true;u.animationTime=c*.2F;
            renderer.DrawUnit(u,font);
            font.Draw(u.facing>0?"RIGHT ->":"<- LEFT",u.feet.x-40,u.feet.y+12,16,TerminalUi::Ink);
        }
    });
    for(int page=0;page<(kinds+11)/12;++page) {
    const std::string filename=page==0?"enemy-roster.png":"enemy-roster-"+std::to_string(page+1)+".png";
    capture(filename.c_str(),[&]{
        ClearBackground({188,198,203,255});
        DrawRectangle(0,0,1280,86,TerminalUi::Ink);
        font.Draw("敌方单位 / 战斗设计",42,24,34,TerminalUi::Paper);
        font.Draw(TextFormat("%d 种 · 客户端模型离线烘焙 · 数值为本项目设计",kinds),640,44,18,Fade(TerminalUi::Paper,.75F));
        // Six cards per row at 213px pitch (190px wide, 23px gutter) keeps all eleven inside
        // the 1280px frame; two rows of six. The card only has to hold the *visible* rig -
        // which is d.height tall, not spriteSize, because the baked cell is padded - so
        // 288px is comfortable even for the 110px soldier.
        constexpr float cardW=190.0F,halfW=95.0F,cardH=288.0F,pitch=213.0F;
        for(int i=page*12;i<std::min(kinds,(page+1)*12);++i){
            const auto& d=EnemyDefinitions()[static_cast<unsigned>(i)];
            const float x=106.0F+(i%6)*pitch;
            const float cardTop=(i%12)<6?100.0F:396.0F;
            const float feetY=cardTop+160.0F;
            DrawRectangle(int(x-halfW),int(cardTop),int(cardW),int(cardH),Color{221,227,226,255});
            EnemyUnit unit;unit.id=i+1;unit.kind=d.kind;unit.health=d.health;
            unit.feet={x,feetY};unit.facing=-1;unit.animationTime=.25F;
            renderer.DrawUnit(unit,font);
            font.Draw(TextFormat("HP %.0f / SPEED %.0f",d.health,d.speed),x-88,feetY+34,14,TerminalUi::Ink);
            TerminalUi::Wrap(font,d.tactic,{x-88,feetY+58,176,cardH-224.0F},14,TerminalUi::Ink);
        }
    });
    }
    // The two "leaves a parting gift when it dies" slugs, plus the archer's aim line: the
    // three telegraphs a player has to read before the punish window opens.
    capture("enemy-telegraphs.png",[&]{
        ClearBackground({188,198,203,255});
        EnemyUnit bomb;bomb.kind=EnemyKind::SlugHigh;bomb.health=15;bomb.feet={280,540};bomb.state=EnemyState::Fuse;bomb.timer=.5F;
        EnemyUnit archer;archer.kind=EnemyKind::Crossbow;archer.health=9;archer.feet={660,540};archer.state=EnemyState::Windup;archer.timer=.5F;archer.aim={980,450};archer.facing=1;
        EnemyUnit acid;acid.kind=EnemyKind::AcidSlug;acid.health=11;acid.feet={1090,560};acid.state=EnemyState::Windup;acid.timer=.6F;acid.aim={1250,470};acid.facing=1;
        renderer.DrawUnit(bomb,font);renderer.DrawUnit(archer,font);renderer.DrawUnit(acid,font);
        font.Draw("范围预警 / 瞄准锁定 / 酸液腐蚀",51,54,36,TerminalUi::Ink);
        font.Draw("物理引爆（死亡必定触发）",132,624,18,TerminalUi::Ink);
        font.Draw("瞄准线锁定后侧移",560,624,18,TerminalUi::Ink);
        font.Draw("酸液命中削弱防护",980,624,18,TerminalUi::Ink);
    });
    // The stealth mechanic is the one rule that cannot be checked by looking at a single
    // frame, so capture both sides of the threshold side by side: cloaked when the player
    // is far, solid the moment the player is inside kCloakRevealRange. The bottom strip
    // shows 辐能源石虫's parting gift, which is otherwise invisible in a still frame.
    capture("enemy-stealth.png",[&]{
        ClearBackground({188,198,203,255});
        DrawRectangle(0,0,1280,86,TerminalUi::Ink);
        font.Draw("隐身弩手 / 显形判定",42,24,34,TerminalUi::Paper);
        EnemyUnit far;far.kind=EnemyKind::StealthCrossbow;far.health=EnemyData(far.kind).health;
        far.feet={330,350};far.facing=1;far.cloaked=true;far.animationTime=.25F;
        EnemyUnit near=far;near.feet={880,350};near.cloaked=false;
        renderer.DrawUnit(far,font);renderer.DrawUnit(near,font);
        font.Draw(TextFormat("距离 > %.0f px  ·  隐身",EnemySystem::kCloakRevealRange),198,430,19,TerminalUi::Ink);
        font.Draw(TextFormat("距离 < %.0f px  ·  显形",EnemySystem::kCloakRevealRange),748,430,19,TerminalUi::Ink);
        font.Draw("远程弹药直接穿过隐身单位；近战伤害不受影响",330,496,19,TerminalUi::Ink);
        EnemyUnit fed;fed.kind=EnemyKind::Soldier;fed.health=EnemyData(fed.kind).health;
        fed.feet={310,706};fed.facing=-1;fed.haste=2.0F;fed.animationTime=.25F;
        renderer.DrawUnit(fed,font);
        font.Draw("辐能源石虫死亡后为其他敌人「供能」加速",440,636,21,Color{214,110,214,255});
        font.Draw(TextFormat("供能期间移动速度 x%.1f，持续 %.1f 秒",EnemySystem::kEnergizeSpeedScale,EnemySystem::kEnergizeDuration),440,670,18,TerminalUi::Ink);
    });
}
