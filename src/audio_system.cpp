#include "audio_system.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

constexpr unsigned int kSampleRate = 44100;
constexpr float kPi = 3.14159265358979323846F;

std::int16_t ToPcm16(float sample) {
    return static_cast<std::int16_t>(std::clamp(sample, -1.0F, 1.0F) * 32767.0F);
}

template <typename Generator>
Sound CreateSound(float duration, Generator&& generator) {
    const unsigned int frameCount =
        static_cast<unsigned int>(duration * static_cast<float>(kSampleRate));
    std::vector<std::int16_t> samples(frameCount);
    for (unsigned int frame = 0; frame < frameCount; ++frame) {
        const float time = static_cast<float>(frame) / static_cast<float>(kSampleRate);
        samples[frame] = ToPcm16(generator(time, frame));
    }
    Wave wave{frameCount, kSampleRate, 16, 1, samples.data()};
    return LoadSoundFromWave(wave);
}

Sound CreateGunshot() {
    std::uint32_t noiseState = 0xA53C9E21U;
    return CreateSound(0.085F, [&noiseState](float time, unsigned int) {
        noiseState = noiseState * 1664525U + 1013904223U;
        const float noise = static_cast<float>((noiseState >> 8U) & 0x00FFFFFFU) /
                                static_cast<float>(0x00FFFFFFU) *
                                2.0F -
                            1.0F;
        const float body = std::sin(2.0F * kPi * 155.0F * time);
        return noise * std::exp(-55.0F * time) * 0.72F +
               body * std::exp(-30.0F * time) * 0.25F +
               (time < 0.004F ? 0.25F : 0.0F);
    });
}

Sound CreateClick() {
    return CreateSound(0.055F, [](float time, unsigned int) {
        return std::sin(2.0F * kPi * 1750.0F * time) *
               std::exp(-85.0F * time) * 0.5F;
    });
}

Sound CreateReload() {
    return CreateSound(0.34F, [](float time, unsigned int) {
        const float first = std::sin(2.0F * kPi * 680.0F * time) *
                            std::exp(-70.0F * time) * 0.42F;
        if (time < 0.19F) {
            return first;
        }
        const float secondTime = time - 0.19F;
        const float second = std::sin(2.0F * kPi * 1050.0F * secondTime) *
                             std::exp(-80.0F * secondTime) * 0.55F;
        return first + second;
    });
}

Sound CreateHit(float frequency, float duration) {
    std::uint32_t noiseState = 0x7F4A7C15U;
    return CreateSound(duration, [frequency, &noiseState](float time, unsigned int) {
        noiseState = noiseState * 1103515245U + 12345U;
        const float noise = static_cast<float>((noiseState >> 16U) & 0x7FFFU) /
                                16383.5F -
                            1.0F;
        const float tone = std::sin(2.0F * kPi * frequency * time);
        return (tone * 0.6F + noise * 0.25F) * std::exp(-22.0F * time);
    });
}

}  // namespace

AudioSystem::AudioSystem()
    : gunshot_(CreateGunshot()),
      emptyClick_(CreateClick()),
      reload_(CreateReload()),
      playerHit_(CreateHit(105.0F, 0.18F)),
      bossHit_(CreateHit(240.0F, 0.09F)) {
    SetSoundVolume(gunshot_, 0.28F);
    SetSoundVolume(emptyClick_, 0.35F);
    SetSoundVolume(reload_, 0.55F);
    SetSoundVolume(playerHit_, 0.55F);
    SetSoundVolume(bossHit_, 0.18F);
}

AudioSystem::~AudioSystem() {
    StopSound(gunshot_);
    UnloadSound(gunshot_);
    UnloadSound(emptyClick_);
    UnloadSound(reload_);
    UnloadSound(playerHit_);
    UnloadSound(bossHit_);
}

void AudioSystem::PlayGunshot(int ammoRemaining) {
    SetSoundPitch(gunshot_, 0.96F + static_cast<float>(ammoRemaining % 5) * 0.02F);
    PlaySound(gunshot_);
}

void AudioSystem::PlayEmptyClick() const {
    PlaySound(emptyClick_);
}

void AudioSystem::PlayReload() const {
    PlaySound(reload_);
}

void AudioSystem::PlayPlayerHit() const {
    PlaySound(playerHit_);
}

void AudioSystem::PlayBossHit() const {
    PlaySound(bossHit_);
}
