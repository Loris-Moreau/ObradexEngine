#pragma once

// PostProcess.h - Dithering and palette post-processor.
//
// Pipeline applied after the 3-D scene is rendered into the low-res FBO:
//   1. Contrast / brightness colour grading
//   2. Optional greyscale conversion (Obra Dinn mode)
//   3. Bayer 8x8 ordered dithering in screen space
//   4. Nearest-palette-colour quantisation
//   5. Radial vignette
//   6. Optional CRT scanline overlay
//
// The scene is rendered at a low internal resolution (e.g. 320x180) and
// upscaled to the window with nearest-neighbour filtering for a crisp
// pixel-art look. Apply() computes a letterbox viewport each frame so the
// image is always centred and never stretched at any window size.

#include <array>
#include <glm/glm.hpp>

class Shader;

// Runtime-tunable settings; exposed in the Renderer editor tab.
struct PostProcessSettings
{
    float ditherStrength  = 0.92f;  // 0 = off, 1 = full Bayer dither
    int   paletteSize     = 24;     // Active palette entries (2 to 32)
    float vignetteRadius  = 1.025f;
    float vignetteFeather = 0.25f;
    bool  scanlines       = true;
    float scanlineAlpha   = 0.12f;
    float contrast        = 1.1f;
    float brightness      = 0.1f;   // Additive offset, range -1 to 1
    bool  obraDinnMode    = false;  // Forces monochrome + heavy dither
};

class PostProcess
{
public:
    PostProcess()  = default;
    ~PostProcess();

    // Create the FBO and compile the embedded post-process shader.
    // renderW / renderH set the low-res internal resolution.
    bool Init(int renderW, int renderH);

    // Redirect draw calls to the low-res FBO.
    void BeginCapture();

    // Stop writing to the FBO and restore the default framebuffer.
    void EndCapture();

    // Apply the dither + palette pass, then blit to the window with
    // letterboxing. windowW / windowH are the current framebuffer dimensions.
    void Apply(int windowW, int windowH);

    PostProcessSettings& Settings() { return m_settings; }

private:
    void BuildPalette();         // Upload m_palette to the GPU 1D texture
    void CreateFBO();            // Allocate the low-res FBO and colour texture
    void CreateFullscreenQuad(); // Build the NDC quad VAO for the blit

    PostProcessSettings m_settings;

    unsigned m_fbo       = 0;
    unsigned m_colourTex = 0;
    unsigned m_depthRBO  = 0;
    int      m_renderW   = 0;
    int      m_renderH   = 0;

    unsigned m_quadVAO   = 0;
    unsigned m_quadVBO   = 0;

    unsigned m_paletteTex = 0;  // 1D texture with kMaxPaletteSize RGB entries
    Shader*  m_shader     = nullptr;

    static constexpr int kMaxPaletteSize = 32;
    std::array<glm::vec3, kMaxPaletteSize> m_palette;
};
