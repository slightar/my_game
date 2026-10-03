#pragma once
#include "raylib.h"
#include <array>
#include <filesystem>

enum class AudioCue {
    Gunshot, Empty, Reload, PlayerHit, EnemyHit, Slash, Skill, SwordRain,
    UiConfirm, UiSelect, UiBack, OperatorSwitch, SceneEnter, Investigate,
    Encounter, Clear, Pause, Narration, Denied, HiddenEnter, Count
};

class AudioSystem {
public:
    explicit AudioSystem(const std::filesystem::path& assetDirectory = {});
    ~AudioSystem();
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
    // Returns false for unavailable, muted or rate-limited cues.
    bool Play(AudioCue cue) const;
    bool Available(AudioCue cue) const;
    void PlayGunshot(int ammoRemaining);
    void PlayEmptyClick() const { Play(AudioCue::Empty); }
    void PlayReload() const { Play(AudioCue::Reload); }
    void PlayPlayerHit() const { Play(AudioCue::PlayerHit); }
    void PlayBossHit() const { Play(AudioCue::EnemyHit); }
    void SetEffectsVolume(float volume);
private:
    friend struct AudioSystemTestAccess;
    struct Clip {
        std::array<Sound, 4> voices{};
        int voiceCount = 0;
        mutable int nextVoice = 0;
        mutable double lastPlayed = -100;
    };
    std::array<Clip, static_cast<int>(AudioCue::Count)> clips_{};
    float effectsVolume_ = 1;
};
