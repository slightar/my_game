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
        if(game.RemainingTrialEnemies()!=kinds || game.PendingTrialEnemies()!=kinds)
            throw std::runtime_error("Trial did not queue the full roster");
        frames(180);
        if(game.PendingTrialEnemies()!=kinds)throw std::runtime_error("Enemies spawned before arena entry");
        capture("trial-before-entry.png");
        key(KEY_D,true);frames(155);key(KEY_D,false);
        capture("trial-start.png");
        key(KEY_J,true);frames(620);key(KEY_J,false);
        // The assist aim alone should be able to clear the nearest chaff from range.
        if(game.RemainingTrialEnemies()>kinds-2)throw std::runtime_error("Gunfire failed to defeat low-profile target");
        capture("trial-gunfire.png");
        // Texas again for the melee frame.
        press(KEY_TWO);press(KEY_Q);
        key(KEY_J,true);frames(24);key(KEY_J,false);
        capture("trial-texas.png");
        press(KEY_ESCAPE);
        const int pending=game.PendingTrialEnemies();
        frames(180);
        if(game.PendingTrialEnemies()!=pending)throw std::runtime_error("Pause advanced the spawn queue");
        capture("trial-paused.png");
        // Pause -> home -> original boss demonstration remains selectable.
        press(KEY_DOWN);press(KEY_ENTER);press(KEY_B);
        if(game.RemainingTrialEnemies()!=0)throw std::runtime_error("Boss mode retained trial enemies");
        capture("original-boss-mode.png");
        press(KEY_ESCAPE);press(KEY_DOWN);press(KEY_ENTER);press(KEY_N);
        key(KEY_D,true);frames(300);key(KEY_D,false);
        capture("red-gate-in-battle.png");
        // Inspect the entrance and the first client's sprite at their actual world anchors.
        EnemySystem entryPreview;entryPreview.ResetTrial();
        for(int i=0;i<75;++i)entryPreview.Update(1.0F/60,{1800,GameConfig::kFloorY});
        if(entryPreview.Pending()!=kinds-1)throw std::runtime_error("Entry preview did not spawn");
        EnemyRenderer renderer;UiFont font;
        if(!renderer.HasEntrySprite())throw std::runtime_error("Client entrance sprite failed to load");
        font.SetAdditionalText("敌方入口待入场增援结束红门");
        auto target=LoadRenderTexture(1280,720);BeginTextureMode(target);
        ClearBackground({188,198,203,255});
        BeginMode2D({{640,360}, {1900,360},0,1});
        DrawRectangle(1260,int(GameConfig::kFloorY),1280,80,{68,78,87,255});
        renderer.DrawEntry(entryPreview);renderer.Draw(entryPreview,font);
        EndMode2D();EndTextureMode();
        auto image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
        if(!ExportImage(image,(output/"red-gate-entry.png").string().c_str()))
            throw std::runtime_error("Cannot export entrance screenshot");
        UnloadImage(image);UnloadRenderTexture(target);
        std::cout<<"Enemy trial, low-target gunfire, character switch, pause and boss return passed\n";
    } catch(const std::exception& e){std::cerr<<e.what()<<'\n';result=1;}
    CloseAudioDevice();CloseWindow();return result;
}
