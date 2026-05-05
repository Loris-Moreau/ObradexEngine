// ============================================================
//  PostProcess.cpp  —  Dithering + Palette Post-Processor
// ============================================================

#include "PostProcess.h"
#include "Shader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <iostream>

// ── Embedded GLSL source ──────────────────────────────────────
// Kept here so the engine has zero external shader file deps
// for the post-process pass (world shaders are still external).

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

uniform sampler2D u_Scene;       // Low-res scene texture
uniform sampler1D u_Palette;     // Colour palette (32 entries)
uniform int       u_PaletteSize; // Active palette entries
uniform float     u_Dither;      // Dither strength 0–1
uniform float     u_Contrast;
uniform float     u_Brightness;
uniform float     u_VigRadius;
uniform float     u_VigFeather;
uniform bool      u_Scanlines;
uniform float     u_ScanAlpha;
uniform bool      u_ObraDinn;    // Monochrome 1-bit mode
uniform vec2      u_Resolution;  // Low-res size (for screen-space Bayer)

// ── 8×8 Bayer matrix (normalised to 0..1) ────────────────────
// Pre-divided by 65 (max value + 1) for direct threshold use.
float BayerMatrix(ivec2 p)
{
    const float B[8][8] = float[8][8](
        float[8]( 0,32, 8,40, 2,34,10,42),
        float[8](48,16,56,24,50,18,58,26),
        float[8](12,44, 4,36,14,46, 6,38),
        float[8](60,28,52,20,62,30,54,22),
        float[8]( 3,35,11,43, 1,33, 9,41),
        float[8](51,19,59,27,49,17,57,25),
        float[8](15,47, 7,39,13,45, 5,37),
        float[8](63,31,55,23,61,29,53,21)
    );
    return B[p.y & 7][p.x & 7] / 64.0;
}

// ── Find nearest palette colour (brute-force, 32 entries max) ─
vec3 NearestPalette(vec3 col)
{
    vec3  best    = vec3(0.0);
    float bestDst = 1e9;
    for (int i = 0; i < u_PaletteSize; ++i)
    {
        vec3  p = texelFetch(u_Palette, i, 0).rgb;
        float d = dot(col - p, col - p); // Squared L2 distance
        if (d < bestDst) { bestDst = d; best = p; }
    }
    return best;
}

void main()
{
    // ── 1. Sample scene ───────────────────────────────────────
    vec3 col = texture(u_Scene, v_UV).rgb;

    // ── 2. Contrast / brightness ──────────────────────────────
    col = (col - 0.5) * u_Contrast + 0.5 + u_Brightness;
    col = clamp(col, 0.0, 1.0);

    // ── 3. Obra Dinn monochrome conversion ────────────────────
    if (u_ObraDinn)
    {
        float lum = dot(col, vec3(0.299, 0.587, 0.114));
        col = vec3(lum);
    }

    // ── 4. Bayer ordered dithering ────────────────────────────
    if (u_Dither > 0.001)
    {
        ivec2 screenPos = ivec2(v_UV * u_Resolution);
        float threshold = BayerMatrix(screenPos);
        // Spread is proportional to the quantisation step size
        float spread = u_Dither / float(u_PaletteSize);
        col += (threshold - 0.5) * spread;
        col  = clamp(col, 0.0, 1.0);
    }

    // ── 5. Palette quantisation ───────────────────────────────
    col = NearestPalette(col);

    // ── 6. Vignette ───────────────────────────────────────────
    vec2  centred = v_UV * 2.0 - 1.0;
    float dist    = length(centred);
    float vig     = 1.0 - smoothstep(u_VigRadius,
                                      u_VigRadius + u_VigFeather, dist);
    col *= vig;

    // ── 7. Scanlines ──────────────────────────────────────────
    if (u_Scanlines)
    {
        // One dark bar every 2 pixels in low-res space
        float scanline = mod(floor(v_UV.y * u_Resolution.y), 2.0);
        col *= mix(1.0, 1.0 - u_ScanAlpha, scanline);
    }

    FragColor = vec4(col, 1.0);
}
)";

