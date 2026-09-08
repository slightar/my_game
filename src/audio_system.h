#pragma once

#include "raylib.h"

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void PlayGunshot(int ammoRemaining);
    void PlayEmptyClick() const;
    void PlayReload() const;
    void PlayPlayerHit() const;
    void PlayBossHit() const;

private:
    Sound gunshot_{};
    Sound emptyClick_{};
    Sound reload_{};
    Sound playerHit_{};
    Sound bossHit_{};
};
