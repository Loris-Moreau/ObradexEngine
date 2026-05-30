#pragma once

// ============================================================
//  PostProcess.h  -  Dithering + Palette Post-Processor
// ============================================================
//  Implements the distinctive "Obra Dinn meets 8-bit" look:
//
//  Pipeline (applied after the 3-D scene is rendered):
//    1. Luminance calculation  - convert to greyscale
//    2. Bayer ordered dithering - 8×8 threshold matrix
//       applied in screen space to add halftone noise that
//       simulates the limited shading of classic displays.
//    3. Palette quantisation  - final colour snapped to the
//       nearest entry in a configurable 32-colour palette.
//    4. Vignette              - darkening at screen edges
//    5. Scanline overlay      - optional CRT scanline effect
//
//  The scene is rendered to a low-resolution FBO (e.g. 320×180)
//  and then upscaled via nearest-neighbour to the window size,
//  preserving the crisp pixel-art look.
// ============================================================

#include <array>
#include <glm/glm.hpp>

class Shader;

// ── PostProcessSettings ───────────────────────────────────────
/// Tweak-able parameters exposed in the editor UI.
struct PostProcessSettings
{
    // Dithering
    float ditherStrength = 0.92f;  ///< 0 = off, 1 = full Bayer dither
    int   paletteSize    = 24;    ///< Number of palette colours to use (2–32)

    // Vignette
    float vignetteRadius  = 1.025f; ///< Normalised distance from centre (higher = smaller vignette)
    float vignetteFeather = 0.25f; ///< Soft-edge width (lower = tighter fade)

    // Scanlines
    bool  scanlines       = true;
    float scanlineAlpha   = 0.12f; ///< How dark the scanline bars are

    // Colour grading
    float contrast        = 1.1f;
    float brightness      = 0.1f;  ///< Additive offset [-1..1]

    // Obra Dinn mode: forces monochrome + 1-bit dither
    bool  obraDinnMode    = false;
};

// ── PostProcess ───────────────────────────────────────────────
class PostProcess
{
public:
    PostProcess()  = default;
    ~PostProcess();

    // ── Lifecycle ─────────────────────────────────────────────

    /// Create the FBO and compile the post-process shader.
    /// @param renderW / renderH   Low-res internal resolution.
    bool Init(int renderW, int renderH);

    // ── Frame usage ───────────────────────────────────────────

    /// Redirect subsequent draw calls into the low-res FBO.
    void BeginCapture();

    /// Stop capturing; restore the default framebuffer.
    void EndCapture();

    /// Apply the dither + palette pass and blit to the window.
    /// @param windowW / windowH   Actual window pixel size (upscale target).
    void Apply(int windowW, int windowH);

    // ── Settings ─────────────────────────────────────────────
    PostProcessSettings& Settings() { return m_settings; }

private:
    void BuildPalette();   ///< Upload current palette to GPU
    void CreateFBO();      ///< Allocate framebuffer objects
    void CreateFullscreenQuad();

    PostProcessSettings m_settings;

    // Framebuffer
    unsigned m_fbo          = 0;
    unsigned m_colourTex    = 0;
    unsigned m_depthRBO     = 0;
    int      m_renderW      = 0;
    int      m_renderH      = 0;

    // Fullscreen quad
    unsigned m_quadVAO      = 0;
    unsigned m_quadVBO      = 0;

    // Palette texture (32-entry 1D RGBA texture uploaded to GPU)
    unsigned m_paletteTex   = 0;

    // The post-process shader (compiled from embedded GLSL)
    Shader*  m_shader       = nullptr;

    // Built-in palettes
    static constexpr int kMaxPaletteSize = 32;
    std::array<glm::vec3, kMaxPaletteSize> m_palette;
};
