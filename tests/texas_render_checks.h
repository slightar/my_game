#pragma once
#include "audio_system.h"
#include "game_settings.h"
#include "player.h"
#include "texas_animation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

// Exercise the real input -> combat -> Player::Draw path against the battle sheet.
// This catches the original Wave binding, incomplete swings and wrong mirroring.
template<class Render>
void CheckTexasAttackRendering(const CharacterArt& art, Render render) {
    if (!art.HasTexasBattle()) throw std::runtime_error("Texas client battle sheet missing or malformed");
    AudioSystem audio;
    GameSettings settings(std::filesystem::path(GetApplicationDirectory()) / "texas-fixture");
    const auto key=[](int code,bool down) {PlayAutomationEvent({0,down?2U:1U,{code,0,0,0}});};
    const auto snapshot=[](auto draw) {
        auto target=LoadRenderTexture(1280,720);
        BeginTextureMode(target);ClearBackground(BLANK);draw();EndTextureMode();
        auto image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
        UnloadRenderTexture(target);return image;
    };
    const auto checkPose=[&](const Player& player,bool attack,float time) {
        auto actual=snapshot([&]{player.Draw(art);});
        const auto box=player.Hitbox();const Vector2 feet{player.Position().x,box.y+box.height};
        auto expected=snapshot([&]{
            art.DrawFacingRing({feet.x,GameConfig::kFloorY+2},player.FacingDirection());
            art.DrawTexas(feet,player.FacingDirection(),ChibiAnimation::Idle,attack,false,time,time,0);
        });
        auto a=LoadImageColors(actual);auto b=LoadImageColors(expected);
        int differences=0;
        for(int i=0;i<1280*720;++i)
            if(a[i].a!=b[i].a || (b[i].a>0 &&
               (a[i].r!=b[i].r || a[i].g!=b[i].g || a[i].b!=b[i].b))) ++differences;
        UnloadImageColors(a);UnloadImageColors(b);UnloadImage(actual);UnloadImage(expected);
        if(differences>20) throw std::runtime_error("Texas input path uses wrong battle pose/frame");
    };
    std::vector<Bullet> bullets;
    for(int facing : {1,-1}) {
        Player player;player.Reset(OperatorKind::Texas);player.PlaceAt({640,605},facing);
        checkPose(player,false,0);
        key(KEY_J,true);
        for(int step=1;step<=22;++step) {
            player.Update(1.0F/60,{640+facing*60.0F,605},20,bullets,audio,settings);
            if(step==1) key(KEY_J,false);
            if(step==1||step==6||step==10||step==14||step==18||step==20||step==22)
                checkPose(player,step<=20,step/60.0F);
        }
        if(bullets.size()!=1 || bullets.front().kind!=BulletKind::MeleeSlash ||
           std::abs(bullets.front().damage-1.9F)>.001F ||
           std::abs(bullets.front().lifetime-.17F)>.001F)
            throw std::runtime_error("Normal attack damage/hit window changed");
        bullets.clear();
        key(KEY_J,true);
        for(int step=1;step<=61;++step) {
            player.Update(1.0F/60,{640+facing*60.0F,605},20,bullets,audio,settings);
            if(player.Actions().melee) checkPose(player,true,1.0F/60);
        }
        key(KEY_J,false);
        if(bullets.size()!=4) throw std::runtime_error("Normal attack cadence changed");
        bullets.clear();
    }
    render("texas-normal-attack.png",[&] {
        DrawText("TEXAS / CLIENT ATTACK_START + ATTACK_LOOP + ATTACK_END",30,24,22,SKYBLUE);
        for(int facing : {1,-1}) for(int i=0;i<6;++i) {
            const Vector2 feet{110+i*205.0F,facing==1?270.0F:520.0F};
            DrawLine(static_cast<int>(feet.x-65),static_cast<int>(feet.y),
                     static_cast<int>(feet.x+65),static_cast<int>(feet.y),GRAY);
            art.DrawTexas(feet,facing,ChibiAnimation::Idle,true,false,0,i*.06F,0);
            DrawText(TextFormat("%.2fs",i*.06F),static_cast<int>(feet.x-30),
                     static_cast<int>(feet.y+20),18,LIGHTGRAY);
        }
    });
    render("texas-scale-and-mode.png",[&] {
        DrawText("NORMAL IDLE / WALK / ATTACK / E MODE",30,24,26,SKYBLUE);
        art.DrawTexas({240,420},1,ChibiAnimation::Idle,false,false,0,0,0);
        art.DrawTexas({500,420},1,ChibiAnimation::Run,false,false,.3F,0,0);
        art.DrawTexas({760,420},1,ChibiAnimation::Idle,true,false,0,.14F,0);
        art.DrawTexasSkill2Battle({1020,420},1,120,true,false,.14F);
        DrawLine(120,420,1140,420,GRAY);
    });
    // A sword wave has the same real swing, not a separate building gesture.
    Player ranged;ranged.Reset(OperatorKind::Texas);ranged.PlaceAt({640,605},1);
    key(KEY_J,true);ranged.Update(1.0F/60,{1500,605},20,bullets,audio,settings);key(KEY_J,false);
    if(bullets.size()!=1||bullets.front().kind!=BulletKind::SwordWave)
        throw std::runtime_error("Sword-wave fixture did not attack");
    checkPose(ranged,true,1.0F/60);
    std::cout<<"Texas normal melee/sword-wave poses, full swing, left/right, cadence and hit window passed\n";
}
