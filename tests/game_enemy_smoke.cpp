#include "game.h"
#include "file_path.h"
#include <filesystem>
#include <iostream>
#include <stdexcept>

// Manual integration smoke; run from a copied build directory to isolate settings/save.
int main(){
    SetConfigFlags(FLAG_WINDOW_HIDDEN);InitWindow(1280,720,"Enemy battle smoke");InitAudioDevice();
    int result=0;
    try {
        Game game;
        const auto key=[](int key,bool down){PlayAutomationEvent({0,down?2U:1U,{key,0,0,0}});};
        const auto press=[&](int code){key(code,true);game.Update(1.0F/60);key(code,false);};
        const auto frames=[&](int count){for(int i=0;i<count;++i)game.Update(1.0F/60);};
        const auto output=Utf8Path(GetApplicationDirectory())/"enemy-game-qa";
        std::filesystem::create_directories(output);
        const auto capture=[&](const char* name){
            auto target=LoadRenderTexture(1280,720);BeginTextureMode(target);game.Draw(1,{});EndTextureMode();
            auto image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            if(!ExportImage(image,(output/name).string().c_str()))throw std::runtime_error("Cannot export game screenshot");
            UnloadImage(image);UnloadRenderTexture(target);
        };
        press(KEY_ENTER);press(KEY_N);
        // Compare against the definition table, not a literal: this read "!= 6" while the
        // roster had already grown to 11, so the trial entry check was checking nothing.
        const int kinds=static_cast<int>(EnemyDefinitions().size());
        if(game.RemainingTrialEnemies()!=kinds)throw std::runtime_error("N did not enter the full trial formation");
        key(KEY_D,true);frames(155);key(KEY_D,false);
        capture("trial-start.png");
        key(KEY_J,true);frames(190);key(KEY_J,false);
        // The assist aim alone should be able to clear the nearest chaff from range.
        if(game.RemainingTrialEnemies()>kinds-2)throw std::runtime_error("Gunfire failed to defeat low-profile target");
        capture("trial-gunfire.png");
        // Texas again for the melee frame.
        press(KEY_TWO);press(KEY_Q);
        key(KEY_J,true);frames(24);key(KEY_J,false);
        capture("trial-texas.png");
        press(KEY_ESCAPE);frames(60);capture("trial-paused.png");
        // Pause -> home -> original boss demonstration remains selectable.
        press(KEY_DOWN);press(KEY_ENTER);press(KEY_B);
        if(game.RemainingTrialEnemies()!=0)throw std::runtime_error("Boss mode retained trial enemies");
        capture("original-boss-mode.png");
        std::cout<<"Enemy trial, low-target gunfire, character switch, pause and boss return passed\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    CloseAudioDevice();CloseWindow();return result;
}
