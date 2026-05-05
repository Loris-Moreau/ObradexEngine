// ============================================================
//  Window.cpp  —  GLFW Window & OpenGL Context
// ============================================================

#include "Window.h"

// GLAD must be included before GLFW (provides OpenGL headers)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

// ── Init ──────────────────────────────────────────────────────
bool Window::Init(const std::string& title,
                  int width, int height,
                  bool fullscreen, bool vsync)
{
    // ── GLFW initialisation ───────────────────────────────────
    if (!glfwInit())
    {
        std::cerr << "[Window] glfwInit() failed.\n";
        return false;
    }

    // Request OpenGL 4.1 Core Profile (broadly supported, incl. macOS)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on macOS
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // ── Monitor selection for fullscreen ──────────────────────
    GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    if (fullscreen)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width  = mode->width;
        height = mode->height;
    }

    // ── Create window ─────────────────────────────────────────
    m_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!m_window)
    {
        std::cerr << "[Window] glfwCreateWindow() failed.\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    // ── Load OpenGL function pointers via GLAD ─────────────────
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "[Window] GLAD failed to load OpenGL functions.\n";
        return false;
    }

    // ── VSync ─────────────────────────────────────────────────
    glfwSwapInterval(vsync ? 1 : 0);

    // ── Framebuffer resize callback ───────────────────────────
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);

    m_width  = width;
    m_height = height;

    // Print OpenGL info for diagnostics
    std::cout << "[Window] OpenGL "
              << glGetString(GL_VERSION) << " on "
              << glGetString(GL_RENDERER) << "\n";

    return true;
}

// ── Destructor ────────────────────────────────────────────────
Window::~Window()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

// ── PollEvents / SwapBuffers ──────────────────────────────────
void Window::PollEvents()  { glfwPollEvents(); }
void Window::SwapBuffers() { glfwSwapBuffers(m_window); }

// ── Queries ───────────────────────────────────────────────────
bool  Window::ShouldClose() const { return glfwWindowShouldClose(m_window); }
float Window::GetAspect()   const
{
    if (m_height == 0) return 1.0f;
    return static_cast<float>(m_width) / static_cast<float>(m_height);
}

// ── Cursor lock (FPS capture) ─────────────────────────────────
void Window::SetCursorLocked(bool locked)
{
    m_cursorLocked = locked;
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

// ── Framebuffer resize callback ───────────────────────────────
void Window::FramebufferSizeCallback(GLFWwindow* win, int w, int h)
{
    // Update stored dimensions; the Renderer reads these when blitting.
    auto* self   = static_cast<Window*>(glfwGetWindowUserPointer(win));
    self->m_width  = w;
    self->m_height = h;

    // Keep the full-window viewport in sync
    glViewport(0, 0, w, h);
}
