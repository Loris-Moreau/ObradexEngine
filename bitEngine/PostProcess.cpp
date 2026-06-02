// PostProcess.cpp - Dithering, palette, and blit pass.

#include "PostProcess.h"
#include "Shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

// The vertex and fragment shaders are embedded here so the post-process pass
// has no external file dependencies.

static const char* kPostVertSrc = R"(
#version 410 core
layout(location = 0) in vec2 a_Pos;
layout(location = 1) in vec2 a_UV;
out vec2 v_UV;
void main()
{
    v_UV        = a_UV;
    gl_Position = vec4(a_Pos, 0.0, 1.0);
}
)";

static const char* kPostFragSrc = R"(
#version 410 core
in  vec2 v_UV;
out vec4 FragColor;

uniform sampler2D u_Scene;
uniform sampler1D u_Palette;
uniform int       u_PaletteSize;
uniform float     u_Dither;
uniform float     u_Contrast;
uniform float     u_Brightness;
uniform float     u_VigRadius;
uniform float     u_VigFeather;
uniform bool      u_Scanlines;
uniform float     u_ScanAlpha;
uniform bool      u_ObraDinn;
uniform vec2      u_Resolution;

// 8x8 Bayer ordered dither matrix normalised to [0, 1].
// GLSL forbids multi-dimensional arrays, so stored as float[64] indexed by row*8+col.
float BayerMatrix(ivec2 p)
{
    const float B[64] = float[64](
         0.0,32.0, 8.0,40.0, 2.0,34.0,10.0,42.0,
        48.0,16.0,56.0,24.0,50.0,18.0,58.0,26.0,
        12.0,44.0, 4.0,36.0,14.0,46.0, 6.0,38.0,
        60.0,28.0,52.0,20.0,62.0,30.0,54.0,22.0,
         3.0,35.0,11.0,43.0, 1.0,33.0, 9.0,41.0,
        51.0,19.0,59.0,27.0,49.0,17.0,57.0,25.0,
        15.0,47.0, 7.0,39.0,13.0,45.0, 5.0,37.0,
        63.0,31.0,55.0,23.0,61.0,29.0,53.0,21.0
    );
    int row = int(mod(float(p.y), 8.0));
    int col = int(mod(float(p.x), 8.0));
    return (B[row * 8 + col] + 0.5) / 64.0;
}

// Brute-force nearest-colour search across the active palette entries.
vec3 NearestPalette(vec3 col)
{
    vec3  best    = vec3(0.0);
    float bestDst = 1e9;
    for (int i = 0; i < u_PaletteSize; ++i)
    {
        vec3  p = texelFetch(u_Palette, i, 0).rgb;
        float d = dot(col - p, col - p);
        if (d < bestDst) { bestDst = d; best = p; }
    }
    return best;
}

void main()
{
    vec3 col = texture(u_Scene, v_UV).rgb;

    col = (col - 0.5) * u_Contrast + 0.5 + u_Brightness;
    col = clamp(col, 0.0, 1.0);

    if (u_ObraDinn)
    {
        float lum = dot(col, vec3(0.299, 0.587, 0.114));
        col = vec3(lum);
    }

    if (u_Dither > 0.001)
    {
        ivec2 screenPos = ivec2(v_UV * u_Resolution);
        float threshold = BayerMatrix(screenPos);
        float spread    = u_Dither / float(u_PaletteSize);
        col += (threshold - 0.5) * spread;
        col  = clamp(col, 0.0, 1.0);
    }

    col = NearestPalette(col);

    vec2  centred = v_UV * 2.0 - 1.0;
    float dist    = length(centred);
    float vig     = 1.0 - smoothstep(u_VigRadius, u_VigRadius + u_VigFeather, dist);
    col *= vig;

    if (u_Scanlines)
    {
        float scanline = mod(floor(v_UV.y * u_Resolution.y), 2.0);
        col *= mix(1.0, 1.0 - u_ScanAlpha, scanline);
    }

    FragColor = vec4(col, 1.0);
}
)";

// Default 32-colour Obra Dinn-inspired palette.
// First 16 entries are the standard low-contrast set; entries 17-32 extend
// the palette with additional hues for non-monochrome modes.
static const glm::vec3 kDefaultPalette[32] = {
    {0.047f,0.043f,0.055f},
    {0.122f,0.114f,0.133f},
    {0.220f,0.212f,0.235f},
    {0.380f,0.368f,0.392f},
    {0.600f,0.592f,0.608f},
    {0.820f,0.816f,0.824f},
    {0.980f,0.976f,0.980f},
    {0.824f,0.820f,0.706f},
    {0.698f,0.631f,0.412f},
    {0.545f,0.420f,0.196f},
    {0.380f,0.275f,0.094f},
    {0.200f,0.125f,0.027f},
    {0.094f,0.200f,0.282f},
    {0.165f,0.349f,0.431f},
    {0.259f,0.529f,0.584f},
    {0.008f,0.094f,0.157f},
    {0.549f,0.067f,0.067f},
    {0.400f,0.051f,0.051f},
    {0.200f,0.031f,0.031f},
    {0.847f,0.753f,0.376f},
    {0.573f,0.478f,0.098f},
    {0.208f,0.431f,0.204f},
    {0.110f,0.259f,0.110f},
    {0.047f,0.133f,0.051f},
    {0.471f,0.353f,0.624f},
    {0.302f,0.208f,0.471f},
    {0.157f,0.094f,0.282f},
    {0.600f,0.200f,0.400f},
    {0.800f,0.400f,0.200f},
    {0.600f,0.271f,0.118f},
    {0.824f,0.631f,0.506f},
    {0.700f,0.510f,0.380f},
};

