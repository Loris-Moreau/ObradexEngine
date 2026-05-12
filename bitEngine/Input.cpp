// ============================================================
//  Input.cpp  —  Keyboard, Mouse & Controller Input
// ============================================================

#include "Input.h"
#include <GLFW/glfw3.h>

// ── Init ──────────────────────────────────────────────────────
void Input::Init(GLFWwindow* window)
{
    m_window = window;

    // Store 'this' so the static scroll callback can reach us
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, ScrollCallback);

    // Seed mouse position so the first delta isn't a huge jump
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_mousePos  = { static_cast<float>(x), static_cast<float>(y) };
    m_mousePrev = m_mousePos;
}

// ── Update ────────────────────────────────────────────────────
void Input::Update()
{
    // Advance the double-buffer index FIRST so that every query
    // function reading (m_frame & 1) sees the buffer we are about
    // to fill, not the one we just filled last frame.
    ++m_frame;
    int cur  = m_frame & 1;
    int prev = 1 - cur;

    // ── Keyboard snapshot ─────────────────────────────────────
    const int keyCount = static_cast<int>(Key::_Count);
    for (int i = 0; i < keyCount; ++i)
    {
        // Copy current into previous, then re-query
        m_keys[prev][i] = m_keys[cur][i];
        m_keys[cur][i]  = glfwGetKey(m_window, i);
    }

    // ── Mouse button snapshot ─────────────────────────────────
    const int btnCount = static_cast<int>(MouseButton::_Count);
    for (int i = 0; i < btnCount; ++i)
    {
        m_buttons[prev][i] = m_buttons[cur][i];
        m_buttons[cur][i]  = glfwGetMouseButton(m_window, i);
    }

    // ── Mouse position & delta ────────────────────────────────
    m_mousePrev = m_mousePos;
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    m_mousePos  = { static_cast<float>(x), static_cast<float>(y) };

    // Delta = this - last (in pixels, Y-down screen space)
    m_mouseDelta = m_mousePos - m_mousePrev;

    // ── Scroll ────────────────────────────────────────────────
    // Grab what the callback has accumulated and reset the bucket
    m_scrollDelta = m_scrollAccum;
    m_scrollAccum = 0.f;

}

// ── Keyboard queries ──────────────────────────────────────────
bool Input::IsKeyHeld(Key k) const
{
    int idx = static_cast<int>(k);
    if (idx < 0 || idx >= static_cast<int>(Key::_Count)) return false;
    return m_keys[m_frame & 1][idx] == GLFW_PRESS;
}

bool Input::IsKeyJustPressed(Key k) const
{
    int idx = static_cast<int>(k);
    if (idx < 0 || idx >= static_cast<int>(Key::_Count)) return false;
    int cur  = m_frame & 1;
    int prev = 1 - cur;
    return m_keys[cur][idx]  == GLFW_PRESS &&
           m_keys[prev][idx] == GLFW_RELEASE;
}

bool Input::IsKeyJustReleased(Key k) const
{
    int idx = static_cast<int>(k);
    if (idx < 0 || idx >= static_cast<int>(Key::_Count)) return false;
    int cur  = m_frame & 1;
    int prev = 1 - cur;
    return m_keys[cur][idx]  == GLFW_RELEASE &&
           m_keys[prev][idx] == GLFW_PRESS;
}

// ── Mouse button queries ──────────────────────────────────────
bool Input::IsButtonHeld(MouseButton b) const
{
    int idx = static_cast<int>(b);
    return m_buttons[m_frame & 1][idx] == GLFW_PRESS;
}

bool Input::IsButtonJustPressed(MouseButton b) const
{
    int idx  = static_cast<int>(b);
    int cur  = m_frame & 1;
    int prev = 1 - cur;
    return m_buttons[cur][idx]  == GLFW_PRESS &&
           m_buttons[prev][idx] == GLFW_RELEASE;
}

// ── Static scroll callback ────────────────────────────────────
void Input::ScrollCallback(GLFWwindow* win, double /*xOff*/, double yOff)
{
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(win));
    if (self) self->m_scrollAccum += static_cast<float>(yOff);
}
