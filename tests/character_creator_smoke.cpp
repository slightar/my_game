#include "character_creator.h"
#include "character_art.h"
#include "character_repository.h"
#include "ui_font.h"
#include "file_path.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <nlohmann/json.hpp>
#include "character_dialog_checks.h"
#include "character_motion_render_checks.h"

struct CharacterCreatorTestAccess {
    // Explicit opt-in only: never called by automated/offline smoke checks.
    static void LiveExusiai(CharacterCreator& c) {
        c.name_="能天使·百炼测试";c.autoName_=false;c.manualCombat_=true;c.combat_=1;
        auto settings=c.settings_;
        settings.instructions="参考人物为明日方舟能天使，保留红棕短发、天使光环、黑白外套与枪械，右向待机，双脚站稳";
        c.job_.Start(settings,c.key_,c.sourcePng_);
    }
    static int Stage(const CharacterCreator& c){return c.job_.Stage();}
    static const std::string& Status(const CharacterCreator& c){return c.status_;}
    static void ExportLive(CharacterCreator& c,const std::filesystem::path& output) {
        if(!c.hasPreview_||!c.generated_.data)throw std::runtime_error(c.status_);
        if(!ExportImage(c.generated_,(output/"generated-original.png").string().c_str()))throw std::runtime_error("Cannot export live result");
        std::filesystem::copy_file(c.stagingFile_.parent_path()/"sprite.png",output/"sprite.png");
        std::filesystem::copy_file(c.stagingFile_.parent_path()/"reference.png",output/"reference.png");
        c.Save();
    }
    static bool Configured(const CharacterCreator& c){return !c.key_.empty();}
    static bool Busy(const CharacterCreator& c){return c.job_.Busy();}
    static bool SuggestionApplied(const CharacterCreator& c){return c.name_=="智能测试"&&c.combat_==1;}
    static bool ManualPreserved(const CharacterCreator& c){return c.name_=="手动角色"&&c.combat_==2;}
    static void Smart(CharacterCreator& c,bool manual) {
        using Json=nlohmann::json;
        c.autoName_=!manual;c.manualCombat_=manual;c.name_=manual?"手动角色":"新角色";c.combat_=2;
        const auto analysis=Json{{"choices",Json::array({{{"message",{{"content",Json{{"name","智能测试"},{"appearance","蓝色外套"},{"combat","ranged"}}.dump()}}}}})}}.dump();
        auto settings=c.settings_;settings.smart=true;
        c.job_.Start(settings,"fake-test-credential",c.sourcePng_,[bytes=c.sourcePng_,analysis](const CharacterHttpRequest& r,const std::atomic_bool&){
            if(r.url.ends_with("/chat/completions"))return CharacterHttpResponse{200,analysis};
            if(r.method=="POST")return CharacterHttpResponse{200,R"({"output":{"choices":[{"message":{"content":[{"image":"https://fixture.example.test/sprite.png"}]}}]}})"};
            return CharacterHttpResponse{200,bytes};
        });
    }
    static bool HasPreview(const CharacterCreator& c) { return c.hasPreview_; }
    static bool Attacking(const CharacterCreator& c) { return c.animator_.Action()==CharacterAction::Attack; }
    static void Error(CharacterCreator& c) {
        try { CharacterGenerationImageUrl({403,R"({"code":"AllocationQuota.FreeTierOnly"})"}); }
        catch(const std::exception& e){c.status_=e.what();}
    }
    static std::string Draft(const CharacterCreator& c) {
        return c.name_ + c.settings_.apiHost + c.settings_.model + c.settings_.instructions + c.sourcePng_;
    }
    static CharacterAction Action(const CharacterCreator& c) { return c.animator_.Action(); }
};
#ifdef _WIN32
void CheckCreatorDialog() {
    const auto app = Utf8Path(GetApplicationDirectory());
    const auto isolated = app/"character-qa"/("dialog-fixture-"+std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    CharacterArt art; UiFont font; CharacterCreator creator(isolated);
    creator.Open(art);
    creator.SetReference(app/".."/"examples"/"single_sprite.png");
    font.SetAdditionalText(creator.Glyphs());
    const auto draft = CharacterCreatorTestAccess::Draft(creator);
    const float scale = std::min(GetScreenWidth()/1280.0F, GetScreenHeight()/720.0F);
    const Vector2 offset{(GetScreenWidth()-1280*scale)/2, (GetScreenHeight()-720*scale)/2};
    const auto redraw = [&] {
        if (!creator.Active() || CharacterCreatorTestAccess::Draft(creator) != draft)
            throw std::runtime_error("Workshop page or draft changed while picking a reference");
        BeginDrawing(); ClearBackground(BLACK);
        BeginMode2D({offset, {}, 0, scale}); creator.Draw(font, art); EndMode2D();
        EndDrawing();
    };
    CharacterDialogChecks::Run([&](const std::function<void()>& present) {
        // Exercise the actual Choose Reference button, not only the platform API.
        PlayAutomationEvent({0,7,{int(offset.x+150*scale),int(offset.y+485*scale),0,0}});
        PlayAutomationEvent({0,6,{MOUSE_BUTTON_LEFT,0,0,0}}); creator.Update(.016F,art,present);
        PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});
        const bool saved = creator.Update(.016F,art,present);
        creator.Update(.016F,art,present); // Cancellation must not leak into navigation on the next update.
        return !saved && creator.Active() && CharacterCreatorTestAccess::Draft(creator) == draft;
    }, redraw);
}
#endif
int main(int argc, char** argv) {
    const bool dialogMode = argc > 1 && std::string(argv[1]) == "--file-dialog";
    SetConfigFlags(dialogMode ? FLAG_WINDOW_UNDECORATED | FLAG_VSYNC_HINT : FLAG_WINDOW_HIDDEN);
    InitWindow(dialogMode ? 0 : 1280, dialogMode ? 0 : 720,"Character creator QA");
    if(!IsWindowReady())return 1;
    int result=0;
    try {
#ifdef _WIN32
        if (dialogMode) {
            CheckCreatorDialog();
            std::cout<<"Workshop choose button: continuous repaint, draft preservation, cancellation, reopening and focus restoration passed\n";
            CloseWindow(); return 0;
        }
#endif
        const auto app=Utf8Path(GetApplicationDirectory());
        const auto output=app/"character-qa";std::filesystem::create_directories(output);
        if(argc>1&&std::string(argv[1])=="--live-exusiai") {
            const auto liveOutput=output/("bailian-exusiai-"+std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
            std::filesystem::create_directories(liveOutput);
            CharacterArt art;UiFont font;CharacterCreator live(app);live.Open(art);
            if(!CharacterCreatorTestAccess::Configured(live))throw std::runtime_error("Local API key missing");
            live.SetReference(app/"assets"/"operators"/"exusiai_portrait.png");
            CharacterCreatorTestAccess::LiveExusiai(live);
            int last=-1;
            while(CharacterCreatorTestAccess::Busy(live)) {
                const int stage=CharacterCreatorTestAccess::Stage(live);
                if(stage!=last){std::cout<<"Live generation stage "<<stage<<std::endl;last=stage;}
                live.Update(.05F,art);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            live.Update(.016F,art);
            if(!CharacterCreatorTestAccess::HasPreview(live))throw std::runtime_error(CharacterCreatorTestAccess::Status(live));
            font.SetAdditionalText(live.Glyphs());
            auto target=LoadRenderTexture(1280,720);BeginTextureMode(target);ClearBackground(BLACK);live.Draw(font,art);EndTextureMode();
            auto image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            const bool saved=ExportImage(image,(liveOutput/"workshop-preview.png").string().c_str());
            UnloadImage(image);UnloadRenderTexture(target);
            if(!saved)throw std::runtime_error("Cannot export workshop preview");
            CharacterCreatorTestAccess::ExportLive(live,liveOutput);
            std::cout<<"LIVE_RESULT "<<liveOutput.string()<<std::endl;
            std::cout<<"Actual Bailian generation, workshop processing and roster save passed"<<std::endl;
            return 0;
        }
        if(argc>1&&std::string(argv[1])=="--local-configuration") {
            CharacterArt art;UiFont font;CharacterCreator configured(app);configured.Open(art);
            if(!CharacterCreatorTestAccess::Configured(configured))throw std::runtime_error("Local key was not loaded");
            font.SetAdditionalText(configured.Glyphs());
            auto target=LoadRenderTexture(1280,720);BeginTextureMode(target);configured.Draw(font,art);EndTextureMode();
            auto image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            const bool saved=ExportImage(image,(output/"creator-configured.png").string().c_str());UnloadImage(image);UnloadRenderTexture(target);
            if(!saved)throw std::runtime_error("Cannot export configured workshop");
            std::cout<<"Local credential preload and masked workshop render passed; no inference submitted\n";
            return 0;
        }
        const auto isolated=output/("creator-fixture-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(isolated);
        CharacterArt art;UiFont font;CharacterCreator creator(isolated);creator.Open(art);
        const auto render=[&](const char* name) {
            font.SetAdditionalText(creator.Glyphs());
            if(!font.SupportsText(creator.Glyphs()))throw std::runtime_error("Creator has unloaded glyphs");
            auto target=LoadRenderTexture(1280,720);BeginTextureMode(target);ClearBackground(BLACK);creator.Draw(font,art);EndTextureMode();
            Image image=LoadImageFromTexture(target.texture);ImageFlipVertical(&image);
            if(!ExportImage(image,(output/name).string().c_str()))throw std::runtime_error("Cannot export capture");
            UnloadImage(image);UnloadRenderTexture(target);
        };
        render("creator-empty.png");
        const auto reference=app/".."/"examples"/"single_sprite.png";
        creator.SetReference(reference);
        const auto click=[&](int x,int y) {
            PlayAutomationEvent({0,7,{x,y,0,0}});PlayAutomationEvent({0,6,{MOUSE_BUTTON_LEFT,0,0,0}});creator.Update(.016F,art);
            PlayAutomationEvent({0,5,{MOUSE_BUTTON_LEFT,0,0,0}});return creator.Update(.016F,art);
        };
        click(600,485); // Use original image, through the actual UI release handler.
        if(!CharacterCreatorTestAccess::HasPreview(creator))throw std::runtime_error("Use original did not build a preview");
        render("creator-preview.png");
        click(942,510);creator.Update(.1F,art);render("creator-running.png");
        click(1100,452);click(1060,510);creator.Update(.1F,art);render("creator-attack-mirrored.png");
        if(!CharacterCreatorTestAccess::Attacking(creator))throw std::runtime_error("Attack preview did not start immediately");
        click(600,547); // Arts combat template.
        click(1176,551); creator.Update(.1F,art); render("creator-defeated.png");
        if(CharacterCreatorTestAccess::Action(creator)!=CharacterAction::Defeated)throw std::runtime_error("Defeat preview missing");
        click(1092,551); render("creator-hurt.png");
        if(CharacterCreatorTestAccess::Action(creator)!=CharacterAction::Hurt)throw std::runtime_error("Hurt preview missing");
        click(1060,510); creator.Update(.08F,art); render("creator-caster-attack.png");
        for(const bool manual : {false,true}) {
            CharacterCreatorTestAccess::Smart(creator,manual);
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            while(CharacterCreatorTestAccess::Busy(creator)&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(CharacterCreatorTestAccess::Busy(creator))throw std::runtime_error("Smart fixture timed out");
            creator.Update(.016F,art);
            if(manual?!CharacterCreatorTestAccess::ManualPreserved(creator):!CharacterCreatorTestAccess::SuggestionApplied(creator))
                throw std::runtime_error("Smart suggestion overwrote manual choice or was not applied");
            render(manual?"creator-smart-manual.png":"creator-smart-auto.png");
        }
        CharacterCreatorTestAccess::Error(creator);render("creator-error.png");
        if(!click(1000,600) || creator.Active())throw std::runtime_error("Save button did not close creator");
        LocalCharacterRepository repository(isolated/"characters");
        if(repository.LoadAll().characters.empty())throw std::runtime_error("Creator save did not persist a character");
        bool arts=false;for(const auto& c:repository.LoadAll().characters)
            if(c.attack.damageType==DamageType::Arts && c.assets.motionTemplate=="caster")arts=true;
        if(!arts)throw std::runtime_error("Combat selection was not saved");
        art.ClearCustomCharacters();creator.Open(art);render("creator-reopened.png");
        CheckMotionTemplateRendering(art,app,output);
        std::cout<<"Creator empty/preview/error render and save passed\n";
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';result=1;}
    CloseWindow();return result;
}
