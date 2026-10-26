#pragma once
#include "character.h"
#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

struct CharacterGenerationSettings {
    std::string apiHost = "https://dashscope.aliyuncs.com";
    std::string model = "qwen-image-2.0";
    std::string instructions;
    bool smart = true;
    std::string analysisModel = "qwen3.8-max";
    void Normalize();
    void Validate() const;
    static CharacterGenerationSettings Load(const std::filesystem::path& file);
    void Save(const std::filesystem::path& file) const; // Credentials never belong here.
};
std::string LoadCharacterApiKey(const std::filesystem::path& file);
struct CharacterAnalysis {
    std::string name, appearance;
    int combat = -1;
    bool fallback = false;
};

struct CharacterHttpRequest {
    std::string url, method = "GET", bearer, body;
    std::size_t maxResponseBytes = 2 * 1024 * 1024;
};
struct CharacterHttpResponse { int status = 0; std::string body; };
using CharacterHttpTransport = std::function<CharacterHttpResponse(const CharacterHttpRequest&, const std::atomic_bool&)>;
CharacterHttpResponse SendCharacterHttp(const CharacterHttpRequest&, const std::atomic_bool& cancelled);

std::string CharacterGenerationRequest(const CharacterGenerationSettings&, const std::string& pngBytes);
std::string CharacterGenerationImageUrl(const CharacterHttpResponse&);
std::string CharacterAnalysisRequest(const CharacterGenerationSettings&, const std::string& pngBytes);
CharacterAnalysis ParseCharacterAnalysis(const CharacterHttpResponse&);
bool IsCharacterImageUrl(const std::string& url);
Character MakeGeneratedCharacter(const std::string& id, const std::string& name, int combatTemplate);

// The worker owns only shared CPU data. It never touches a window, texture or repository.
// Cancelling discards results; an already submitted cloud generation can still be billed.
class CharacterGenerationJob {
public:
    ~CharacterGenerationJob() { Cancel(); }
    void Start(CharacterGenerationSettings settings, std::string key, std::string pngBytes,
               CharacterHttpTransport transport = SendCharacterHttp);
    void Cancel();
    bool Busy() const;
    bool Take(std::string& imageBytes, std::string& error, CharacterAnalysis* analysis = nullptr);
    int Stage() const { return state_ ? state_->stage.load() : 0; }
private:
    struct State {
        std::atomic_bool cancelled{false}, finished{false};
        std::atomic_int stage{0};
        CharacterAnalysis analysis;
        std::string image, error;
    };
    std::shared_ptr<State> state_;
};
