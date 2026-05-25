#pragma once

// ============================================================
//  Player.h  —  First-Person Player Controller
// ============================================================
//  Implements the Deus Ex / Assassin's Creed Syndicate
//  movement model:
//
//  Movement modes (state machine):
//    Standing   — normal speed, full height
//    Crouching  — slower, half height, quieter footsteps
//    Sprinting  — fast, standing only, no lean
//    Sliding    — brief burst after sprint+crouch
//    Vaulting   — auto-climb low ledges (Syndicate-style)
//
//  Abilities (Deus Ex style):
//    Lean (Q/E)          — peek around corners
//    Interaction (E)     — pick up / open / activate
//    Crouch-slide (LCtrl)— momentum preserved briefly
//
//  Physics:
//    Simple kinematic controller — no rigid body, just
//    velocity integration + AABB vs world collision.
//    Gravity is applied; jumping is supported.
// ============================================================

#include <glm/glm.hpp>
#include <string>

#include "Camera.h"

class Input;
class World;

// ── Player movement state ─────────────────────────────────────
enum class MoveState
{
    Standing,
    Crouching,
    Sprinting,
    Sliding,
    InAir,
    Vaulting
};

// ── Player stats (tweak-able from the editor) ─────────────────
struct PlayerStats
{
    float walkSpeed       = 2.25f;   // m/s
    float sprintSpeed     = 8.0f;   // m/s
    float crouchSpeed     = walkSpeed / 2.0f;   // m/s
    float slideSpeed      = 10.0f;  // m/s initial
    float jumpHeight      = 1.2f;   // metres
    float gravity         = -12.0f;  // m/s²
    float mouseSensitivity= 0.12f;
    float eyeHeight       = 1.8f;   // Standing eye height (m)
    float crouchHeight    = 0.85f;  // Crouching eye height (m)
    float interactRange   = 2.25f;   // Reach distance (m)
};

// ── Player ───────────────────────────────────────────────────
class Player
{
public:
    Player()  = default;
    ~Player() = default;

    // ── Lifecycle ─────────────────────────────────────────────
    void Init();
    void Update      (float dt, const Input& input, World& world); ///< Physics tick — called N times per real frame
    void ProcessEvents(             const Input& input, World& world); ///< Interaction events — called ONCE per real frame

    // ── Accessors ─────────────────────────────────────────────
    const Camera&     GetCamera()    const { return m_camera;   }
    Camera&           GetCamera()          { return m_camera;   }
    const glm::vec3&  GetPosition()  const { return m_position; }
    MoveState         GetMoveState() const { return m_state;    }
    float             GetSpeed()     const { return m_speed;    }
    PlayerStats&      GetStats()           { return m_stats;    }

    /// The prompt text of the interactable the player is looking at.
    /// Empty string if none in range.
    const std::string& GetInteractPrompt() const { return m_interactPrompt; }

    /// True if the player is currently hidden (crouching in shadow).
    bool IsHidden() const { return m_isHidden; }

private:
    // ── Per-frame updates ─────────────────────────────────────
    void HandleMouseLook(const Input& input, float dt);
    void HandleMovement(const Input& input, float dt);
    void HandleActions(const Input& input, World& world);
    void HandleInteraction(const Input& input, World& world);
    void UpdateMoveState(const Input& input, float dt);  ///< Evaluate and transition movement states
    void ApplyGravity(float dt);
    void UpdateCameraHeight(float dt, bool suppressBob = false);
    void CheckTriggers(World& world);

    // ── Physics helpers ───────────────────────────────────────
    void ResolveCollision(World& world);

    // ── State ─────────────────────────────────────────────────
    glm::vec3   m_position  = {0.f, 0.f, 0.f};
    glm::vec3   m_velocity  = {0.f, 0.f, 0.f};
    Camera      m_camera;
    PlayerStats m_stats;
    MoveState   m_state     = MoveState::Standing;

    float m_speed           = 0.f;
    float m_currentEyeH     = 1.7f;
    float m_slideTimer      = 0.f;
    float m_vaultTimer      = 0.f;
    bool  m_onGround        = true;
    bool  m_isHidden        = false;

    std::string m_interactPrompt;
};
