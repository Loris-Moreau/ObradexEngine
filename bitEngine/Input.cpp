// ============================================================
//  Input.cpp
// ============================================================

#include "Input.h"
#include <GLFW/glfw3.h>
#include <iostream>

// ── Init ──────────────────────────────────────────────────────
void Input::Init(GLFWwindow* window)
{
    m_window = window;

    // Register scroll accumulator callback
    glfwSetWindowUserPointer(window, this);
    glfwSetScrollCallback(window, ScrollCallback);

    // Seed cursor position so the first delta isn't a huge jump
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_mousePos  = { static_cast<float>(x), static_cast<float>(y) };
    m_mousePrev = m_mousePos;

    // Zero both key buffers
    m_keys[0].fill(0);
    m_keys[1].fill(0);
    m_buttons[0].fill(0);
    m_buttons[1].fill(0);
    m_cur = 0;
}

// ── Update ───────────────────────────────────────────────────
// Call once per frame AFTER glfwPollEvents() and BEFORE any
// IsKey* queries.
//
// Pattern: flip m_cur, then overwrite m_keys[m_cur] with a
// fresh glfwGetKey snapshot.  The opposite slot is NEVER
// written here — it retains the previous frame's snapshot,
// which is exactly what IsKeyJustPressed / IsKeyJustReleased
// need for the "was it different last frame?" check.
void Input::Update()
{
    // Flip current buffer — the slot we are about to overwrite
    // naturally becomes "current", and the untouched slot becomes
    // "previous" (it still holds last Update()'s snapshot).
    m_cur = 1 - m_cur;

    // ── Keyboard snapshot ────────────────────────────────────
    const int keyCount = static_cast<int>(Key::_Count);
    for (int i = 0; i < keyCount; ++i)
        m_keys[m_cur][i] = glfwGetKey(m_window, i);

    // ── Mouse button snapshot ────────────────────────────────
    const int btnCount = static_cast<int>(MouseButton::_Count);
    for (int i = 0; i < btnCount; ++i)
        m_buttons[m_cur][i] = glfwGetMouseButton(m_window, i);

    // ── Cursor delta ─────────────────────────────────────────
    m_mousePrev = m_mousePos;
    double x, y;
    glfwGetCursorPos(m_window, &x, &y);
    m_mousePos   = { static_cast<float>(x), static_cast<float>(y) };
    m_mouseDelta = m_mousePos - m_mousePrev;

    // ── Scroll ───────────────────────────────────────────────
    m_scrollDelta = m_scrollAccum;
    m_scrollAccum = 0.f;
}

// ── Keyboard queries ─────────────────────────────────────────
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

// ── Mouse button queries ─────────────────────────────────────
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

// ── GetKeyName ───────────────────────────────────────────────
const char* Input::GetKeyName(Key k)
{
    switch (k)
    {
        case Key::Z:      return "Z";
        case Key::Q:      return "Q";
        case Key::S:      return "S";
        case Key::D:      return "D";
        case Key::A:      return "A";
        case Key::E:      return "E";
        //case Key::W:      return "W";
        case Key::Space:  return "Space";
        case Key::LShift: return "Shift";
        case Key::LCtrl:  return "Ctrl";
        case Key::F1:     return "F1";
        case Key::Escape: return "Esc";
        case Key::R:      return "R";
        case Key::F:      return "F";
        case Key::G:      return "G";
        case Key::Tab:    return "Tab";
        default:          return "?";
    }
}

// ── Scroll callback ───────────────────────────────────────────
void Input::ScrollCallback(GLFWwindow* window, double /*xOff*/, double yOff)
{
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (self) self->m_scrollAccum += static_cast<float>(yOff);
}
