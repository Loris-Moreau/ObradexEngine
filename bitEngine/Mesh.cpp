// Mesh.cpp - GPU mesh (VAO/VBO/EBO) implementation.

#include "Mesh.h"
#include <glad/glad.h>
#include <cmath>

Mesh::~Mesh() { Destroy(); }

Mesh::Mesh(Mesh&& o) noexcept
    : m_vao(o.m_vao), m_vbo(o.m_vbo), m_ebo(o.m_ebo), m_indexCount(o.m_indexCount)
{
    o.m_vao = o.m_vbo = o.m_ebo = o.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept
{
    if (this != &o)
    {
        Destroy();
        m_vao = o.m_vao; m_vbo = o.m_vbo;
        m_ebo = o.m_ebo; m_indexCount = o.m_indexCount;
        o.m_vao = o.m_vbo = o.m_ebo = o.m_indexCount = 0;
    }
    return *this;
}

void Mesh::Destroy()
{
    if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    m_indexCount = 0;
}

void Mesh::Upload(const std::vector<Vertex>&   vertices,
                  const std::vector<unsigned>& indices)
{
    if (m_vao) Destroy();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned)),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Location 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, position)));

    // Location 1: normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));

    // Location 2: texcoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

    glBindVertexArray(0);

    m_indexCount = static_cast<unsigned>(indices.size());
}

void Mesh::Draw() const
{
    if (!m_vao) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

Mesh Mesh::MakeCube()
{
    // 6 faces, 4 unique vertices each. Vertices are duplicated at edges so
    // each face has its own flat normal.
    const float h = 0.5f;
    std::vector<Vertex> verts = {
        {{-h,-h, h},{0,0,1},{0,0}}, {{ h,-h, h},{0,0,1},{1,0}},
        {{ h, h, h},{0,0,1},{1,1}}, {{-h, h, h},{0,0,1},{0,1}},
        {{ h,-h,-h},{0,0,-1},{0,0}}, {{-h,-h,-h},{0,0,-1},{1,0}},
        {{-h, h,-h},{0,0,-1},{1,1}}, {{ h, h,-h},{0,0,-1},{0,1}},
        {{ h,-h, h},{1,0,0},{0,0}}, {{ h,-h,-h},{1,0,0},{1,0}},
        {{ h, h,-h},{1,0,0},{1,1}}, {{ h, h, h},{1,0,0},{0,1}},
        {{-h,-h,-h},{-1,0,0},{0,0}}, {{-h,-h, h},{-1,0,0},{1,0}},
        {{-h, h, h},{-1,0,0},{1,1}}, {{-h, h,-h},{-1,0,0},{0,1}},
        {{-h, h, h},{0,1,0},{0,0}}, {{ h, h, h},{0,1,0},{1,0}},
        {{ h, h,-h},{0,1,0},{1,1}}, {{-h, h,-h},{0,1,0},{0,1}},
        {{-h,-h,-h},{0,-1,0},{0,0}}, {{ h,-h,-h},{0,-1,0},{1,0}},
        {{ h,-h, h},{0,-1,0},{1,1}}, {{-h,-h, h},{0,-1,0},{0,1}},
    };

    std::vector<unsigned> idx;
    idx.reserve(36);
    for (unsigned f = 0; f < 6; ++f)
    {
        unsigned b = f * 4;
        idx.insert(idx.end(), {b,b+1,b+2, b,b+2,b+3});
    }

    Mesh m;
    m.Upload(verts, idx);
    return m;
}

Mesh Mesh::MakePlane(float size, int subdivisions)
{
    std::vector<Vertex>   verts;
    std::vector<unsigned> idx;

    int   n      = subdivisions + 1;
    float step   = size / static_cast<float>(subdivisions);
    float uvStep = 1.f  / static_cast<float>(subdivisions);

    for (int z = 0; z < n; ++z)
    for (int x = 0; x < n; ++x)
    {
        Vertex v;
        v.position = { x * step - size * 0.5f, 0.f, z * step - size * 0.5f };
        v.normal   = { 0.f, 1.f, 0.f };
        v.texCoord = { x * uvStep, z * uvStep };
        verts.push_back(v);
    }

    for (int z = 0; z < subdivisions; ++z)
    for (int x = 0; x < subdivisions; ++x)
    {
        unsigned tl = z * n + x;
        unsigned tr = tl + 1;
        unsigned bl = (z + 1) * n + x;
        unsigned br = bl + 1;
        idx.insert(idx.end(), {tl, bl, tr, tr, bl, br});
    }

    Mesh m;
    m.Upload(verts, idx);
    return m;
}

Mesh Mesh::MakeSphere(float radius, int segments)
{
    std::vector<Vertex>   verts;
    std::vector<unsigned> idx;

    for (int lat = 0; lat <= segments; ++lat)
    {
        float theta = lat * 3.14159265f / segments;
        for (int lon = 0; lon <= segments; ++lon)
        {
            float phi = lon * 2.f * 3.14159265f / segments;
            Vertex v;
            v.normal   = { std::sin(theta) * std::cos(phi),
                           std::cos(theta),
                           std::sin(theta) * std::sin(phi) };
            v.position  = v.normal * radius;
            v.texCoord  = { static_cast<float>(lon) / segments,
                            static_cast<float>(lat) / segments };
            verts.push_back(v);
        }
    }

    int stride = segments + 1;
    for (int lat = 0; lat < segments; ++lat)
    for (int lon = 0; lon < segments; ++lon)
    {
        unsigned tl = lat * stride + lon;
        unsigned tr = tl + 1;
        unsigned bl = (lat + 1) * stride + lon;
        unsigned br = bl + 1;
        idx.insert(idx.end(), {tl, bl, tr, tr, bl, br});
    }

    Mesh m;
    m.Upload(verts, idx);
    return m;
}
