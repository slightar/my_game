#include "boss_audio.h"
#include "file_path.h"
#include <algorithm>
BossAudio::BossAudio() {
    if(!IsAudioDeviceReady())return;
    const auto root=Utf8Path(GetApplicationDirectory())/"assets/enemies/bosses/audio";
    ring_=LoadSound((root/"ice_ring.wav").string().c_str());seal_=LoadSound((root/"ice_seal.wav").string().c_str());
    const auto common=Utf8Path(GetApplicationDirectory())/"assets/audio/client";
    leap_=LoadSound((common/"skill.wav").string().c_str());impact_=LoadSound((common/"sword_rain.wav").string().c_str());
}
BossAudio::~BossAudio() {
    for(auto& layer:music_)UnloadMusicStream(layer.stream);
    if(IsSoundValid(ring_))UnloadSound(ring_);if(IsSoundValid(seal_))UnloadSound(seal_);
    if(IsSoundValid(leap_))UnloadSound(leap_);if(IsSoundValid(impact_))UnloadSound(impact_);
}
bool BossAudio::MusicReady() const {
    return std::any_of(music_.begin(),music_.end(),[&](const auto& layer){
        return layer.track==track_&&IsMusicValid(layer.stream);
    });
}
void BossAudio::SyncMusic(const std::string& desired,bool bossMusic,bool paused,float volume,float dt) {
    constexpr float fadeDuration=.8F;
    if(desired!=track_) {
        // Reuse a fading stream when returning quickly to its page; preserve its cursor and gain.
        const auto existing=std::find_if(music_.begin(),music_.end(),[&](const auto& layer){return layer.track==desired;});
        if(!desired.empty()&&existing==music_.end()) {
            const auto path=Utf8Path(GetApplicationDirectory())/(bossMusic?"assets/enemies/bosses/audio":"assets/audio/music")/(desired+".ogg");
            Music stream=LoadMusicStream(path.string().c_str());
            if(IsMusicValid(stream)) {
                stream.looping=true;SetMusicVolume(stream,0);PlayMusicStream(stream);
                music_.push_back({desired,stream,bossMusic?.42F:.25F});
            }
        }
        for(auto& layer:music_){layer.fadeFrom=layer.level;layer.fadeTime=0;}
        track_=desired;
        for(auto sound:{ring_,seal_,leap_,impact_})if(IsSoundValid(sound))StopSound(sound);
    }
    for(auto& layer:music_) {
        const bool current=layer.track==track_;
        if(!paused) {
            layer.fadeTime=std::min(fadeDuration,layer.fadeTime+std::clamp(dt,0.0F,.25F));
            const float t=layer.fadeTime/fadeDuration;
            const float smooth=t*t*(3-2*t);
            layer.level=layer.fadeFrom+((current?1.0F:0.0F)-layer.fadeFrom)*smooth;
        }
        SetMusicVolume(layer.stream,volume*layer.gain*layer.level);
        if(paused&&!layer.paused)PauseMusicStream(layer.stream);
        else if(!paused&&layer.paused)ResumeMusicStream(layer.stream);
        if(!paused)UpdateMusicStream(layer.stream);
        layer.paused=paused;
    }
    std::erase_if(music_,[&](auto& layer){
        if(layer.track==track_||layer.level>0)return false;
        StopMusicStream(layer.stream);UnloadMusicStream(layer.stream);return true;
    });
}
void BossAudio::Sync(const StoryBoss* boss,bool paused,float volume,const char* background,float dt) {
    volume=std::clamp(volume,0.0F,1.0F);
    const bool active=boss&&boss->TwoLives()&&boss->Active();
    const std::string desired=!active?background:boss->Kind()==StoryBossKind::Patriot?"patriot":boss->PhaseTwo()?"frost_phase2":"frost_phase1";
    SyncMusic(desired,active,paused,volume,dt);
    if(IsSoundValid(ring_))SetSoundVolume(ring_,volume*.35F);
    if(IsSoundValid(seal_))SetSoundVolume(seal_,volume*.35F);
    if(IsSoundValid(leap_))SetSoundVolume(leap_,volume*.30F);
    if(IsSoundValid(impact_))SetSoundVolume(impact_,volume*.32F);
    if(!active){
        for(auto sound:{ring_,seal_,leap_,impact_})if(IsSoundValid(sound))StopSound(sound);
        encounter_=nullptr;kind_=StoryBossKind::None;revision_=0;return;
    }
    if(encounter_!=boss||kind_!=boss->Kind()){encounter_=boss;kind_=boss->Kind();revision_=boss->ActionRevision();}
    if(revision_!=boss->ActionRevision()) {
        revision_=boss->ActionRevision();
        if(!paused&&volume>0&&boss->Kind()==StoryBossKind::FrostNova) {
            if(boss->State()==StoryBossState::IceRing&&IsSoundValid(ring_))PlaySound(ring_);
            if((boss->State()==StoryBossState::BlackIce||boss->State()==StoryBossState::Revive)&&IsSoundValid(seal_))PlaySound(seal_);
            if(boss->State()==StoryBossState::FrostStep&&IsSoundValid(seal_))PlaySound(seal_);
        }
        if(!paused&&volume>0&&boss->Kind()==StoryBossKind::Patriot) {
            if(boss->State()==StoryBossState::LeapWindup&&IsSoundValid(leap_))PlaySound(leap_);
            if(boss->State()==StoryBossState::Recover&&boss->RecoveryFrom()==StoryBossState::Leap&&IsSoundValid(impact_))PlaySound(impact_);
        }
    }
    if(!active||paused||volume<=0){for(auto sound:{ring_,seal_,leap_,impact_})if(IsSoundValid(sound))StopSound(sound);}
}
