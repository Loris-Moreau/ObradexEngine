#pragma once

// ============================================================
//  Renderer.h  —  Main OpenGL Renderer
// ============================================================
//  Coordinates the 3-D rendering pipeline:
//
//    BeginFrame()        — clear buffers, begin low-res capture
//    RenderWorld(...)    — draw all entities in the world
//    ApplyPostProcess()  — dither + palette pass
//    Present(w, h)       — blit to window
//
//  Rendering is deferred-ish: opaque objects first, then a
//  simple forward pass for transparent/emissive geometry.
//  No full deferred shading — the low-res aesthetic means we
//  can afford a simpler forward renderer.
//
//  Lighting model:
//    - One directional "moon" light (cool blue, Obra Dinn mood)
//    - Up to 8 point lights per frame (lanterns, fires, etc.)
//    - Ambient term for fill
// ============================================================

#include <memory>
#include <glm/glm.hpp>

class World;
class Camera;
class Shader;
class PostProcess;

// ── Point light POD ──────────────────────────────────────────
struct PointLight
{
    glm::vec3 position  = {0.f, 1.f, 0.f};
    glm::vec3 colour    = {1.f, 0.9f, 0.6f};  // Warm candlelight default
    float     radius    = 5.f;                  // Attenuation radius (m)
    float     intensity = 1.f;
};

// ── Renderer ─────────────────────────────────────────────────
class Renderer
{
public:
    Renderer()  = default;
    ~Renderer() = default;

    // ── Lifecycle ─────────────────────────────────────────────

    /// Initialise OpenGL state and compile shaders.
    /// @param renderW / renderH  Low-res internal resolution.
    bool Init(int renderW, int renderH);

    // ── Per-frame pipeline ────────────────────────────────────
    void BeginFrame();
    void RenderWorld(const World& world, const Camera& camera);
    void ApplyPostProcess();
    void Present(int windowW, int windowH);

    // ── Light management ─────────────────────────────────────
    void  ClearLights();
    void  AddPointLight(const PointLight& light);

    /// Directional light direction (world space, normalised).
    void  SetSunDirection(const glm::vec3& dir);
    void  SetSunColour   (const glm::vec3& col);
    void  SetAmbient     (const glm::vec3& col);

    // ── Accessors ─────────────────────────────────────────────
    PostProcess& GetPostProcess() { return *m_postProcess; }
    int RenderWidth()  const { return m_renderW; }
    int RenderHeight() const { return m_renderH; }

private:
    /// Upload all light data to the world shader uniforms.
    void UploadLights(Shader& shader) const;

    int m_renderW = 320;
    int m_renderH = 180;

    std::unique_ptr<Shader>      m_worldShader;
    std::unique_ptr<PostProcess> m_postProcess;

    // Lighting state
    static constexpr int kMaxPointLights = 8;
    PointLight  m_pointLights[kMaxPointLights];
    int         m_pointLightCount = 0;

    glm::vec3   m_sunDir     = glm::normalize(glm::vec3(-0.4f, -0.8f, -0.3f));
    glm::vec3   m_sunColour  = {0.7f, 0.75f, 0.85f};  // Cool moonlight
    glm::vec3   m_ambient    = {0.05f, 0.05f, 0.08f}; // Near-black ambient
};
