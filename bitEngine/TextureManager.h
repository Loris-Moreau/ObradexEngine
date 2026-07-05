#pragma once

// TextureManager.h - stb_image texture loader with GPU cache.
// All textures keyed by file path. Returns white 1x1 fallback on failure.

#include <string>
#include <unordered_map>

class TextureManager
{
public:
    static TextureManager& Get();
    TextureManager(const TextureManager&)            = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    void         Init();
    unsigned int Load(const std::string& path);
    void         Shutdown();
    unsigned int GetWhite() const { return m_white; }

    // Native pixel dimensions of a loaded texture; {0,0} if not loaded.
    // Used by UVMode::Fit to correct aspect ratio.
    struct Size { int w = 0; int h = 0; };
    Size GetSize(const std::string& path) const;

private:
    TextureManager() = default;
    unsigned int m_white = 0;
    std::unordered_map<std::string, unsigned int> m_cache;
    std::unordered_map<std::string, Size>         m_sizes;
};
