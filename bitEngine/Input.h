#pragma once

// ============================================================
//  Input.h  —  Keyboard, Mouse & Controller Input
// ============================================================
//  Abstracts GLFW input into a frame-based polling API.
//  Every frame, Update() snapshots the raw GLFW state and
//  computes per-frame deltas (just-pressed, just-released,
//  mouse delta) so callers never need raw GLFW callbacks.
//
//  Key enum maps 1-to-1 with GLFW_KEY_* constants for zero
//  overhead — just cast to int.
// ============================================================

#include <array>
#include <glm/glm.hpp>

struct GLFWwindow;

// ── Key enum ─────────────────────────────────────────────────
// Values match GLFW_KEY_* so we can cast without a lookup table.
enum class Key : int
{
    // ── AZERTY movement keys ─────────────────────────────────
    // Physical layout (AZERTY):  A Z E R T Y  /  Q S D F G H
    // Z = forward   (QWERTY W position, GLFW_KEY_Z = 90)
    // Q = left      (QWERTY A position, GLFW_KEY_Q = 81)
    // S = backward  (same position on both layouts)
    // D = right     (same position on both layouts)
    Z = 90, Q = 81, S = 83, D = 68,

    // ── AZERTY lean / action keys ────────────────────────────
    // A = lean left   (QWERTY Q position, GLFW_KEY_A = 65)
    // E = lean right + interact (same position on both layouts)
    A      = 65,   // Lean left
    E      = 69,   // Lean right / Interact
    W      = 87,   // (unused in AZERTY mode, kept for reference)

    Space  = 32,   // Jump / vault
    LShift = 340,  // Sprint
    LCtrl  = 341,  // Crouch / slide

    // ── UI / debug ───────────────────────────────────────────
    F1     = 290,  // Toggle editor overlay
    Escape = 256,

    // Misc
    R = 82, F = 70, G = 71, Tab = 258,
    // Digits
    D1 = 49, D2 = 50, D3 = 51, D4 = 52,
    // Count — keep last
    _Count = 350
};

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

    /// Register GLFW callbacks with the given window.
    void Init(GLFWwindow* window);

    /// Snapshot input state for the current frame.
    /// Call once per frame before any IsKey* queries.
    void Update();

    // ── Keyboard ─────────────────────────────────────────────
    bool IsKeyHeld(Key k)        const;  ///< Held this frame
    bool IsKeyJustPressed(Key k) const;  ///< Went down this frame
    bool IsKeyJustReleased(Key k)const;  ///< Went up this frame

    // ── Mouse ─────────────────────────────────────────────────
    bool IsButtonHeld(MouseButton b)        const;
    bool IsButtonJustPressed(MouseButton b) const;

    /// Mouse position in window-space pixels (top-left origin)
    glm::vec2 GetMousePos()   const { return m_mousePos;   }

    /// Mouse movement since last frame (raw, unscaled)
    glm::vec2 GetMouseDelta() const { return m_mouseDelta; }

    /// Scroll wheel accumulator (Y axis, in detents)
    float     GetScrollDelta() const { return m_scrollDelta; }

private:
    // ── GLFW scroll callback (static, routed via user pointer) ─
    static void ScrollCallback(GLFWwindow*, double xOff, double yOff);

    GLFWwindow* m_window = nullptr;

    // Key states: [0] = current frame, [1] = previous frame
    std::array<int, static_cast<int>(Key::_Count)>         m_keys[2]   = {};
    std::array<int, static_cast<int>(MouseButton::_Count)> m_buttons[2]= {};

    glm::vec2 m_mousePos   = {0.f, 0.f};
    glm::vec2 m_mousePrev  = {0.f, 0.f};
    glm::vec2 m_mouseDelta = {0.f, 0.f};
    float     m_scrollDelta = 0.f;
    float     m_scrollAccum = 0.f;  // Written by callback

    int  m_frame = 0;  ///< Toggles between 0 and 1 each Update()
};
