#pragma once

// Mesh.h - GPU mesh (VAO/VBO/EBO).
//
// Stores interleaved vertex data on the GPU and issues indexed draw calls.
// Vertex layout (32 bytes per vertex):
//   Position  vec3  12 bytes
//   Normal    vec3  12 bytes
//   TexCoord  vec2   8 bytes
//
// Static geometry only (GL_STATIC_DRAW).

#include <vector>
#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh
{
public:
    Mesh()  = default;
    ~Mesh();

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& o) noexcept;
    Mesh& operator=(Mesh&& o) noexcept;

    // Upload vertex and index data to the GPU.
    void Upload(const std::vector<Vertex>&   vertices,
                const std::vector<unsigned>& indices);

    // Issue glDrawElements. The caller must bind a shader first.
    void Draw() const;

    static Mesh MakeCube();
    static Mesh MakePlane(float size = 1.f, int subdivisions = 1);
    static Mesh MakeSphere(float radius = 1.f, int segments = 16);

    unsigned GetIndexCount() const { return m_indexCount; }
    bool     IsValid()       const { return m_vao != 0;   }

private:
    void Destroy();

    unsigned m_vao        = 0;
    unsigned m_vbo        = 0;
    unsigned m_ebo        = 0;
    unsigned m_indexCount = 0;
};
