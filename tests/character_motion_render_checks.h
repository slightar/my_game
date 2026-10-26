#pragma once
#include "character_art.h"
#include "character_generation.h"
#include <fstream>
#include <sstream>

inline void CheckMotionTemplateRendering(CharacterArt& art, const std::filesystem::path& app,
                                        const std::filesystem::path& output) {
    const auto examples = app/".."/"examples";
    std::ifstream input(examples/"rig_character.json");
    std::stringstream text; text << input.rdbuf();
    auto rig = Character::FromJson(text.str());
    rig.id="qa_motion_rig"; rig.assets.height=120;
    art.RegisterCharacter(rig,examples);
    auto single=MakeGeneratedCharacter("qa_motion_single","Motion QA",0);
    single.assets.sprite="single_sprite.png"; single.assets.height=120;
    art.RegisterCharacter(single,examples);
    auto target=LoadRenderTexture(1280,720);
    const auto save=[&](const char* name) {
        Image image=LoadImageFromTexture(target.texture); ImageFlipVertical(&image);
        const bool ok=ExportImage(image,(output/name).string().c_str());
        UnloadImage(image);
        if (!ok) throw std::runtime_error("Cannot export motion template sheet");
    };
    BeginTextureMode(target); ClearBackground({230,235,237,255});
    const float times[]={0,.04F,.08F,.12F,.20F,.40F};
    const char* names[]={"MELEE / SLASH","RANGED / RECOIL","CASTER / RELEASE"};
    for(int row=0;row<3;++row) {
        DrawText(names[row],18,12+row*240,20,DARKGRAY);
        for(int col=0;col<6;++col) {
            CharacterAnimator animator;
            CharacterMotion m; m.attackTriggered=true;
            m.style=static_cast<CharacterMotionStyle>(row+1);
            animator.Update(.016F,m); m.attackTriggered=false;
            for(float t=0;t<times[col]-.0001F;t+=.01F) animator.Update(.01F,m);
            DrawText(TextFormat("%.2fs",times[col]),col*212+80,row*240+40,16,DARKGRAY);
            art.DrawAnimatedCharacter(single,{col*212+95.0F,row*240+215.0F},1,animator);
        }
    }
    EndTextureMode(); save("motion-template-attacks.png");
    BeginTextureMode(target); ClearBackground({230,235,237,255});
    const char* actions[]={"IDLE","RUN","ATTACK","JUMP","FALL","LAND","DODGE","HURT","DEFEATED"};
    for(int row=0;row<2;++row) {
        DrawText(row?"PARTS / SAME TEMPLATE":"SINGLE IMAGE / SAME TEMPLATE",18,row*340+18,20,DARKGRAY);
        for(int col=0;col<9;++col) {
            CharacterAnimator animator; CharacterMotion m;
            m.style=CharacterMotionStyle::Melee;
            if(col==1)m.velocity.x=320;
            if(col==2)m.attackTriggered=true;
            if(col==3 || col==4){m.grounded=false;m.velocity.y=col==3?-200:200;}
            if(col==5){CharacterMotion air;air.grounded=false;animator.Update(0,air);}
            if(col==6)m.dodging=true;
            if(col==7)animator.TriggerHurt();
            if(col==8)m.dead=true;
            animator.Update(.016F,m);m.attackTriggered=false;
            const int steps=col==8?45:col==5?1:5;
            for(int i=0;i<steps;++i)animator.Update(.016F,m);
            DrawText(actions[col],col*141+18,row*340+65,16,DARKGRAY);
            const float x=col==8?(row?1225.0F:1135.0F):col*141+58.0F;
            art.DrawAnimatedCharacter(row?rig:single,{x,row*340+245.0F},
                                      row?-1:1,animator);
        }
    }
    EndTextureMode(); save("motion-template-states.png"); UnloadRenderTexture(target);
}
