// Window.cpp - GLFW window and OpenGL context.

#include "Window.h"

// GLAD must be included before GLFW.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

bool Window::Init(const std::string& title,
                  int width, int height,
                  bool fullscreen, bool vsync)
{
    if (!glfwInit())
    {
        std::cerr << "[Window] glfwInit() failed.\n";
        return false;
    }

    // Request OpenGL 4.1 Core Profile (supported on Windows, Linux, and macOS).
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    if (fullscreen)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width  = mode->width;
        height = mode->height;
    }

    m_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!m_window)
    {
        std::cerr << "[Window] glfwCreateWindow() failed.\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        std::cerr << "[Window] GLAD failed to load OpenGL functions.\n";
        return false;
    }

    glfwSwapInterval(vsync ? 1 : 0);

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);

    m_width  = width;
    m_height = height;

    std::cout << "[Window] OpenGL "
              << glGetString(GL_VERSION) << " on "
              << glGetString(GL_RENDERER) << "\n";

    return true;
}

Window::~Window()
{
    if (m_window)
    {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void Window::PollEvents()  { glfwPollEvents(); }
void Window::SwapBuffers() { glfwSwapBuffers(m_window); }
bool Window::ShouldClose() const { return glfwWindowShouldClose(m_window); }

// Query the framebuffer size live on each call.
// Relying on a cached m_width/m_height value set by the resize callback can
// be stale after a maximize or DPI-scaling transition, causing the letterbox
// calculation in PostProcess::Apply to use the wrong dimensions.
int Window::GetWidth() const
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    return w;
}

int Window::GetHeight() const
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    return h;
}

float Window::GetAspect() const
{
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    if (h == 0) return 1.0f;
    return static_cast<float>(w) / static_cast<float>(h);
}

void Window::SetCursorLocked(bool locked)
{
    m_cursorLocked = locked;
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void Window::FramebufferSizeCallback(GLFWwindow* win, int w, int h)
{
    // Update the cached dimensions. GetWidth/GetHeight query live via
    // glfwGetFramebufferSize so these values are mainly used for fallback;
    // do NOT call glViewport here as it would override PostProcess::Apply's
    // letterbox viewport on the same frame.
    auto* self   = static_cast<Window*>(glfwGetWindowUserPointer(win));
    self->m_width  = w;
    self->m_height = h;
}
