#pragma once

// ============================================================
//  Window.h  -  GLFW Window & OpenGL Context
// ============================================================
//  Wraps GLFW for window creation, OpenGL context management,
//  and basic window queries. Owns the GLFW lifecycle.
// ============================================================

#include <string>

struct GLFWwindow;  // Forward-declare; avoids including GLFW here

class Window
{
public:
    Window()  = default;
    ~Window();

    // ── Lifecycle ─────────────────────────────────────────────

    /// Create the OS window and an OpenGL 4.1 core context.
    /// @param title       Window title bar string.
    /// @param width Client area dimensions in pixels.
    /// @param height Client area dimensions in pixels.
    /// @param fullscreen  Borderless fullscreen on primary monitor.
    /// @param vsync       Enable vertical sync (limits frame rate).
    bool Init(const std::string& title,
              int width, int height,
              bool fullscreen, bool vsync);

    /// Poll OS events (keyboard, mouse, resize, close).
    void PollEvents();

    /// Swap the back-buffer to the screen.
    void SwapBuffers();

    // ── Queries ───────────────────────────────────────────────
    bool  ShouldClose() const;   ///< True when the user closes the window
    int   GetWidth()    const;   ///< Current framebuffer width  (live query)
    int   GetHeight()   const;   ///< Current framebuffer height (live query)
    float GetAspect()   const;   ///< width / height (live query)

    /// Raw GLFW handle (needed by ImGui, Input callbacks).
    GLFWwindow* GetGLFWWindow() const { return m_window; }

    /// Lock the mouse cursor to the window centre (FPS mode).
    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const { return m_cursorLocked; }

private:
    // Called by GLFW when the window is resized
    static void FramebufferSizeCallback(GLFWwindow*, int w, int h);

    GLFWwindow* m_window      = nullptr;
    int         m_width       = 0;
    int         m_height      = 0;
    bool        m_cursorLocked = false;
};
