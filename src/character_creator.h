#pragma once
#include "character_generation.h"
#include "character_animation.h"
#include "ui_input.h"
#include <functional>

class UiFont;
class CharacterArt;
void SetCharacterTextInput(bool enabled);
std::filesystem::path ChooseCharacterReference(const std::function<void()>& redraw = {});

// Returns a new owned image. Only green pixels connected to the border are keyed.
Image PrepareCharacterSprite(const Image& source, bool removeGreen);
Image DecodeGeneratedCharacterImage(const std::string& bytes);

class CharacterCreator {
public:
    explicit CharacterCreator(std::filesystem::path appDirectory);
    ~CharacterCreator();
    void Open(CharacterArt& art);
    bool Active() const { return active_; }
    bool Update(float dt, CharacterArt& art, const std::function<void()>& redraw = {});
    void Draw(const UiFont& font, const CharacterArt& art) const;
    std::string Glyphs() const;
    void SetReference(const std::filesystem::path& file);
private:
    friend struct CharacterCreatorTestAccess;
    void Close();
    void Start();
    void MakePreview(CharacterArt& art);
    void InvalidatePreview();
    void Save();
    std::filesystem::path app_, referenceFile_, stagingFile_;
    CharacterGenerationSettings settings_;
    CharacterGenerationJob job_;
    std::string key_, name_ = "新角色", status_ = "选择或拖入一张参考图，生成你的角色", sourcePng_;
    Image reference_{}, generated_{};
    Texture2D referenceTexture_{};
    Character preview_;
    CharacterAnimator animator_;
    UiPointerState pointerState_;
    UiPointer pointer_;
    int focus_ = -1, combat_ = 0, previewAction_ = 0, facing_ = 1;
    float previewTime_ = 0;
    float generationTime_ = 0;
    bool manualCombat_ = false, autoName_ = false, localKey_ = false;
    std::string analysisDescription_;
    bool active_ = false, removeGreen_ = true, hasPreview_ = false, transparentPreview_ = false;
};
