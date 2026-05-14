#pragma once

// ============================================================
//  Input.h  —  Keyboard, Mouse & Scroll Input
// ============================================================
//  Frame-snapshot polling built on top of GLFW.
//
//  How the double-buffer works
//  ---------------------------
//  We keep two key-state arrays, indexed by m_cur (0 or 1).
//  Each call to Update() flips m_cur, then overwrites
//  m_keys[m_cur] with a fresh glfwGetKey() snapshot.
//  The OTHER slot — m_keys[1-m_cur] — is NEVER touched
//  during that Update(); it naturally holds the previous
//  frame's snapshot from the last time it was current.
//
//  This "flip-then-overwrite" pattern is subtly different
//  from the naive "copy-then-overwrite" approach.  The naive
//  version copies m_keys[cur] → m_keys[prev] before querying,
//  but after the index flip, cur and prev have swapped, so
//  it ends up copying the STALE buffer (two frames old) over
//  the RECENT one — making IsKeyJustPressed fire every frame
//  a key is held, not just the first.
//
//  Key enum
//  --------
//  Values are identical to GLFW_KEY_* so we can cast directly
//  with no lookup table overhead.  AZERTY keys are annotated.
// ============================================================

#include <array>
#include <glm/glm.hpp>

struct GLFWwindow;

// ── Key enum ─────────────────────────────────────────────────
enum class Key : int
{
    // Movement — AZERTY layout
    // Z is at the W position on QWERTY (GLFW_KEY_Z = 90)
    // Q is at the A position on QWERTY (GLFW_KEY_Q = 81)
    Z = 87, Q = 65, S = 83, D = 68,

    // Actions — AZERTY layout
    // A is at the Q position on QWERTY (GLFW_KEY_A = 65)
    A      = 81,   // Lean left
    E      = 69,   // Lean right
    //W      = 87,   // Kept for reference (unused in AZERTY mode)

    Space  = 32,   // Jump
    LShift = 340,  // Sprint
    LCtrl  = 341,  // Crouch / slide

    // UI
    F1     = 290,  // Toggle editor overlay
    Escape = 256,

    // Misc
    R = 82, F = 70, G = 71, Tab = 258,
    D1 = 49, D2 = 50, D3 = 51, D4 = 52,

    _Count = 350   // Must be last — sets array size
};

// ── The key bound to "interact" — change here to remap globally
// Every system that needs to display or check the interact key
// should use this constant rather than Key::E directly.
constexpr Key INTERACT_KEY = Key::F;

// ── MouseButton enum ─────────────────────────────────────────
enum class MouseButton : int
{
    Left   = 0,
    Right  = 1,
    Middle = 2,
    _Count = 3
};

// ── Input ─────────────────────────────────────────────────────
class Input
{
public:
    Input() = default;

    void Init(GLFWwindow* window);  ///< Register GLFW callbacks
    void Update();                  ///< Call once per frame, before any queries

    // ── Keyboard ─────────────────────────────────────────────
    bool IsKeyHeld        (Key k) const;  ///< True every frame the key is down
    bool IsKeyJustPressed (Key k) const;  ///< True only on the frame it goes down
    bool IsKeyJustReleased(Key k) const;  ///< True only on the frame it goes up

    // ── Mouse ─────────────────────────────────────────────────
    bool      IsButtonHeld       (MouseButton b) const;
    bool      IsButtonJustPressed(MouseButton b) const;
    glm::vec2 GetMousePos  () const { return m_mousePos;    }
    glm::vec2 GetMouseDelta() const { return m_mouseDelta;  }
    float     GetScrollDelta()const { return m_scrollDelta; }

    // ── Utilities ─────────────────────────────────────────────
    /// Human-readable name for a key (e.g. Key::E → "E").
    /// Used by the HUD to dynamically display keybind prompts.
    static const char* GetKeyName(Key k);

private:
    static void ScrollCallback(GLFWwindow*, double xOff, double yOff);

    GLFWwindow* m_window = nullptr;

    // Two state arrays — m_cur flips each Update().
    // m_keys[m_cur]   = this frame's snapshot
    // m_keys[1-m_cur] = last frame's snapshot
    std::array<int, static_cast<int>(Key::_Count)>         m_keys[2]    = {};
    std::array<int, static_cast<int>(MouseButton::_Count)> m_buttons[2] = {};
    int m_cur = 0;

    glm::vec2 m_mousePos    = {};
    glm::vec2 m_mousePrev   = {};
    glm::vec2 m_mouseDelta  = {};
    float     m_scrollDelta = 0.f;
    float     m_scrollAccum = 0.f;
};
