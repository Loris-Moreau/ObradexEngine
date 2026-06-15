#pragma once

// Renderer.h - Main OpenGL renderer.
//
// Pipeline per frame:
//   BeginFrame()       - bind low-res FBO
//   RenderWorld()      - draw all scene entities with Blinn-Phong lighting
//   ApplyPostProcess() - stop FBO capture
//   Present()          - dither/palette blit to window with letterboxing
//
// Point lights are fed from World LightComponent entities by Engine::Render
// each frame via ClearLights() + AddPointLight(). Up to 8 per frame.

#include <memory>
#include <glm/glm.hpp>
#include "Shader.h"
#include "PostProcess.h"

class World;
class Camera;

struct PointLight
{
    glm::vec3 position  = {0.f, 1.f, 0.f};
    glm::vec3 colour    = {1.f, 0.9f, 0.6f};
    float     radius    = 5.f;
    float     intensity = 1.f;
};

class Renderer
{
public:
    Renderer()  = default;
    ~Renderer() = default;

    bool Init(int renderW, int renderH);

    void BeginFrame();
    void RenderBillboards(const World& world, const Camera& camera);
    void RenderWorld(const World& world, const Camera& camera);
    void ApplyPostProcess();
    void Present(int windowW, int windowH);

    void ClearLights();
    void AddPointLight(const PointLight& light);
    void SetSunDirection(const glm::vec3& dir);
    void SetSunColour   (const glm::vec3& col);
    void SetAmbient     (const glm::vec3& col);
    void SetFogDensity  (float density);
    void SetFogColour   (const glm::vec3& col);

    PostProcess& GetPostProcess() { return *m_postProcess; }
    int RenderWidth()  const { return m_renderW; }
    int RenderHeight() const { return m_renderH; }

private:
    void UploadLights(Shader& shader) const;

    int m_renderW = 320;
    int m_renderH = 180;

    std::unique_ptr<Shader>      m_worldShader;
    std::unique_ptr<PostProcess> m_postProcess;

    static constexpr int kMaxPointLights = 8;
    PointLight m_pointLights[kMaxPointLights];
    int        m_pointLightCount = 0;

    glm::vec3 m_sunDir     = glm::normalize(glm::vec3(-0.4f, -0.8f, -0.3f));
    glm::vec3 m_sunColour  = {0.7f, 0.75f, 0.85f};
    glm::vec3 m_ambient    = {0.05f, 0.05f, 0.08f};
    float     m_fogDensity = 0.f;
    glm::vec3 m_fogColour  = {0.04f, 0.04f, 0.06f};
};
