#pragma once

// AudioSystem.h - miniaudio-backed audio system.
//
// Handles SFX, music, and ambience via three named channel types.
// All sounds are keyed by a string ID assigned at LoadSound time.
//
// Channel types:
//   SFX       - one-shot sounds (footsteps, interact, pickup)
//   Music     - looped, only one track plays at a time; crossfade on switch
//   Ambience  - looped background layer, independent of music
//
// Sound asset layout:
//   assets/sounds/effects/   SFX files
//   assets/sounds/music/     Music and ambience files
//
// miniaudio integration:
//   1. Download miniaudio.h into third_party/miniaudio/
//   2. In one .cpp: #define MINIAUDIO_IMPLEMENTATION then #include "miniaudio.h"
//   3. Flip AUDIO_ENABLED below to 1 and rebuild.
//   All call sites are already wired; no other files need to change.

#define AUDIO_ENABLED 0  // Set to 1 after placing miniaudio.h

#include <string>
#include <unordered_map>

#if AUDIO_ENABLED
  #include "miniaudio.h"
#endif

// Per-interactable sound set.  Any field may be empty (no sound plays).
struct SoundSet
{
    std::string onOpen;   // Container search, door open, pickup grab
    std::string onClose;  // Door close
    std::string onBreak;  // Crate destroyed
    std::string onAlarm;  // Alarm triggered
    std::string onDefuse; // Alarm defused
};

class AudioSystem
{
public:
    bool Init();
    void Shutdown();

    // Register a sound file under an ID.  Safe to call multiple times for the
    // same ID (second call is ignored).  Path is relative to working directory.
    bool LoadSound(const std::string& id, const std::string& path);

    // Play a one-shot SFX at the given volume (0..1).
    void Play(const std::string& id, float volume = 1.f);

    // Start looping music.  Stops the previous music track first.
    void PlayMusic(const std::string& id, float volume = 1.f);

    // Start looping ambience independently of the music channel.
    void PlayAmbience(const std::string& id, float volume = 1.f);

    void StopMusic();
    void StopAmbience();
    void Stop(const std::string& id);

    // Convenience: play a field from a SoundSet if it is non-empty.
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
    std::unordered_map<std::string, bool> m_sounds;  // stub placeholder
#endif
};
