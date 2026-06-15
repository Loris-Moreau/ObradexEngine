#pragma once

// TextureManager.h - OpenGL texture loader and cache.
// All textures are loaded via stb_image and cached by file path.
// Returns 0 on failure (white 1x1 fallback is used by the renderer).

#include <string>
#include <unordered_map>

class TextureManager
{
public:
    static TextureManager& Get();

    TextureManager(const TextureManager&)            = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    // Load from path; returns cached handle if already loaded.
    unsigned int Load(const std::string& path);

    // Release all GPU textures.
    void Shutdown();

    // Returns the 1x1 white fallback texture (always valid after Init).
    unsigned int GetWhite() const { return m_whiteTexture; }

    void Init();

private:
    TextureManager() = default;

    unsigned int m_whiteTexture = 0;
    std::unordered_map<std::string, unsigned int> m_cache;
};
