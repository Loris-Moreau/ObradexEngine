// Renderer.cpp - Main OpenGL renderer.

#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "PostProcess.h"
#include "World.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

bool Renderer::Init(int renderW, int renderH)
{
    m_renderW = renderW;
    m_renderH = renderH;

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    m_worldShader = std::make_unique<Shader>();
    if (!m_worldShader->Load("Shaders/world.vert", "Shaders/world.frag"))
    {
        std::cerr << "[Renderer] Failed to load world shader.\n";
        return false;
    }

    m_postProcess = std::make_unique<PostProcess>();
    if (!m_postProcess->Init(renderW, renderH))
    {
        std::cerr << "[Renderer] Post-process init failed.\n";
        return false;
    }

    std::cout << "[Renderer] Internal resolution: "
              << renderW << "x" << renderH << "\n";
    return true;
}

void Renderer::BeginFrame()
{
    m_postProcess->BeginCapture();
}

void Renderer::RenderWorld(const World& world, const Camera& camera)
{
    float aspect = static_cast<float>(m_renderW) / static_cast<float>(m_renderH);
    glm::mat4 proj = camera.GetProjection(aspect);
    glm::mat4 view = camera.GetView();

    m_worldShader->Bind();

    m_worldShader->SetMat4("u_Proj", proj);
    m_worldShader->SetMat4("u_View", view);
    m_worldShader->SetVec3("u_CamPos", camera.GetPosition());

    m_worldShader->SetVec3("u_SunDir",    m_sunDir);
    m_worldShader->SetVec3("u_SunColour", m_sunColour);
    m_worldShader->SetVec3("u_Ambient",   m_ambient);
    m_worldShader->SetFloat("u_FogDensity", m_fogDensity);
    m_worldShader->SetVec3 ("u_FogColour",  m_fogColour);
    UploadLights(*m_worldShader);

    world.Render(*m_worldShader);

    m_worldShader->Unbind();
}

void Renderer::ApplyPostProcess()
{
    m_postProcess->EndCapture();
}

void Renderer::Present(int windowW, int windowH)
{
    m_postProcess->Apply(windowW, windowH);
}

void Renderer::ClearLights()                             { m_pointLightCount = 0; }
void Renderer::SetSunDirection(const glm::vec3& d)       { m_sunDir    = glm::normalize(d); }
void Renderer::SetSunColour   (const glm::vec3& c)       { m_sunColour = c; }
void Renderer::SetAmbient     (const glm::vec3& c)       { m_ambient   = c; }
void Renderer::SetFogDensity(float d)                    { m_fogDensity = d; }
void Renderer::SetFogColour (const glm::vec3& c)         { m_fogColour  = c; }

void Renderer::AddPointLight(const PointLight& l)
{
    if (m_pointLightCount < kMaxPointLights)
        m_pointLights[m_pointLightCount++] = l;
}

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
