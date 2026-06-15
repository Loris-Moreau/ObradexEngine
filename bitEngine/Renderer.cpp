// Renderer.cpp - Main OpenGL renderer.

#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "PostProcess.h"
#include "World.h"
#include "TextureManager.h"
#include "Billboard.h"
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

    RenderBillboards(world, camera);
}

void Renderer::ApplyPostProcess()
{
    m_postProcess->EndCapture();
}

void Renderer::Present(int windowW, int windowH)
{
    m_postProcess->Apply(windowW, windowH);
}


void Renderer::RenderBillboards(const World& world, const Camera& camera)
{
    // Billboard pass: depth-test on, depth-write off, alpha blend.
    // Quads face the camera by decomposing the view matrix rotation.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glm::mat4 view = camera.GetView();
    // Camera right and up extracted from the view matrix rows.
    glm::vec3 camRight = { view[0][0], view[1][0], view[2][0] };
    glm::vec3 camUp    = { view[0][1], view[1][1], view[2][1] };

    m_worldShader->Bind();

    for (const auto& rec : world.GetAllRecords())
    {
        if (!rec.active || !rec.billboard || !rec.transform) continue;
        const BillboardComponent& bb = *rec.billboard;
        const glm::vec3& pos = rec.transform->position;

        // Build a billboard model matrix: scale by size, orient toward camera.
        glm::mat4 model(1.f);
        model[0] = glm::vec4(camRight * bb.size.x, 0.f);
        model[1] = glm::vec4(camUp    * bb.size.y, 0.f);
        model[2] = glm::vec4(glm::cross(camRight, camUp), 0.f);
        model[3] = glm::vec4(pos, 1.f);

        m_worldShader->SetMat4("u_Model",       model);
        m_worldShader->SetVec3("u_AlbedoColour",{bb.tint.r, bb.tint.g, bb.tint.b});
        m_worldShader->SetFloat("u_Specular",   0.f);
        m_worldShader->SetFloat("u_Roughness",  1.f);
        m_worldShader->SetInt  ("u_HasTexture", bb.textureID != 0 ? 1 : 0);

        unsigned int tex = bb.textureID != 0
            ? bb.textureID
            : TextureManager::Get().GetWhite();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        m_worldShader->SetInt("u_AlbedoTex", 0);

        // Draw a single unit quad - same VAO as the fullscreen quad would be,
        // but we use the plane mesh from the world instead.
        // Billboard mesh must be set up as a unit quad centred at origin.
        const_cast<World&>(world).GetPlaneMesh()->Draw();
    }

    m_worldShader->Unbind();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
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
