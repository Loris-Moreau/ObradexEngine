// ============================================================
//  Renderer.cpp  —  Main OpenGL Renderer
// ============================================================

#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "PostProcess.h"
#include "world/World.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

// ── Init ──────────────────────────────────────────────────────
bool Renderer::Init(int renderW, int renderH)
{
    m_renderW = renderW;
    m_renderH = renderH;

    // ── OpenGL global state ───────────────────────────────────
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // ── World shader ──────────────────────────────────────────
    m_worldShader = std::make_unique<Shader>();
    if (!m_worldShader->Load("assets/shaders/world.vert",
                              "assets/shaders/world.frag"))
    {
        std::cerr << "[Renderer] Failed to load world shader.\n";
        return false;
    }

    // ── Post-process pipeline ─────────────────────────────────
    m_postProcess = std::make_unique<PostProcess>();
    if (!m_postProcess->Init(renderW, renderH))
    {
        std::cerr << "[Renderer] Post-process init failed.\n";
        return false;
    }

    std::cout << "[Renderer] Internal resolution: "
              << renderW << "×" << renderH << "\n";
    return true;
}

// ── BeginFrame ────────────────────────────────────────────────
void Renderer::BeginFrame()
{
    // Redirect draw calls to the low-res FBO
    m_postProcess->BeginCapture();
}

// ── RenderWorld ───────────────────────────────────────────────
void Renderer::RenderWorld(const World& world, const Camera& camera)
{
    float aspect = static_cast<float>(m_renderW) / static_cast<float>(m_renderH);
    glm::mat4 proj = camera.GetProjection(aspect);
    glm::mat4 view = camera.GetView();

    m_worldShader->Bind();

    // Camera / transform matrices
    m_worldShader->SetMat4("u_Proj", proj);
    m_worldShader->SetMat4("u_View", view);
    m_worldShader->SetVec3("u_CamPos", camera.GetPosition());

    // Lighting
    m_worldShader->SetVec3("u_SunDir",    m_sunDir);
    m_worldShader->SetVec3("u_SunColour", m_sunColour);
    m_worldShader->SetVec3("u_Ambient",   m_ambient);
    UploadLights(*m_worldShader);

    // Delegate to World to draw its entities
    world.Render(*m_worldShader);

    m_worldShader->Unbind();
}

// ── ApplyPostProcess ──────────────────────────────────────────
void Renderer::ApplyPostProcess()
{
    m_postProcess->EndCapture();  // Stop writing to FBO
    // The actual blit happens in Present() so window size is known
}

// ── Present ───────────────────────────────────────────────────
void Renderer::Present(int windowW, int windowH)
{
    m_postProcess->Apply(windowW, windowH);
}

// ── Light helpers ─────────────────────────────────────────────
void Renderer::ClearLights()        { m_pointLightCount = 0; }
void Renderer::SetSunDirection(const glm::vec3& d) { m_sunDir    = glm::normalize(d); }
void Renderer::SetSunColour   (const glm::vec3& c) { m_sunColour = c; }
void Renderer::SetAmbient     (const glm::vec3& c) { m_ambient   = c; }

void Renderer::AddPointLight(const PointLight& l)
{
    if (m_pointLightCount < kMaxPointLights)
        m_pointLights[m_pointLightCount++] = l;
}

// ── UploadLights ──────────────────────────────────────────────
void Renderer::UploadLights(Shader& sh) const
{
    sh.SetInt("u_PointLightCount", m_pointLightCount);
    for (int i = 0; i < m_pointLightCount; ++i)
    {
        const auto& l  = m_pointLights[i];
        std::string pi = "u_PointLights[" + std::to_string(i) + "].";
        sh.SetVec3 (pi + "position",  l.position);
        sh.SetVec3 (pi + "colour",    l.colour);
        sh.SetFloat(pi + "radius",    l.radius);
        sh.SetFloat(pi + "intensity", l.intensity);
    }
}
