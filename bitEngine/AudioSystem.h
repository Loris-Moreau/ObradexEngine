#pragma once
// AudioSystem.h - Audio stub.
// To integrate miniaudio: download miniaudio.h to third_party/miniaudio/,
// add a .cpp with #define MINIAUDIO_IMPLEMENTATION before the include,
// then replace the stub bodies in AudioSystem.cpp with ma_engine calls.
#include <string>
#include <unordered_map>
class AudioSystem {
public:
    bool  Init();
    void  Shutdown();
    bool  LoadSound(const std::string& id, const std::string& path);
    void  Play    (const std::string& id);
    void  PlayLoop(const std::string& id);
    void  Stop    (const std::string& id);
    void  SetMasterVolume(float v);
    float GetMasterVolume() const { return m_vol; }
private:
    float m_vol = 1.f;
    std::unordered_map<std::string,bool> m_sounds; // placeholder
};
