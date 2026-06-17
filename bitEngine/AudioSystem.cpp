// AudioSystem.cpp - miniaudio implementation.

#include "AudioSystem.h"
#include <iostream>

bool AudioSystem::Init()
{
#if AUDIO_ENABLED
    ma_result r = ma_engine_init(nullptr, &m_engine);
    if (r != MA_SUCCESS)
    {
        std::cerr << "[Audio] ma_engine_init failed: " << r << "\n";
        return false;
    }
    ma_engine_set_volume(&m_engine, m_masterVolume);
    m_initialised = true;
    std::cout << "[Audio] miniaudio engine initialised.\n";
#else
    m_initialised = true;
    std::cout << "[Audio] Stub audio (AUDIO_ENABLED=0).\n";
#endif
    return true;
}

void AudioSystem::Shutdown()
{
#if AUDIO_ENABLED
    for (auto& [id, snd] : m_sounds)
    { ma_sound_uninit(snd); delete snd; }
    m_sounds.clear();
    ma_engine_uninit(&m_engine);
#endif
    m_initialised = false;
}

bool AudioSystem::LoadSound(const std::string& id, const std::string& path)
{
    if (m_sounds.count(id)) return true;
#if AUDIO_ENABLED
    auto* snd = new ma_sound();
    ma_result r = ma_sound_init_from_file(&m_engine, path.c_str(),
                                          MA_SOUND_FLAG_DECODE,
                                          nullptr, nullptr, snd);
    if (r != MA_SUCCESS)
    {
        std::cerr << "[Audio] Failed to load: " << path << "\n";
        delete snd;
        return false;
    }
    m_sounds[id] = snd;
#else
    m_sounds[id] = true;
    (void)path;
#endif
    return true;
}

void AudioSystem::Play(const std::string& id, float volume)
{
    if (!m_initialised) return;
#if AUDIO_ENABLED
    auto it = m_sounds.find(id);
    if (it == m_sounds.end()) return;
    ma_sound_set_volume(it->second, volume * m_masterVolume);
    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
#else
    (void)id; (void)volume;
#endif
}

void AudioSystem::PlayMusic(const std::string& id, float volume)
{
    if (!m_initialised) return;
    StopMusic();
    m_currentMusic = id;
#if AUDIO_ENABLED
    auto it = m_sounds.find(id);
    if (it == m_sounds.end()) return;
    ma_sound_set_volume(it->second, volume * m_masterVolume * m_musicVolume);
    ma_sound_set_looping(it->second, MA_TRUE);
    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
#else
    (void)volume;
#endif
}

void AudioSystem::PlayAmbience(const std::string& id, float volume)
{
    if (!m_initialised) return;
    StopAmbience();
    m_currentAmbience = id;
#if AUDIO_ENABLED
    auto it = m_sounds.find(id);
    if (it == m_sounds.end()) return;
    ma_sound_set_volume(it->second, volume * m_masterVolume);
    ma_sound_set_looping(it->second, MA_TRUE);
    ma_sound_seek_to_pcm_frame(it->second, 0);
    ma_sound_start(it->second);
#else
    (void)volume;
#endif
}

void AudioSystem::StopMusic()
{
#if AUDIO_ENABLED
    if (!m_currentMusic.empty())
    {
        auto it = m_sounds.find(m_currentMusic);
        if (it != m_sounds.end()) ma_sound_stop(it->second);
    }
#endif
    m_currentMusic.clear();
}

void AudioSystem::StopAmbience()
{
#if AUDIO_ENABLED
    if (!m_currentAmbience.empty())
    {
        auto it = m_sounds.find(m_currentAmbience);
        if (it != m_sounds.end()) ma_sound_stop(it->second);
    }
#endif
    m_currentAmbience.clear();
}

void AudioSystem::Stop(const std::string& id)
{
#if AUDIO_ENABLED
    auto it = m_sounds.find(id);
    if (it != m_sounds.end()) ma_sound_stop(it->second);
#else
    (void)id;
#endif
}

void AudioSystem::PlayFromSet(const SoundSet& set,
                               std::string SoundSet::* field,
                               float volume)
{
    const std::string& id = set.*field;
    if (!id.empty()) Play(id, volume);
}

void AudioSystem::SetMasterVolume(float v)
{
    m_masterVolume = v;
#if AUDIO_ENABLED
    if (m_initialised) ma_engine_set_volume(&m_engine, v);
#endif
}

void AudioSystem::SetMusicVolume(float v)
{
    m_musicVolume = v;
#if AUDIO_ENABLED
    if (!m_currentMusic.empty())
    {
        auto it = m_sounds.find(m_currentMusic);
        if (it != m_sounds.end())
            ma_sound_set_volume(it->second, v * m_masterVolume);
    }
#endif
}
