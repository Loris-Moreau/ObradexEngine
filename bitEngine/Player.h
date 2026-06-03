#pragma once

// Player.h - First-person player controller.
//
// Implements a Deus Ex / Assassin's Creed Syndicate-style movement model.
//
// Movement states:
//   Standing   - default, full height
//   Crouching  - slower, reduced height
//   Sprinting  - fast, no lean available
//   Sliding    - brief burst after sprint + crouch input
//   InAir      - jumping or falling
//   Vaulting   - stub for auto-climbing low ledges
//
// Physics:
//   Kinematic velocity integration + AABB vs world collision.
//   No rigid-body solver. Gravity is explicit.
//
// Input is processed in two places:
//   Update()        - movement, gravity, collision (fixed-step, N times per frame)
//   ProcessEvents() - interactions (once per real frame, safe for IsKeyJustPressed)

#include <string>
#include <glm/glm.hpp>

#include "Camera.h"

class Input;
class World;

enum class MoveState
{
    Standing,
    Crouching,
    Sprinting,
    Sliding,
    InAir,
    Vaulting
};

// Runtime-tunable movement parameters. Exposed in the editor Player tab.
struct PlayerStats
{
    float walkSpeed          = 2.25f;   // m/s
    float sprintSpeed        = 8.0f;    // m/s
    float crouchSpeed        = walkSpeed / 2.0f;
    float slideSpeed         = 10.0f;   // Initial m/s burst
    float jumpHeight         = 1.2f;    // metres
    float gravity            = -12.0f;  // m/s^2
    float mouseSensitivity   = 0.12f;
    float eyeHeight          = 1.8f;    // Standing eye height (m)
    float crouchHeight       = 0.85f;   // Crouching eye height (m)
    float interactRange      = 2.25f;   // Maximum interact reach (m)
    float airControl         = 4.0f;    // m/s^2 horizontal nudge while airborne
    float jumpVelocityRetain = 0.8f;    // Fraction of horizontal speed kept at jump (0 to 1)
    float leanDistance       = 0.25f;    // Max lateral eye offset at full lean (m)
};

class Player
{
public:
    Player()  = default;
    ~Player() = default;

    void Init();

    // Physics tick. Called N times per real frame inside the fixed-step loop.
    void Update(float dt, const Input& input, World& world);

    // Interaction events. Called exactly once per real frame from Engine::ProcessInput.
    void ProcessEvents(const Input& input, World& world);

    const Camera&    GetCamera()       const { return m_camera;   }
    Camera&          GetCamera()             { return m_camera;   }
    const glm::vec3& GetPosition()     const { return m_position; }
    MoveState        GetMoveState()    const { return m_state;    }
    float            GetSpeed()        const { return m_speed;    }
    PlayerStats&     GetStats()              { return m_stats;    }

    // Prompt text of the nearest interactable in range. Empty if none.
    const std::string& GetInteractPrompt() const { return m_interactPrompt; }

    bool IsHidden() const { return m_isHidden; }

private:
    void HandleMouseLook  (const Input& input, float dt);
    void HandleMovement   (const Input& input, float dt);
    void HandleActions    (const Input& input, World& world);
    void HandleInteraction(const Input& input, World& world);
    void UpdateMoveState  (const Input& input, float dt);
    void ApplyGravity     (float dt);
    void UpdateCameraHeight(float dt, bool suppressBob = false);
    void CheckTriggers    (World& world);
    void ResolveCollision (World& world);

    glm::vec3   m_position = {0.f, 0.f, 0.f};
    glm::vec3   m_velocity = {0.f, 0.f, 0.f};
    Camera      m_camera;
    PlayerStats m_stats;
    MoveState   m_state    = MoveState::Standing;

    float m_speed       = 0.f;
    float m_currentEyeH = 1.7f;
    float m_slideTimer  = 0.f;
    float m_vaultTimer  = 0.f;
    float m_airSpeedCap = 0.f;    // Horizontal speed at jump time; used as in-air cap

    bool m_onGround     = true;
    bool m_jumpConsumed = false;  // Prevents re-firing the jump within the same key press
    bool m_isHidden     = false;

    std::string m_interactPrompt;
};
