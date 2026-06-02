#pragma once

// Window.h - GLFW window and OpenGL context wrapper.
//
// Owns the GLFW lifecycle. Destructor calls glfwTerminate.
// GetWidth/GetHeight query the framebuffer live via glfwGetFramebufferSize
// each call to avoid stale cached values after a maximize or DPI change.

#include <string>

struct GLFWwindow;

class Window
{
public:
    Window()  = default;
    ~Window();

    // Create the OS window and an OpenGL 4.1 Core context.
    // width/height are the client area in pixels.
    // fullscreen uses the primary monitor's native resolution.
    bool Init(const std::string& title,
              int width, int height,
              bool fullscreen, bool vsync);

    void PollEvents();   // Process queued OS events (keyboard, mouse, resize)
    void SwapBuffers();  // Present the back-buffer to the screen

    bool  ShouldClose() const;  // True when the user has requested close
    int   GetWidth()    const;  // Live framebuffer width in pixels
    int   GetHeight()   const;  // Live framebuffer height in pixels
    float GetAspect()   const;  // width / height

    // Raw GLFW handle required by ImGui and Input callbacks.
    GLFWwindow* GetGLFWWindow() const { return m_window; }

    // Capture the mouse cursor for FPS look (GLFW_CURSOR_DISABLED).
    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const { return m_cursorLocked; }

private:
    // GLFW callback registered via glfwSetFramebufferSizeCallback.
    static void FramebufferSizeCallback(GLFWwindow*, int w, int h);

    GLFWwindow* m_window       = nullptr;
    int         m_width        = 0;
    int         m_height       = 0;
    bool        m_cursorLocked = false;
};
