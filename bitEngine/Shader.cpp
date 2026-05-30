// ============================================================
//  Shader.cpp  -  GLSL Shader Program Wrapper
// ============================================================

#include "Shader.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>

// ── Destructor / Move ─────────────────────────────────────────
Shader::~Shader()
{
    if (m_id) { glDeleteProgram(m_id); m_id = 0; }
}

Shader::Shader(Shader&& o) noexcept : m_id(o.m_id) { o.m_id = 0; }
Shader& Shader::operator=(Shader&& o) noexcept
{
    if (this != &o) { if (m_id) glDeleteProgram(m_id); m_id = o.m_id; o.m_id = 0; }
    return *this;
}

// ── Load from file ────────────────────────────────────────────
bool Shader::Load(const std::string& vertPath, const std::string& fragPath)
{
    auto readFile = [](const std::string& path) -> std::string
    {
        std::ifstream f(path);
        if (!f.is_open())
        {
            std::cerr << "[Shader] Cannot open: " << path << "\n";
            return {};
        }
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    };

    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);
    if (vertSrc.empty() || fragSrc.empty()) return false;

    return LoadFromSource(vertSrc.c_str(), fragSrc.c_str());
}

// ── Compile & link from source strings ───────────────────────
bool Shader::LoadFromSource(const char* vertSrc, const char* fragSrc)
{
    unsigned int vert = CompileStage(vertSrc, GL_VERTEX_SHADER);
    unsigned int frag = CompileStage(fragSrc, GL_FRAGMENT_SHADER);
    if (!vert || !frag) return false;

    // Link the program
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    // Check link status
    int success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(prog, sizeof(log), nullptr, log);
        std::cerr << "[Shader] Link error:\n" << log << "\n";
        glDeleteProgram(prog);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return false;
    }

    // Detach & delete stage objects (program keeps a reference)
    glDetachShader(prog, vert);
    glDetachShader(prog, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    if (m_id) glDeleteProgram(m_id);
    m_id = prog;
    return true;
}

// ── CompileStage ──────────────────────────────────────────────
unsigned int Shader::CompileStage(const char* src, unsigned int type)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        std::cerr << "[Shader] Compile error (" << typeName << "):\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// ── Bind / Unbind ─────────────────────────────────────────────
void Shader::Bind()   const { glUseProgram(m_id); }
void Shader::Unbind() const { glUseProgram(0);    }

// ── Uniform helper ────────────────────────────────────────────
int Shader::GetLocation(const std::string& name) const
{
    return glGetUniformLocation(m_id, name.c_str());
}

// ── Uniform setters ───────────────────────────────────────────
void Shader::SetInt  (const std::string& n, int v)            const { glUniform1i (GetLocation(n), v);             }
void Shader::SetFloat(const std::string& n, float v)          const { glUniform1f (GetLocation(n), v);             }
void Shader::SetVec2 (const std::string& n, const glm::vec2& v) const { glUniform2fv(GetLocation(n), 1, glm::value_ptr(v)); }
void Shader::SetVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(GetLocation(n), 1, glm::value_ptr(v)); }
void Shader::SetVec4 (const std::string& n, const glm::vec4& v) const { glUniform4fv(GetLocation(n), 1, glm::value_ptr(v)); }
void Shader::SetMat3 (const std::string& n, const glm::mat3& v) const { glUniformMatrix3fv(GetLocation(n), 1, GL_FALSE, glm::value_ptr(v)); }
void Shader::SetMat4 (const std::string& n, const glm::mat4& v) const { glUniformMatrix4fv(GetLocation(n), 1, GL_FALSE, glm::value_ptr(v)); }
