#include "audio_system.h"
#include "file_path.h"
#include <algorithm>

namespace {
struct CueDefinition { const char* file; float gain; double interval; int voices; };
// Gameplay emits events without knowing resource paths or managing buffers.
constexpr std::array<CueDefinition, static_cast<int>(AudioCue::Count)> kCues{{
    {"gunshot", .22F, .04, 4}, {"empty", .25F, .18, 1},
    {"reload", .40F, .25, 1}, {"player_hit", .38F, .25, 1},
    {"enemy_hit", .13F, .07, 3}, {"slash", .27F, .10, 2},
    {"skill", .30F, .30, 1}, {"sword_rain", .32F, .30, 1},
    {"ui_confirm", .35F, .10, 1}, {"ui_select", .25F, .08, 1},
    {"ui_back", .30F, .10, 1}, {"operator_switch", .38F, .18, 1},
    {"scene_enter", .30F, .35, 1}, {"investigate", .38F, 1.0, 1},
    {"encounter", .32F, .60, 1}, {"clear", .40F, .60, 1},
    {"pause", .32F, .10, 1}, {"narration", .20F, 1.4, 1},
    {"denied", .28F, .60, 1}, {"hidden_enter", .35F, 1.0, 1}
}};
bool Valid(AudioCue cue) { return cue >= AudioCue::Gunshot && cue < AudioCue::Count; }
}

AudioSystem::AudioSystem(const std::filesystem::path& assetDirectory) {
    if (!IsAudioDeviceReady()) return;
    const auto root = assetDirectory.empty()
        ? Utf8Path(GetApplicationDirectory()) / "assets/audio/client" : assetDirectory;
    for (int i = 0; i < static_cast<int>(kCues.size()); ++i) {
        const auto path = root / (std::string(kCues[i].file) + ".wav");
        const auto utf8 = path.u8string();
        const auto* file = reinterpret_cast<const char*>(utf8.c_str());
        if (!FileExists(file)) {
            TraceLog(LOG_WARNING, "AUDIO: Missing original client cue: %s", file);
            continue;
        }
        auto& clip = clips_[i];
        clip.voices[0] = LoadSound(file);
        if (!IsSoundValid(clip.voices[0])) continue;
        clip.voiceCount = 1;
        // Independent cursors share PCM data. Bounded polyphony retains short
        // tails during rapid fire without unbounded sound stacking.
        for (int v = 1; v < kCues[i].voices; ++v) {
            auto alias = LoadSoundAlias(clip.voices[0]);
            if (!IsSoundValid(alias)) break;
            clip.voices[clip.voiceCount++] = alias;
        }
    }
    SetEffectsVolume(1);
}

AudioSystem::~AudioSystem() {
    for (auto& clip : clips_)
        for (int v = clip.voiceCount - 1; v >= 0; --v) {
            StopSound(clip.voices[v]);
            if (v) UnloadSoundAlias(clip.voices[v]);
            else UnloadSound(clip.voices[v]);
        }
}

bool AudioSystem::Available(AudioCue cue) const {
    return Valid(cue) && clips_[static_cast<int>(cue)].voiceCount > 0;
}

bool AudioSystem::Play(AudioCue cue) const {
    if (!Available(cue) || effectsVolume_ <= 0) return false;
    const int i = static_cast<int>(cue);
    const auto& clip = clips_[i];
    const double now = GetTime();
    if (now - clip.lastPlayed < kCues[i].interval) return false;
    clip.lastPlayed = now;
    PlaySound(clip.voices[clip.nextVoice]);
    clip.nextVoice = (clip.nextVoice + 1) % clip.voiceCount;
    return true;
}

void AudioSystem::SetEffectsVolume(float volume) {
    effectsVolume_ = std::clamp(volume, 0.0F, 1.0F);
    for (int i = 0; i < static_cast<int>(kCues.size()); ++i)
        for (int v = 0; v < clips_[i].voiceCount; ++v) {
            SetSoundVolume(clips_[i].voices[v], kCues[i].gain * effectsVolume_);
            if (effectsVolume_ == 0) StopSound(clips_[i].voices[v]);
        }
}

void AudioSystem::PlayGunshot(int) { Play(AudioCue::Gunshot); }
