#include "character_creator.h"
#include "character_repository.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <thread>
#include <stdexcept>

using Json = nlohmann::json;
void Require(bool condition, const char* why) { if (!condition) throw std::runtime_error(why); }
template<class F> void Throws(F f) { bool threw=false; try { f(); } catch (...) { threw=true; } Require(threw,"Expected validation failure"); }
void Wait(CharacterGenerationJob& job) {
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(job.Busy() && std::chrono::steady_clock::now()<until) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    Require(!job.Busy(),"Worker did not finish");
}
int main(int argc, char** argv) { try {
    if (argc == 3 && std::string(argv[1]) == "--network") {
        CharacterGenerationSettings probe; probe.apiHost=argv[2]; probe.Validate();
        std::atomic_bool cancelled{false};
        const auto response=SendCharacterHttp({probe.apiHost+"/api/v1/services/aigc/multimodal-generation/generation",
            "POST", {}, "{}", 1024*1024},cancelled);
        // No key is sent: this verifies the real transport without submitting a billable job.
        Require(response.status==401 || response.status==403,"API did not return the expected authentication challenge");
        std::cout<<"HTTPS/TLS endpoint reachable; unauthenticated HTTP "<<response.status<<" (no generation submitted)\n";
        return 0;
    }
    const auto root=std::filesystem::temp_directory_path()/("character-generation-test-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    CharacterGenerationSettings settings; settings.instructions="保留红色外套";
    settings.Save(root/"settings.json");
    const auto loaded=CharacterGenerationSettings::Load(root/"settings.json");
    Require(loaded.model==settings.model && loaded.instructions==settings.instructions,"Settings roundtrip failed");
    std::ifstream in(root/"settings.json"); const auto saved=Json::parse(in);
    Require(saved.size()==6 && !saved.contains("apiKey"),"Settings unexpectedly store credentials");
    auto normalized=settings;normalized.apiHost=" https://dashscope.aliyuncs.com/compatible-mode/v1/ ";normalized.Normalize();
    Require(normalized.apiHost=="https://dashscope.aliyuncs.com","Pasted SDK base URL was not normalized");
    auto chat=settings;chat.model="qwen3.8-max";Throws([&]{chat.Validate();});
    for (const auto& host : {"http://dashscope.aliyuncs.com", "https://example.com", "https://dashscope.aliyuncs.com.evil.test", "https://dashscope.aliyuncs.com/path"}) {
        auto bad=settings; bad.apiHost=host; Throws([&]{bad.Validate();});
    }
    auto workspace=settings; workspace.apiHost="https://workspace-test.cn-beijing.maas.aliyuncs.com"; workspace.Validate();
    const auto request=Json::parse(CharacterGenerationRequest(settings,"abc"));
    const auto& content=request.at("input").at("messages").at(0).at("content");
    Require(content.at(0).at("image")=="data:image/png;base64,YWJj","Reference was not encoded as inline PNG");
    Require(content.at(1).at("text").get<std::string>().find(settings.instructions)!=std::string::npos,"Lost character instructions");
    Require(request.at("parameters").at("n")==1 && request.at("parameters").at("watermark")==false,"Unexpected generation count/watermark");
    Throws([&]{CharacterGenerationRequest(settings,"");});
    const std::string officialResponse=R"({"output":{"choices":[{"finish_reason":"stop","message":{"role":"assistant","content":[{"image":"https://assets.example.test/generated.png?Expires=123&Signature=abc"}]}}]},"request_id":"fixture"})";
    Require(CharacterGenerationImageUrl({200,officialResponse}).starts_with("https://assets."),"Cannot parse documented Bailian result");
    for(int status : {400,401,403,404,429,500}) Throws([&]{CharacterGenerationImageUrl({status,"DO-NOT-ECHO-SERVER-BODY"});});
    Throws([&]{CharacterGenerationImageUrl({200,"not json"});});
    Throws([&]{CharacterGenerationImageUrl({200,R"({"output":{"choices":[]}})"});});
    Require(!IsCharacterImageUrl("http://example.test/x") && !IsCharacterImageUrl("https://user:password@example.test/x"),"Unsafe URL accepted");
    settings.smart=false;
    CharacterGenerationJob job; int calls=0;
    job.Start(settings,"test-credential", "abc", [&](const CharacterHttpRequest& r,const std::atomic_bool&) {
        ++calls;
        if(r.method=="POST") { Require(r.bearer=="test-credential","POST lacks auth"); Require(r.url.ends_with("/api/v1/services/aigc/multimodal-generation/generation"),"Wrong API path"); return CharacterHttpResponse{200,officialResponse}; }
        Require(r.bearer.empty(),"Credential leaked to signed image URL"); Require(r.body.empty(),"Download contains request body");
        return CharacterHttpResponse{200,"png-result"};
    });
    Wait(job); std::string image,error;
    Require(job.Take(image,error) && image=="png-result" && error.empty() && calls==2,"Generation/download chain failed");
    Require(!job.Take(image,error),"Result was delivered twice");
    settings.smart=true;int analyses=0,generations=0,downloads=0;
    const auto analyzed=Json{{"choices",Json::array({{{"message",{{"content",Json{{"name","蓝衣术师"},{"appearance","蓝色外套，手持法杖"},{"combat","caster"}}.dump()}}}}})}}.dump();
    job.Start(settings,"test-key","abc",[&](const CharacterHttpRequest& r,const std::atomic_bool&){
        if(r.url.ends_with("/chat/completions")) {++analyses;Require(Json::parse(r.body).at("messages").at(0).at("content").at(0).at("type")=="image_url","Analysis lacks reference image");return CharacterHttpResponse{200,analyzed};}
        if(r.method=="POST"){++generations;Require(r.body.find("蓝色外套")!=std::string::npos,"Appearance not passed to image generation");return CharacterHttpResponse{200,officialResponse};}
        Require(r.bearer.empty(),"Download leaked API key");++downloads;return CharacterHttpResponse{downloads==1?503:200,"png-result"};
    });
    Wait(job);CharacterAnalysis analysis;
    Require(job.Take(image,error,&analysis)&&error.empty()&&analysis.combat==2&&analysis.name=="蓝衣术师"&&analyses==1&&generations==1&&downloads==2,
            "Smart analysis/download recovery duplicated generation or lost metadata");
    job.Start(settings,"test-key","abc",[&](const CharacterHttpRequest& r,const std::atomic_bool&){
        if(r.url.ends_with("/chat/completions"))return CharacterHttpResponse{404,"unavailable"};
        return r.method=="POST"?CharacterHttpResponse{200,officialResponse}:CharacterHttpResponse{200,"png"};
    });
    Wait(job);Require(job.Take(image,error,&analysis)&&error.empty()&&analysis.fallback,"Unavailable analysis did not fall back explicitly");
    int deniedCalls=0;
    job.Start(settings,"test-key","abc",[&](const CharacterHttpRequest&,const std::atomic_bool&){++deniedCalls;return CharacterHttpResponse{401,"secret-body"};});
    Wait(job);Require(job.Take(image,error)&&!error.empty()&&deniedCalls==1,"Authentication failure submitted an extra request");
    settings.smart=false;
    for(bool nested:{false,true}) {
        int requests=0;
        const Json detail={{"code","AllocationQuota.FreeTierOnly"},{"message","private-test-key"}};
        const auto body=(nested?Json{{"error",detail}}:detail).dump();
        job.Start(settings,"private-test-key","abc",[&](const CharacterHttpRequest&,const std::atomic_bool&){
            ++requests;return CharacterHttpResponse{403,body};
        });
        Wait(job);Require(job.Take(image,error)&&image.empty()&&requests==1&&
            error.find("免费额度")!=std::string::npos&&error.find("图片生成")!=std::string::npos&&
            error.find("鉴权失败")==std::string::npos&&error.find("private-test-key")==std::string::npos,
            "Free-tier exhaustion misreported, leaked response, or retried generation");
    }
    settings.smart=true;
    int quotaCalls=0;
    job.Start(settings,"test-key","abc",[&](const CharacterHttpRequest&,const std::atomic_bool&){
        ++quotaCalls;return CharacterHttpResponse{403,R"({"error":{"code":"AllocationQuota.FreeTierOnly"}})"};
    });
    Wait(job);Require(job.Take(image,error)&&quotaCalls==1&&error.find("图片分析")!=std::string::npos&&error.find("免费额度")!=std::string::npos,
        "Analysis quota failure lost stage or submitted a generation");
    settings.smart=false;
    job.Start(settings,"private-test-key","abc",[](const CharacterHttpRequest&,const std::atomic_bool&)->CharacterHttpResponse { throw std::runtime_error("private-test-key"); });
    Wait(job); Require(job.Take(image,error) && error.find("private-test-key")==std::string::npos,"Failure exposed credential");
    std::mutex mutex; std::condition_variable gate; bool release=false;
    job.Start(settings,"test-key","abc",[&](const CharacterHttpRequest&,const std::atomic_bool&) {
        std::unique_lock lock(mutex); gate.wait(lock,[&]{return release;}); return CharacterHttpResponse{200,officialResponse};
    });
    Throws([&]{job.Start(settings,"test-key","abc");});
    job.Cancel(); {std::lock_guard lock(mutex);release=true;} gate.notify_all(); Wait(job);
    Require(job.Take(image,error) && image.empty() && !error.empty(),"Cancelled generation published a late image");
    for(int type=0;type<3;++type) {
        auto c=MakeGeneratedCharacter("test_hero","测试角色",type);
        Require(c.skills.size()==2 && Character::FromJson(c.ToJson()).name=="测试角色","Combat template invalid");
    }
    // Keep a green costume patch enclosed by an opaque silhouette; only border-connected green is removed.
    Image raw=GenImageColor(40,48,GREEN); ImageDrawRectangle(&raw,10,7,20,34,RED); ImageDrawRectangle(&raw,16,15,6,6,GREEN);
    Image keyed=PrepareCharacterSprite(raw,true);
    Require(keyed.width==32 && keyed.height==46,"Sprite was not cropped with consistent margins");
    auto* pixels=LoadImageColors(keyed);
    Require(pixels[0].a==0,"Background not transparent");
    const auto costume=pixels[(6+8)*keyed.width+(6+6)];
    Require(costume.g>costume.r && costume.a==255,"Enclosed green costume was removed");
    UnloadImageColors(pixels);
    Image opaque=PrepareCharacterSprite(raw,false); pixels=LoadImageColors(opaque);
    Require(pixels[6*opaque.width+6].a==255,"Disable keying did not preserve original"); UnloadImageColors(pixels);
    int count=0; auto* png=ExportImageToMemory(keyed,".png",&count);
    const std::string bytes(reinterpret_cast<char*>(png),count); MemFree(png);
    Image decoded=DecodeGeneratedCharacterImage(bytes); Require(decoded.width==keyed.width,"PNG roundtrip failed"); UnloadImage(decoded);
    Throws([&]{DecodeGeneratedCharacterImage("<html>not an image</html>");});
    auto bomb=bytes; bomb[16]=0x7f; Throws([&]{DecodeGeneratedCharacterImage(bomb);});
    // Import the generated package, delete its source files, then reload the managed character.
    {std::ofstream out(root/"sprite.png",std::ios::binary);out.write(bytes.data(),bytes.size());}
    auto hero=MakeGeneratedCharacter("persistent_hero","测试角色",0); hero.assets.sprite="sprite.png";
    {std::ofstream out(root/"hero.json");out<<hero.ToJson();}
    LocalCharacterRepository repository(root/"characters"); const auto imported=repository.Import(root/"hero.json");
    std::filesystem::remove(root/"sprite.png"); std::filesystem::remove(root/"hero.json");
    Require(std::filesystem::exists(repository.Root()/imported.assets.sprite),"Saved hero depends on draft image");
    Require(repository.LoadAll().characters.size()==1,"Saved hero not reloadable");
    UnloadImage(raw); UnloadImage(keyed); UnloadImage(opaque);
    // Do not recursively delete computed paths; the tiny fixture remains in the OS temp directory.
    std::cout<<"Bailian protocol, credentials, cancellation, image processing and persistence passed\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }
