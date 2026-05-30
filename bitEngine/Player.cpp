// ============================================================
//  Player.cpp
// ============================================================

#include "Player.h"
#include <algorithm>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Input.h"
#include "World.h"

// Player AABB dimensions (metres)
static constexpr float kPlayerHalfW = 0.30f;   // half-width X and Z
static constexpr float kPlayerHeight = 1.80f;  // total standing height

// ── Init ──────────────────────────────────────────────────────
void Player::Init()
{
    m_position      = {0.f, 0.f, 0.f};
    m_velocity      = {0.f, 0.f, 0.f};
    m_state         = MoveState::Standing;
    m_currentEyeH   = m_stats.eyeHeight;
    m_onGround      = true;
    m_camera.SetLean(0.f);
}

// ── Update ────────────────────────────────────────────────────
void Player::Update(float dt, const Input& input, World& world)
{
    // NOTE: HandleActions (interactions) is intentionally NOT called here.
    // Player::Update runs inside the fixed-timestep accumulator loop and
    // may execute multiple times per real frame with the same Input snapshot.
    // IsKeyJustPressed would return true in every iteration, firing onInteract
    // multiple times - toggling doors/lamps back to their original state and
    // making it appear that nothing happened.
    // Interactions are handled in Player::ProcessEvents, which is called
    // exactly once per real frame from Engine::ProcessInput.

    bool containerOpen = world.HasOpenContainer();

    UpdateMoveState(input, dt);

    if (!containerOpen)
    {
        HandleMouseLook(input, dt);
        HandleMovement(input, dt);
    }
    else
    {
        // Reset lean so it damps to zero while UI is open
        m_camera.SetLean(0.f);
    }

    ApplyGravity(dt);
    ResolveCollision(world);
    UpdateCameraHeight(dt, containerOpen);
    CheckTriggers(world);
    ResolveCollision(world);
    UpdateCameraHeight(dt, containerOpen);
    CheckTriggers(world);
}

// ── ProcessEvents ──────────────────────────────────────────────
// Called ONCE per real frame from Engine::ProcessInput, after
// Input::Update() has refreshed the key snapshot.
// Safe to use IsKeyJustPressed here because the snapshot
// cannot change between this call and the next real frame.
void Player::ProcessEvents(const Input& input, World& world)
{
    bool containerOpen = world.HasOpenContainer();
    if (!containerOpen)
        HandleActions(input, world);
}

// ── UpdateMoveState ───────────────────────────────────────────
void Player::UpdateMoveState(const Input& input, float dt)
{
    bool sprint = input.IsKeyHeld(Key::LShift);
    bool crouch = input.IsKeyHeld(Key::LCtrl);

    // AZERTY: Z=forward, Q=left, S=back, D=right
    bool moving = input.IsKeyHeld(Key::Z) || input.IsKeyHeld(Key::S)
               || input.IsKeyHeld(Key::Q) || input.IsKeyHeld(Key::D);

    // ── Slide: sprint + crouch while moving on ground ─────────
    if (m_state == MoveState::Sprinting && crouch && moving && m_onGround)
    {
        m_state      = MoveState::Sliding;
        m_slideTimer = 0.6f;
    }

    // ── Slide timeout ─────────────────────────────────────────
    if (m_state == MoveState::Sliding)
    {
        m_slideTimer -= dt;
        if (m_slideTimer <= 0.f)
            m_state = crouch ? MoveState::Crouching : MoveState::Standing;
    }

    // ── Air state ─────────────────────────────────────────────
    if (!m_onGround && m_state != MoveState::InAir &&
        m_state != MoveState::Sliding)
        m_state = MoveState::InAir;

    if (m_onGround && m_state == MoveState::InAir)
        m_state = MoveState::Standing;

    // ── Ground transitions ────────────────────────────────────
    if (m_onGround && m_state != MoveState::Sliding)
    {
        if (crouch)
            m_state = MoveState::Crouching;
        else if (sprint && moving)
            m_state = MoveState::Sprinting;
        else
            m_state = MoveState::Standing;
    }
}

// ── HandleMouseLook ───────────────────────────────────────────
void Player::HandleMouseLook(const Input& input, float dt)
{
    (void)dt;
    glm::vec2 delta = input.GetMouseDelta();
    m_camera.Rotate(delta.x, delta.y, m_stats.mouseSensitivity);
}

// ── HandleMovement ────────────────────────────────────────────
void Player::HandleMovement(const Input& input, float dt)
{
    // Choose speed from current state
    float targetSpeed = m_stats.walkSpeed;
    switch (m_state)
    {
        case MoveState::Sprinting:  targetSpeed = m_stats.sprintSpeed;  break;
        case MoveState::Crouching:  targetSpeed = m_stats.crouchSpeed;  break;
        case MoveState::Sliding:    targetSpeed = m_stats.slideSpeed;   break;
        default: break;
    }

    // Horizontal input
    glm::vec3 fwd   = m_camera.GetForward();
    fwd.y = 0.f;
    if (glm::length(fwd) > 0.001f) fwd = glm::normalize(fwd);

    glm::vec3 right = m_camera.GetRight();
    right.y = 0.f;
    if (glm::length(right) > 0.001f) right = glm::normalize(right);

    glm::vec3 moveDir = {0.f, 0.f, 0.f};
    // AZERTY layout
    if (input.IsKeyHeld(Key::Z)) moveDir += fwd;
    if (input.IsKeyHeld(Key::S)) moveDir -= fwd;
    if (input.IsKeyHeld(Key::D)) moveDir += right;
    if (input.IsKeyHeld(Key::Q)) moveDir -= right;

    if (glm::length(moveDir) > 0.001f)
        moveDir = glm::normalize(moveDir);

    // Horizontal velocity
    // On the ground: snap to input × targetSpeed (responsive).
    // In the air:    preserve the horizontal speed set at jump time.
    //               Air control nudges direction but is capped at m_airSpeedCap.
    if (m_state != MoveState::Sliding)
    {
        if (m_onGround)
        {
            m_velocity.x = moveDir.x * targetSpeed;
            m_velocity.z = moveDir.z * targetSpeed;
        }
        else
        {
            // Air steering: nudge toward input direction
            m_velocity.x += moveDir.x * m_stats.airControl * dt;
            m_velocity.z += moveDir.z * m_stats.airControl * dt;

            // Cap to the speed captured at jump time
            float hspd = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
            if (hspd > m_airSpeedCap && m_airSpeedCap > 0.f)
            {
                float scale = m_airSpeedCap / hspd;
                m_velocity.x *= scale;
                m_velocity.z *= scale;
            }
        }
    }
    m_speed = glm::length(glm::vec3(m_velocity.x, 0.f, m_velocity.z));

    // ── Lean (A = left, E = right) ────────────────────────────
    // Available when standing, crouching, or in air.
    // Disabled during sprint and slide.
    float lean = 0.f;
    if (m_state != MoveState::Sprinting && m_state != MoveState::Sliding)
    {
        if (input.IsKeyHeld(Key::A)) lean = -1.f;
        if (input.IsKeyHeld(Key::E)) lean =  1.f;
    }
    m_camera.SetLean(lean * 12.f);

    // ── Jump ──────────────────────────────────────────────────
    // m_jumpConsumed prevents the jump from firing more than once per key
    // press even when IsKeyJustPressed stays true across multiple physics
    // sub-steps (fixed-step accumulator) or when ResolveCollision briefly
    // resets m_onGround=true mid-frame after the initial jump displacement.
    if (!input.IsKeyHeld(Key::Space))
        m_jumpConsumed = false;  // Key released - reset for next press

    if (input.IsKeyJustPressed(Key::Space) && m_onGround
        && !m_jumpConsumed
        && m_state != MoveState::Crouching
        && m_state != MoveState::Sliding)
    {
        float jumpVel  = std::sqrt(2.f * std::abs(m_stats.gravity) * m_stats.jumpHeight);
        // Retain only a fraction of the horizontal velocity at launch
        m_velocity.x  *= m_stats.jumpVelocityRetain;
        m_velocity.z  *= m_stats.jumpVelocityRetain;
        m_velocity.y   = jumpVel;
        m_onGround     = false;
        m_jumpConsumed = true;
        m_state        = MoveState::InAir;
        // Lock in the retained horizontal speed as the in-air cap
        m_airSpeedCap  = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
        m_airSpeedCap  = std::max(m_airSpeedCap, m_stats.walkSpeed);
    }

    // Apply horizontal displacement
    m_position.x += m_velocity.x * dt;
    m_position.z += m_velocity.z * dt;
}

// ── ApplyGravity ──────────────────────────────────────────────
void Player::ApplyGravity(float dt)
{
    m_velocity.y += m_stats.gravity * dt;
    m_position.y += m_velocity.y * dt;
}

// ── ResolveCollision ──────────────────────────────────────────
// Two passes:
//   1. Floor plane at y = 0 (always present).
//   2. All solid CollisionComponent boxes in the world.
void Player::ResolveCollision(World& world)
{
    // ── 1. Floor ──────────────────────────────────────────────
    if (m_position.y < 0.f)
    {
        m_position.y   = 0.f;
        m_velocity.y   = 0.f;
        m_onGround     = true;
        m_jumpConsumed = false;
    }
    else
    {
        if (m_position.y > 0.01f) m_onGround = false;
    }

    // ── 2. AABB boxes ─────────────────────────────────────────
    // Player capsule approximated as an axis-aligned box:
    //   feet at m_position.y, head at m_position.y + kPlayerHeight
    //   XZ centred on m_position.xz with radius kPlayerHalfW.
    float eyeH = (m_state == MoveState::Crouching)
                 ? m_stats.crouchHeight : kPlayerHeight;

    glm::vec3 pMin = m_position + glm::vec3(-kPlayerHalfW, 0.f,    -kPlayerHalfW);
    glm::vec3 pMax = m_position + glm::vec3( kPlayerHalfW, eyeH,    kPlayerHalfW);

    for (auto& rec : world.GetAllRecords())
    {
        if (!rec.active || !rec.collision || !rec.transform) continue;
        if (!rec.collision->solid) continue;

        const glm::vec3& ctr  = rec.transform->position;
        // Scale the local half-extents
        glm::vec3 localHalf = rec.collision->halfExtents * rec.transform->scale;

        // Rotate the half-extents by the entity's rotation to get a
        // world-space AABB.  For a rotated OBB we compute the enclosing
        // AABB by transforming each basis vector and summing the absolutes
        // (the standard OBB→AABB expansion formula).
        glm::mat3 rot(rec.transform->rotation);
        glm::vec3 half;
        half.x = std::abs(rot[0].x) * localHalf.x
               + std::abs(rot[1].x) * localHalf.y
               + std::abs(rot[2].x) * localHalf.z;
        half.y = std::abs(rot[0].y) * localHalf.x
               + std::abs(rot[1].y) * localHalf.y
               + std::abs(rot[2].y) * localHalf.z;
        half.z = std::abs(rot[0].z) * localHalf.x
               + std::abs(rot[1].z) * localHalf.y
               + std::abs(rot[2].z) * localHalf.z;

        glm::vec3 bMin = ctr - half;
        glm::vec3 bMax = ctr + half;

        // Broad phase: skip if no overlap on any axis
        if (pMax.x <= bMin.x || pMin.x >= bMax.x) continue;
        if (pMax.y <= bMin.y || pMin.y >= bMax.y) continue;
        if (pMax.z <= bMin.z || pMin.z >= bMax.z) continue;

        // Penetration depth on each axis
        float ox = std::min(pMax.x - bMin.x, bMax.x - pMin.x);
        float oy = std::min(pMax.y - bMin.y, bMax.y - pMin.y);
        float oz = std::min(pMax.z - bMin.z, bMax.z - pMin.z);

        // Push out along the axis of least penetration
        if (ox <= oy && ox <= oz)
        {
            // X axis
            if (pMax.x - bMin.x < bMax.x - pMin.x)
                m_position.x -= ox;   // push left
            else
                m_position.x += ox;   // push right
            m_velocity.x = 0.f;
        }
        else if (oz <= ox && oz <= oy)
        {
            // Z axis
            if (pMax.z - bMin.z < bMax.z - pMin.z)
                m_position.z -= oz;
            else
                m_position.z += oz;
            m_velocity.z = 0.f;
        }
        else
        {
            // Y axis - could be landing on top or hitting ceiling
            if (pMax.y - bMin.y < bMax.y - pMin.y)
            {
                m_position.y -= oy;   // push down (ceiling hit)
                if (m_velocity.y > 0.f) m_velocity.y = 0.f;
            }
            else
            {
                m_position.y += oy;   // push up (land on top)
                m_velocity.y  = 0.f;
                m_onGround     = true;
                m_jumpConsumed = false;
            }
        }

        // Recompute player AABB after each push-out so subsequent
        // boxes see the corrected position.
        pMin = m_position + glm::vec3(-kPlayerHalfW, 0.f,  -kPlayerHalfW);
        pMax = m_position + glm::vec3( kPlayerHalfW, eyeH,  kPlayerHalfW);
    }
}

// ── UpdateCameraHeight ────────────────────────────────────────
void Player::UpdateCameraHeight(float dt, bool suppressBob)
{
    float targetH = (m_state == MoveState::Crouching || m_state == MoveState::Sliding)
                    ? m_stats.crouchHeight : m_stats.eyeHeight;

    m_currentEyeH += (targetH - m_currentEyeH) * std::min(dt * 12.f, 1.f);

    // Zero speed when a UI popup is open so the bob damps out cleanly.
    float bobSpeed = suppressBob ? 0.f : m_speed;
    m_camera.UpdateHeadBob(bobSpeed, dt);
    m_camera.SetPosition(m_position + glm::vec3(0.f, m_currentEyeH, 0.f));
}

// ── HandleActions ─────────────────────────────────────────────
void Player::HandleActions(const Input& input, World& world)
{
    HandleInteraction(input, world);
}

// ── HandleInteraction ─────────────────────────────────────────
void Player::HandleInteraction(const Input& input, World& world)
{
    glm::vec3 eyePos = m_position + glm::vec3(0.f, m_currentEyeH, 0.f);
    EntityID  near   = world.FindNearestInteractable(eyePos, m_stats.interactRange);

    if (near != kNullEntity)
    {
        auto* ia = world.GetRecord(near)->interactable;
        m_interactPrompt = ia ? ia->promptText : "";

        // Use INTERACT_KEY from Input.h - single source of truth for the binding
        if (input.IsKeyJustPressed(INTERACT_KEY) && ia && ia->onInteract)
            ia->onInteract();
    }
    else
    {
        m_interactPrompt.clear();
    }
}

// ── CheckTriggers ─────────────────────────────────────────────
void Player::CheckTriggers(World& world)
{
    for (auto& rec : world.GetAllRecords())
    {
        if (!rec.active || !rec.trigger || !rec.transform) continue;
        auto& tri = *rec.trigger;
        const glm::vec3& tp = rec.transform->position;

        bool inside = (m_position.x >= tp.x - tri.halfExtents.x &&
                       m_position.x <= tp.x + tri.halfExtents.x &&
                       m_position.y >= tp.y - tri.halfExtents.y &&
                       m_position.y <= tp.y + tri.halfExtents.y &&
                       m_position.z >= tp.z - tri.halfExtents.z &&
                       m_position.z <= tp.z + tri.halfExtents.z);

        if (inside && !tri.triggered)
        {
            tri.triggered = true;
            if (tri.onEnter) tri.onEnter();
        }
        else if (!inside && tri.triggered)
        {
            tri.triggered = false;
            if (tri.onExit) tri.onExit();
        }
    }
}
