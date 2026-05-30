#pragma once

// ============================================================
//  Shader.h  -  GLSL Shader Program Wrapper
// ============================================================
//  Compiles vertex + fragment sources, links them into a
//  program, and provides type-safe uniform setters.
//
//  Usage:
//    Shader sh;
//    sh.Load("assets/shaders/world.vert", "assets/shaders/world.frag");
//    sh.Bind();
//    sh.SetMat4("u_MVP", mvp);
// ============================================================

#include <string>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader()  = default;
    ~Shader();

    // No copy (GPU resource ownership)
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    // Move allowed
    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;

    // ── Compile & link ────────────────────────────────────────

    /// Load from file paths.
    bool Load(const std::string& vertPath, const std::string& fragPath);

    /// Compile from in-memory source strings (useful for embedded shaders).
    bool LoadFromSource(const char* vertSrc, const char* fragSrc);

    // ── Binding ───────────────────────────────────────────────
    void Bind()   const;
    void Unbind() const;

    unsigned int GetID() const { return m_id; }

    // ── Uniform setters ───────────────────────────────────────
    void SetInt  (const std::string& name, int v)           const;
    void SetFloat(const std::string& name, float v)         const;
    void SetVec2 (const std::string& name, const glm::vec2&) const;
    void SetVec3 (const std::string& name, const glm::vec3&) const;
    void SetVec4 (const std::string& name, const glm::vec4&) const;
    void SetMat3 (const std::string& name, const glm::mat3&) const;
    void SetMat4 (const std::string& name, const glm::mat4&) const;

private:
    /// Compile a single shader stage.
    /// @param src   GLSL source code string.
    /// @param type  GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.
    /// @returns Shader object ID, or 0 on failure.
    static unsigned int CompileStage(const char* src, unsigned int type);

    int GetLocation(const std::string& name) const;

    unsigned int m_id = 0;  ///< OpenGL program object
};
