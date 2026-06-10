// Input.cpp - Frame-snapshot input implementation.

#include "Input.h"
#include <GLFW/glfw3.h>
#include <cctype>
#include <iostream>

void Input::Init(GLFWwindow* window)
{
    m_window = window;

    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, ScrollCallback);

    // Seed cursor position so the first frame delta is not a large jump.
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_mousePos  = { static_cast<float>(x), static_cast<float>(y) };
    m_mousePrev = m_mousePos;

    m_keys[0].fill(0);
    m_keys[1].fill(0);
    m_buttons[0].fill(0);
    m_buttons[1].fill(0);
    m_cur = 0;
}

// Call once per frame after glfwPollEvents() and before any IsKey* queries.
// Flips m_cur, then overwrites only m_keys[m_cur] with fresh glfwGetKey data.
// The opposite slot is untouched, so it always holds last frame's snapshot.
void Input::Update()
{
    m_cur = 1 - m_cur;

    const int keyCount = static_cast<int>(Key::_Count);
    for (int i = 0; i < keyCount; ++i)
        m_keys[m_cur][i] = glfwGetKey(m_window, i);

    const int btnCount = static_cast<int>(MouseButton::_Count);
    for (int i = 0; i < btnCount; ++i)
        m_buttons[m_cur][i] = glfwGetMouseButton(m_window, i);

    m_mousePrev = m_mousePos;
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    m_mousePos   = { static_cast<float>(x), static_cast<float>(y) };
    m_mouseDelta = m_mousePos - m_mousePrev;

    m_scrollDelta = m_scrollAccum;
    m_scrollAccum = 0.f;
}

bool Input::IsKeyHeld(Key k) const
{
    int idx = static_cast<int>(k);
    if (idx < 0 || idx >= static_cast<int>(Key::_Count)) return false;
    return m_keys[m_cur][idx] == GLFW_PRESS;
}

bool Input::IsKeyJustPressed(Key k) const
{
    int idx = static_cast<int>(k);
    if (idx < 0 || idx >= static_cast<int>(Key::_Count)) return false;
    int prev = 1 - m_cur;
    return m_keys[m_cur][idx]  == GLFW_PRESS &&
           m_keys[prev][idx]   == GLFW_RELEASE;
}

bool Input::IsKeyJustReleased(Key k) const
{
    int idx = static_cast<int>(k);
    if (idx < 0 || idx >= static_cast<int>(Key::_Count)) return false;
    int prev = 1 - m_cur;
    return m_keys[m_cur][idx]  == GLFW_RELEASE &&
           m_keys[prev][idx]   == GLFW_PRESS;
}

bool Input::IsButtonHeld(MouseButton b) const
{
    int idx = static_cast<int>(b);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::_Count)) return false;
    return m_buttons[m_cur][idx] == GLFW_PRESS;
}

bool Input::IsButtonJustPressed(MouseButton b) const
{
    int idx = static_cast<int>(b);
    if (idx < 0 || idx >= static_cast<int>(MouseButton::_Count)) return false;
    int prev = 1 - m_cur;
    return m_buttons[m_cur][idx] == GLFW_PRESS &&
           m_buttons[prev][idx]  == GLFW_RELEASE;
}

const char* Input::GetKeyName(Key k)
{
    // Use GLFW's OS-level key label so the prompt reads correctly on any
    // keyboard layout: physical W shows "W" on QWERTY and "Z" on AZERTY.
    const char* g = glfwGetKeyName(static_cast<int>(k), 0);
    if (g && g[0] != '\0')
    {
        thread_local char buf[8];
        int i = 0;
        while (g[i] && i < 7)
        { buf[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(g[i]))); ++i; }
        buf[i] = '\0';
        return buf;
    }
    // Non-printable fallbacks
    switch (k)
    {
        case Key::Space:  return "Space";
        case Key::LShift: return "Shift";
        case Key::LCtrl:  return "Ctrl";
        case Key::F1:     return "F1";
        case Key::Escape: return "Esc";
        case Key::Tab:    return "Tab";
        default:          return "?";
    }
}

void Input::ScrollCallback(GLFWwindow* window, double /*xOff*/, double yOff)
{
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (self) self->m_scrollAccum += static_cast<float>(yOff);
}
