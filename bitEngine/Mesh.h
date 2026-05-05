#pragma once

// ============================================================
//  Mesh.h  —  GPU Mesh (VAO/VBO/EBO)
// ============================================================
//  Stores geometry on the GPU as interleaved vertex data.
//  Vertex layout (each vertex = 32 bytes):
//    Position  (vec3  — 12 bytes)
//    Normal    (vec3  — 12 bytes)
//    TexCoord  (vec2  —  8 bytes)
//
//  Supports indexed drawing via an Element Buffer Object.
//  Static geometry only (GL_STATIC_DRAW); for dynamic meshes
//  (particles, debug lines) use a separate DynamicMesh class.
// ============================================================

#include <vector>
#include <glm/glm.hpp>

// ── Vertex ───────────────────────────────────────────────────
struct Vertex
{
    glm::vec3 position;  ///< Object-space position
    glm::vec3 normal;    ///< Object-space normal (normalised)
    glm::vec2 texCoord;  ///< UV coordinates [0..1]
};

// ── Mesh ─────────────────────────────────────────────────────
class Mesh
{
public:
    Mesh()  = default;
    ~Mesh();

    // No copy
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    // Move OK
    Mesh(Mesh&& o) noexcept;
    Mesh& operator=(Mesh&& o) noexcept;

    // ── Upload ────────────────────────────────────────────────

    /// Upload vertex and index data to the GPU.
    void Upload(const std::vector<Vertex>&   vertices,
                const std::vector<unsigned>& indices);

    // ── Draw ──────────────────────────────────────────────────
    /// Issue a glDrawElements call. Caller must bind a shader first.
    void Draw() const;

    // ── Primitive factories ───────────────────────────────────
    /// @{
    static Mesh MakeCube();
    static Mesh MakePlane(float size = 1.f, int subdivisions = 1);
    static Mesh MakeSphere(float radius = 1.f, int segments = 16);
    /// @}

    unsigned GetIndexCount() const { return m_indexCount; }
    bool     IsValid()       const { return m_vao != 0;   }

private:
    void Destroy();

    unsigned m_vao        = 0;
    unsigned m_vbo        = 0;
    unsigned m_ebo        = 0;
    unsigned m_indexCount = 0;
};
