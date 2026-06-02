#pragma once

// Shader.h - GLSL shader program wrapper.
//
// Compiles a vertex and fragment stage, links them, and provides typed
// uniform setters. Move-only; no copy (GPU resource ownership).
//
// Usage:
//   Shader sh;
//   sh.Load("world.vert", "world.frag");
//   sh.Bind();
//   sh.SetMat4("u_MVP", mvp);

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader()  = default;
    ~Shader();

    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;

    // Compile and link from file paths.
    bool Load(const std::string& vertPath, const std::string& fragPath);

    // Compile and link from in-memory source strings (used for embedded shaders).
    bool LoadFromSource(const char* vertSrc, const char* fragSrc);

    void Bind()   const;
    void Unbind() const;
    unsigned int GetID() const { return m_id; }

    void SetInt  (const std::string& name, int v)            const;
    void SetFloat(const std::string& name, float v)          const;
    void SetVec2 (const std::string& name, const glm::vec2&) const;
    void SetVec3 (const std::string& name, const glm::vec3&) const;
    void SetVec4 (const std::string& name, const glm::vec4&) const;
    void SetMat3 (const std::string& name, const glm::mat3&) const;
    void SetMat4 (const std::string& name, const glm::mat4&) const;

private:
    // Compile one shader stage. Returns the GL object ID, or 0 on failure.
    static unsigned int CompileStage(const char* src, unsigned int type);

    int GetLocation(const std::string& name) const;

    unsigned int m_id = 0;
};
