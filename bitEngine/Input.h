#pragma once

// Input.h - Frame-snapshot keyboard, mouse, and scroll input.
//
// Built on GLFW. Call Update() once per frame after glfwPollEvents(),
// then query IsKey* and mouse methods freely until the next Update().
//
// Double-buffer design:
//   Two key-state arrays are indexed by m_cur (0 or 1).
//   Update() flips m_cur, then writes only m_keys[m_cur] with a fresh
//   glfwGetKey snapshot. The other slot retains the previous frame's
//   data automatically, so IsKeyJustPressed compares [cur] vs [1-cur].
//
//   The naive "copy-then-overwrite" pattern copies [cur] to [prev] before
//   flipping, but after the flip cur and prev have swapped, so it copies
//   the stale buffer over the fresh one. This causes IsKeyJustPressed to
//   fire every frame a key is held. The flip-first approach avoids that.
//
// Key values match GLFW_KEY_* constants for zero-cost casting.

#include <array>
#include <glm/glm.hpp>

struct GLFWwindow;

enum class Key : int
{
    // AZERTY movement: Z/Q/S/D map to the W/A/S/D GLFW positions.
    Z = 87, Q = 65, S = 83, D = 68,

    A      = 81,   // Lean left  (Q position on QWERTY)
    E      = 69,   // Lean right

    Space  = 32,   // Jump
    LShift = 340,  // Sprint
    LCtrl  = 341,  // Crouch / slide

    F1     = 290,  // Toggle editor overlay
    Escape = 256,

    R = 82, F = 70, G = 71, Tab = 258, I = 73,
    D1 = 49, D2 = 50, D3 = 51, D4 = 52,

    _Count = 350   // Must be last; sets array size
};

// Single source of truth for the interact binding. All systems that display
// or check the interact key should reference this constant.
constexpr Key INTERACT_KEY = Key::F;

enum class MouseButton : int
{
    Left   = 0,
    Right  = 1,
    Middle = 2,
    _Count = 3
};

class Input
{
public:
    Input() = default;

    void Init(GLFWwindow* window);  // Register GLFW callbacks
    void Update();                  // Snapshot current key/mouse state; call once per frame

    // Keyboard queries
    bool IsKeyHeld        (Key k) const;  // True every frame the key is down
    bool IsKeyJustPressed (Key k) const;  // True only on the frame it transitions down
    bool IsKeyJustReleased(Key k) const;  // True only on the frame it transitions up

    // Mouse queries
    bool      IsButtonHeld       (MouseButton b) const;
    bool      IsButtonJustPressed(MouseButton b) const;
    glm::vec2 GetMousePos  () const { return m_mousePos;    }
    glm::vec2 GetMouseDelta() const { return m_mouseDelta;  }
    float     GetScrollDelta()const { return m_scrollDelta; }

    // Returns a human-readable key name (e.g. Key::F -> "F").
    // Used by the HUD to build prompt strings dynamically.
    static const char* GetKeyName(Key k);

private:
    static void ScrollCallback(GLFWwindow*, double xOff, double yOff);

    GLFWwindow* m_window = nullptr;

    // m_keys[m_cur]   = current frame snapshot
    // m_keys[1-m_cur] = previous frame snapshot
    std::array<int, static_cast<int>(Key::_Count)>         m_keys[2]    = {};
    std::array<int, static_cast<int>(MouseButton::_Count)> m_buttons[2] = {};
    int m_cur = 0;

    glm::vec2 m_mousePos    = {};
    glm::vec2 m_mousePrev   = {};
    glm::vec2 m_mouseDelta  = {};
    float     m_scrollDelta = 0.f;
    float     m_scrollAccum = 0.f;
};
