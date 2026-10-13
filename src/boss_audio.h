#pragma once
#include "story_boss.h"
#include <string>
#include <vector>
class BossAudio {
public:
    BossAudio();
    ~BossAudio();
    BossAudio(const BossAudio&)=delete;
    BossAudio& operator=(const BossAudio&)=delete;
    void Sync(const StoryBoss* boss,bool paused,float volume,const char* background="",float dt=1.0F/60);
    bool Ready() const {return IsSoundValid(ring_)&&IsSoundValid(seal_);}
    bool MusicReady() const;
    const std::string& CurrentTrack() const {return track_;}
private:
    friend struct BossAudioTestAccess;
    struct MusicLayer {
        std::string track;
        Music stream{};
        float gain=.25F,level=0,fadeFrom=0,fadeTime=0;
        bool paused=false;
    };
    void SyncMusic(const std::string& desired,bool bossMusic,bool paused,float volume,float dt);
    std::vector<MusicLayer> music_;
    Sound ring_{},seal_{},leap_{},impact_{};
    std::string track_;
    unsigned revision_=0;
    const StoryBoss* encounter_=nullptr;
    StoryBossKind kind_=StoryBossKind::None;
};