// ── Built-in palettes ─────────────────────────────────────────
// "Obra-Dinn" inspired: 16-colour limited palette
// mix of high-contrast blacks, off-whites, and amber/teal accents
static const glm::vec3 kDefaultPalette[32] = {
    {0.047f,0.043f,0.055f}, // Almost black
    {0.122f,0.114f,0.133f}, // Dark grey
    {0.220f,0.212f,0.235f}, // Mid grey
    {0.380f,0.368f,0.392f}, // Light grey
    {0.600f,0.592f,0.608f}, // Silver
    {0.820f,0.816f,0.824f}, // Near-white
    {0.980f,0.976f,0.980f}, // White
    {0.824f,0.820f,0.706f}, // Parchment
    {0.698f,0.631f,0.412f}, // Aged paper
    {0.545f,0.420f,0.196f}, // Warm amber
    {0.380f,0.275f,0.094f}, // Dark amber
    {0.200f,0.125f,0.027f}, // Mahogany
    {0.094f,0.200f,0.282f}, // Deep teal
    {0.165f,0.349f,0.431f}, // Teal
    {0.259f,0.529f,0.584f}, // Cyan-teal
    {0.008f,0.094f,0.157f}, // Navy
    // Extended palette (only used when paletteSize > 16)
    {0.549f,0.067f,0.067f}, // Blood red
    {0.400f,0.051f,0.051f}, // Dark red
    {0.200f,0.031f,0.031f}, // Very dark red
    {0.847f,0.753f,0.376f}, // Gold
    {0.573f,0.478f,0.098f}, // Dark gold
    {0.208f,0.431f,0.204f}, // Mossgreen
    {0.110f,0.259f,0.110f}, // Dark moss
    {0.047f,0.133f,0.051f}, // Very dark green
    {0.471f,0.353f,0.624f}, // Lavender
    {0.302f,0.208f,0.471f}, // Purple
    {0.157f,0.094f,0.282f}, // Dark purple
    {0.600f,0.200f,0.400f}, // Mauve
    {0.800f,0.400f,0.200f}, // Terracotta
    {0.600f,0.271f,0.118f}, // Rust
    {0.824f,0.631f,0.506f}, // Skin
    {0.700f,0.510f,0.380f}, // Tan
};

// ============================================================

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

// ── Init ──────────────────────────────────────────────────────
bool PostProcess::Init(int renderW, int renderH)
{
    m_renderW = renderW;
    m_renderH = renderH;

    CreateFBO();
    CreateFullscreenQuad();

    // Compile the embedded post-process shader
    m_shader = new Shader();
    if (!m_shader->LoadFromSource(kPostVertSrc, kPostFragSrc))
    {
        std::cerr << "[PostProcess] Failed to compile post-process shader.\n";
        return false;
    }

    // Initialise palette from built-in default
    m_palette = {};
    std::copy(std::begin(kDefaultPalette),
              std::end(kDefaultPalette),
              m_palette.begin());
    BuildPalette();

    return true;
}

// ── CreateFBO ─────────────────────────────────────────────────
void PostProcess::CreateFBO()
{
    // Colour texture — scene is rendered here at low resolution
    glGenTextures(1, &m_colourTex);
    glBindTexture(GL_TEXTURE_2D, m_colourTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_renderW, m_renderH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    // Nearest-neighbour filtering preserves sharp pixel edges
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Depth renderbuffer
    glGenRenderbuffers(1, &m_depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                          m_renderW, m_renderH);

    // Framebuffer assembly
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

// ── CreateFullscreenQuad ──────────────────────────────────────
void PostProcess::CreateFullscreenQuad()
{
    // NDC quad covering [-1,1]×[-1,1] with UV [0,1]×[0,1]
    const float quad[] = {
        // position  UV
        -1.f,-1.f,   0.f,0.f,
         1.f,-1.f,   1.f,0.f,
         1.f, 1.f,   1.f,1.f,
        -1.f,-1.f,   0.f,0.f,
         1.f, 1.f,   1.f,1.f,
        -1.f, 1.f,   0.f,1.f,
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

// ── BuildPalette ──────────────────────────────────────────────
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

// ── BeginCapture / EndCapture ─────────────────────────────────
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

// ── Apply ─────────────────────────────────────────────────────
void PostProcess::Apply(int windowW, int windowH)
{
    // Restore full-window viewport for the blit
    glViewport(0, 0, windowW, windowH);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_shader->Bind();

    // Bind scene texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_colourTex);
    m_shader->SetInt("u_Scene", 0);

    // Bind palette texture to unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, m_paletteTex);
    m_shader->SetInt("u_Palette", 1);

    // Upload settings as uniforms
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

    // Draw fullscreen quad
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    m_shader->Unbind();
    glEnable(GL_DEPTH_TEST);
}
