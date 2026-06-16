#pragma once

// AudioSystem.h - miniaudio-backed audio system.
//
// Three channel types:
//   SFX      - one-shot sounds
//   Music    - looped, one track at a time
//   Ambience - looped background, independent of music
//
// Asset layout:
//   assets/sounds/effects/   one-shot SFX
//   assets/sounds/music/     music and ambience loops
//
// To activate: miniaudio.h is already in third_party/miniaudio/.
// AUDIO_ENABLED is set to 1 below. miniaudio_impl.cpp carries the implementation.

#define AUDIO_ENABLED 1

#if AUDIO_ENABLED
  // miniaudio.h lives in third_party/miniaudio/ and is compiled via
  // miniaudio_impl.cpp (one .cpp with MINIAUDIO_IMPLEMENTATION defined).
  #include "../../third_party/miniaudio/miniaudio.h"
#endif

#include <string>
#include <unordered_map>

struct SoundSet
{
    std::string onOpen;
    std::string onClose;
    std::string onBreak;
    std::string onAlarm;
    std::string onDefuse;
};

class AudioSystem
{
public:
    bool Init();
    void Shutdown();

    bool LoadSound(const std::string& id, const std::string& path);

    void Play       (const std::string& id, float volume = 1.f);
    void PlayMusic  (const std::string& id, float volume = 1.f);
    void PlayAmbience(const std::string& id, float volume = 1.f);
    void StopMusic();
    void StopAmbience();
    void Stop(const std::string& id);

    void PlayFromSet(const SoundSet& set, const std::string& SoundSet::* field,
                     float volume = 1.f);

    void  SetMasterVolume(float v);
    void  SetMusicVolume (float v);
    float GetMasterVolume() const { return m_masterVolume; }
    float GetMusicVolume()  const { return m_musicVolume;  }

private:
    float m_masterVolume = 1.f;
    float m_musicVolume  = 0.8f;
    bool  m_initialised  = false;

    std::string m_currentMusic;
    std::string m_currentAmbience;

#if AUDIO_ENABLED
    ma_engine m_engine;
    std::unordered_map<std::string, ma_sound*> m_sounds;
#else
    std::unordered_map<std::string, bool> m_sounds;
#endif
};
