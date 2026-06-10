// AudioSystem.cpp - No-op stub. Replace bodies with ma_engine calls.
#include "AudioSystem.h"
#include <iostream>
bool  AudioSystem::Init()    { std::cout<<"[Audio] Stub OK\n"; return true; }
void  AudioSystem::Shutdown(){}
bool  AudioSystem::LoadSound(const std::string& id,const std::string& fp)
      { m_sounds[id]=true; (void)fp; return true; }
void  AudioSystem::Play    (const std::string& id){ (void)id; }
void  AudioSystem::PlayLoop(const std::string& id){ (void)id; }
void  AudioSystem::Stop    (const std::string& id){ (void)id; }
void  AudioSystem::SetMasterVolume(float v){ m_vol=v; }