PostProcess::~PostProcess()
{
    glDeleteFramebuffers(1, &m_fbo);
    glDeleteTextures(1, &m_colourTex);
    glDeleteTextures(1, &m_paletteTex);
    glDeleteRenderbuffers(1, &m_depthRBO);
    glDeleteVertexArrays(1, &m_quadVAO);
    glDeleteBuffers(1, &m_quadVBO);
    delete m_shader;
}

bool PostProcess::Init(int renderW, int renderH)
{
    m_renderW = renderW;
    m_renderH = renderH;

    CreateFBO();
    CreateFullscreenQuad();

    m_shader = new Shader();
    if (!m_shader->LoadFromSource(kPostVertSrc, kPostFragSrc))
    {
        std::cerr << "[PostProcess] Failed to compile post-process shader.\n";
        return false;
    }

    m_palette = {};
    std::copy(std::begin(kDefaultPalette), std::end(kDefaultPalette), m_palette.begin());
    BuildPalette();

    return true;
}

void PostProcess::CreateFBO()
{
    // Colour texture for the low-res scene. Nearest-neighbour filtering
    // preserves sharp pixel edges when upscaled to the window.
    glGenTextures(1, &m_colourTex);
    glBindTexture(GL_TEXTURE_2D, m_colourTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_renderW, m_renderH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenRenderbuffers(1, &m_depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_renderW, m_renderH);

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_colourTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, m_depthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[PostProcess] Framebuffer incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::CreateFullscreenQuad()
{
    // NDC quad covering [-1,1]x[-1,1] with UV [0,1]x[0,1].
    const float quad[] = {
        -1.f,-1.f,  0.f,0.f,
         1.f,-1.f,  1.f,0.f,
         1.f, 1.f,  1.f,1.f,
        -1.f,-1.f,  0.f,0.f,
         1.f, 1.f,  1.f,1.f,
        -1.f, 1.f,  0.f,1.f,
    };

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void PostProcess::BuildPalette()
{
    if (!m_paletteTex) glGenTextures(1, &m_paletteTex);
    glBindTexture(GL_TEXTURE_1D, m_paletteTex);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB32F,
                 kMaxPaletteSize, 0, GL_RGB, GL_FLOAT,
                 m_palette.data());
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void PostProcess::BeginCapture()
{
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_renderW, m_renderH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void PostProcess::EndCapture()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcess::Apply(int windowW, int windowH)
{
    // Compute a letterbox viewport: the largest rect with the same aspect
    // ratio as the render target that fits inside the window.
    float scaleX = static_cast<float>(windowW) / static_cast<float>(m_renderW);
    float scaleY = static_cast<float>(windowH) / static_cast<float>(m_renderH);
    float scale  = std::min(scaleX, scaleY);
    int vpW = static_cast<int>(m_renderW * scale);
    int vpH = static_cast<int>(m_renderH * scale);
    int vpX = (windowW - vpW) / 2;
    int vpY = (windowH - vpH) / 2;

    // GL_SCISSOR_TEST is left enabled by ImGui after its render pass.
    // Without disabling it first, glClear is clipped to ImGui's last scissor
    // rect and the letterbox bars retain stale pixel content from prior frames.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, windowW, windowH);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    glViewport(vpX, vpY, vpW, vpH);

    m_shader->Bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colourTex);
    m_shader->SetInt("u_Scene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, m_paletteTex);
    m_shader->SetInt("u_Palette", 1);

    m_shader->SetInt  ("u_PaletteSize", m_settings.paletteSize);
    m_shader->SetFloat("u_Dither",      m_settings.ditherStrength);
    m_shader->SetFloat("u_Contrast",    m_settings.contrast);
    m_shader->SetFloat("u_Brightness",  m_settings.brightness);
    m_shader->SetFloat("u_VigRadius",   m_settings.vignetteRadius);
    m_shader->SetFloat("u_VigFeather",  m_settings.vignetteFeather);
    m_shader->SetInt  ("u_Scanlines",   m_settings.scanlines ? 1 : 0);
    m_shader->SetFloat("u_ScanAlpha",   m_settings.scanlineAlpha);
    m_shader->SetInt  ("u_ObraDinn",    m_settings.obraDinnMode ? 1 : 0);
    m_shader->SetVec2 ("u_Resolution",  {(float)m_renderW, (float)m_renderH});

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    m_shader->Unbind();
    glEnable(GL_DEPTH_TEST);
}
