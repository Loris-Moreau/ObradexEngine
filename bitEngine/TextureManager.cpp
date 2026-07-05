// TextureManager.cpp

#include "TextureManager.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

TextureManager& TextureManager::Get()
{
    static TextureManager inst;
    return inst;
}

void TextureManager::Init()
{
    unsigned char white[4] = {255,255,255,255};
    glGenTextures(1, &m_white);
    glBindTexture(GL_TEXTURE_2D, m_white);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,white);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
}

unsigned int TextureManager::Load(const std::string& path)
{
    auto it = m_cache.find(path);
    if (it != m_cache.end()) return it->second;

    stbi_set_flip_vertically_on_load(true);
    int w,h,ch;
    unsigned char* data = stbi_load(path.c_str(),&w,&h,&ch,4);
    if (!data)
    {
        std::cerr << "[Texture] Failed: " << path << "\n";
        return m_white;
    }

    unsigned int id;
    glGenTextures(1,&id);
    glBindTexture(GL_TEXTURE_2D,id);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    stbi_image_free(data);
    m_cache[path] = id;
    m_sizes[path] = {w, h};
    std::cout << "[Texture] " << path << " (" << w << "x" << h << ")\n";
    return id;
}

TextureManager::Size TextureManager::GetSize(const std::string& path) const
{
    auto it = m_sizes.find(path);
    return it != m_sizes.end() ? it->second : Size{};
}

void TextureManager::Shutdown()
{
    for (auto&[p,id]:m_cache) glDeleteTextures(1,&id);
    m_cache.clear();
    m_sizes.clear();
    if (m_white) { glDeleteTextures(1,&m_white); m_white=0; }
}
