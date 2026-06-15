// TextureManager.cpp - stb_image texture loader with GPU cache.

#include "TextureManager.h"
#include <glad/glad.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

TextureManager& TextureManager::Get()
{
    static TextureManager instance;
    return instance;
}

void TextureManager::Init()
{
    // 1x1 opaque white texture used as a no-texture fallback.
    unsigned char white[4] = {255, 255, 255, 255};
    glGenTextures(1, &m_whiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_whiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

unsigned int TextureManager::Load(const std::string& path)
{
    auto it = m_cache.find(path);
    if (it != m_cache.end()) return it->second;

    stbi_set_flip_vertically_on_load(true);

    int w, h, channels;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!data)
    {
        std::cerr << "[Texture] Failed to load: " << path << "\n";
        return m_whiteTexture;
    }

    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    // Nearest-neighbour min, linear mag gives sharp pixels that blend well
    // at close range - correct for the dithered pixel-art aesthetic.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);
    m_cache[path] = id;
    std::cout << "[Texture] Loaded " << path << " (" << w << "x" << h << ")\n";
    return id;
}

void TextureManager::Shutdown()
{
    for (auto& [path, id] : m_cache)
        glDeleteTextures(1, &id);
    m_cache.clear();

    if (m_whiteTexture)
    {
        glDeleteTextures(1, &m_whiteTexture);
        m_whiteTexture = 0;
    }
}
