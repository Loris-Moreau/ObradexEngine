// Player.cpp - First-person player controller.

#include "Player.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Input.h"
#include "World.h"

// Player AABB half-width on X and Z, and full standing height.
static constexpr float kPlayerHalfW  = 0.30f;
static constexpr float kPlayerHeight = 1.80f;

void Player::Init()
{
    m_position    = {0.f, 0.f, 0.f};
    m_velocity    = {0.f, 0.f, 0.f};
    m_state       = MoveState::Standing;
    m_currentEyeH = m_stats.eyeHeight;
    m_onGround    = true;
    m_camera.SetLean(0.f);
}

void Player::Update(float dt, const Input& input, World& world)
{
    // HandleActions (interactions) is deliberately NOT called here.
    // Update() runs N times per real frame inside the fixed-step accumulator.
    // With the same input snapshot each sub-step, IsKeyJustPressed would be
    // true on every iteration, firing onInteract multiple times and toggling
    // doors/lamps back to their original state. Interactions go through
    // ProcessEvents() which is called exactly once per real frame.

    bool containerOpen = world.HasOpenContainer();

    UpdateMoveState(input, dt);

    if (!containerOpen)
    {
        HandleMouseLook(input, dt);
        HandleMovement(input, dt);
    }
    else
    {
        // Damp lean to zero while a UI popup is blocking movement.
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

// Called once per real frame from Engine::ProcessInput, after Input::Update().
// Safe to use IsKeyJustPressed here because the snapshot will not change
// again until the next real frame.
void Player::ProcessEvents(const Input& input, World& world)
{
    if (!world.HasOpenContainer())
        HandleActions(input, world);
}

void Player::UpdateMoveState(const Input& input, float dt)
{
    bool sprint = input.IsKeyHeld(Key::LShift);
    bool crouch = input.IsKeyHeld(Key::LCtrl);
    bool moving = input.IsKeyHeld(Key::Z) || input.IsKeyHeld(Key::S)
               || input.IsKeyHeld(Key::Q) || input.IsKeyHeld(Key::D);

    // Begin a slide when the player presses crouch while sprinting on the ground.
    if (m_state == MoveState::Sprinting && crouch && moving && m_onGround)
    {
        m_state      = MoveState::Sliding;
        m_slideTimer = 0.6f;
    }

    // End the slide when the timer expires.
    if (m_state == MoveState::Sliding)
    {
        m_slideTimer -= dt;
        if (m_slideTimer <= 0.f)
            m_state = crouch ? MoveState::Crouching : MoveState::Standing;
    }

    if (!m_onGround && m_state != MoveState::InAir && m_state != MoveState::Sliding)
        m_state = MoveState::InAir;

    if (m_onGround && m_state == MoveState::InAir)
        m_state = MoveState::Standing;

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

void Player::HandleMouseLook(const Input& input, float dt)
{
    (void)dt;
    glm::vec2 delta = input.GetMouseDelta();
    m_camera.Rotate(delta.x, delta.y, m_stats.mouseSensitivity);
}

void Player::HandleMovement(const Input& input, float dt)
{
    float targetSpeed = m_stats.walkSpeed;
    switch (m_state)
    {
        case MoveState::Sprinting: targetSpeed = m_stats.sprintSpeed; break;
        case MoveState::Crouching: targetSpeed = m_stats.crouchSpeed; break;
        case MoveState::Sliding:   targetSpeed = m_stats.slideSpeed;  break;
        default: break;
    }

    // Flatten camera forward/right to the horizontal plane for movement.
    glm::vec3 fwd = m_camera.GetForward();
    fwd.y = 0.f;
    if (glm::length(fwd) > 0.001f) fwd = glm::normalize(fwd);

    glm::vec3 right = m_camera.GetRight();
    right.y = 0.f;
    if (glm::length(right) > 0.001f) right = glm::normalize(right);

    glm::vec3 moveDir = {0.f, 0.f, 0.f};
    if (input.IsKeyHeld(Key::Z)) moveDir += fwd;
    if (input.IsKeyHeld(Key::S)) moveDir -= fwd;
    if (input.IsKeyHeld(Key::D)) moveDir += right;
    if (input.IsKeyHeld(Key::Q)) moveDir -= right;

    if (glm::length(moveDir) > 0.001f)
        moveDir = glm::normalize(moveDir);

    // On the ground: snap velocity to input * speed (immediate response).
    // In the air: steer with a small acceleration nudge so the jump's initial
    // horizontal speed is preserved rather than overwritten each frame.
    if (m_state != MoveState::Sliding)
    {
        if (m_onGround)
        {
            m_velocity.x = moveDir.x * targetSpeed;
            m_velocity.z = moveDir.z * targetSpeed;
        }
        else
        {
            m_velocity.x += moveDir.x * m_stats.airControl * dt;
            m_velocity.z += moveDir.z * m_stats.airControl * dt;

            // Clamp to the speed captured at jump time, not the current
            // state's target speed (which would be walkSpeed for InAir).
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

    // Lean: A = left, E = right. Disabled while sprinting or sliding.
    float lean = 0.f;
    if (m_state != MoveState::Sprinting && m_state != MoveState::Sliding)
    {
        if (input.IsKeyHeld(Key::A)) lean = -1.f;
        if (input.IsKeyHeld(Key::E)) lean =  1.f;
    }
    m_camera.SetLeanDistance(m_stats.leanDistance);
    m_camera.SetLean(lean * 12.f);

    // Jump.
    // m_jumpConsumed prevents the jump from firing more than once per key press.
    // Without it, the fixed-step accumulator may run multiple sub-steps in one
    // real frame with the same input snapshot. If ResolveCollision resets
    // m_onGround=true mid-frame (float snap on the floor plane), the jump
    // condition would be true on the next sub-step and fire a second time,
    // doubling the vertical velocity.
    if (!input.IsKeyHeld(Key::Space))
        m_jumpConsumed = false;

    if (input.IsKeyJustPressed(Key::Space) && m_onGround
        && !m_jumpConsumed
        && m_state != MoveState::Crouching
        && m_state != MoveState::Sliding)
    {
        float jumpVel  = std::sqrt(2.f * std::abs(m_stats.gravity) * m_stats.jumpHeight);
        // Apply velocity retention before recording the air cap so the cap
        // reflects the actual speed carried into the jump.
        m_velocity.x  *= m_stats.jumpVelocityRetain;
        m_velocity.z  *= m_stats.jumpVelocityRetain;
        m_velocity.y   = jumpVel;
        m_onGround     = false;
        m_jumpConsumed = true;
        m_state        = MoveState::InAir;
        m_airSpeedCap  = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
        m_airSpeedCap  = std::max(m_airSpeedCap, m_stats.walkSpeed);
    }

    m_position.x += m_velocity.x * dt;
    m_position.z += m_velocity.z * dt;
}

void Player::ApplyGravity(float dt)
{
    m_velocity.y += m_stats.gravity * dt;
    m_position.y += m_velocity.y * dt;
}

// Two-pass collision: floor plane at y=0, then all solid CollisionComponents.
void Player::ResolveCollision(World& world)
{
    // Floor plane.
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

    // Player AABB: feet at m_position.y, head at m_position.y + height.
    float eyeH = (m_state == MoveState::Crouching) ? m_stats.crouchHeight : kPlayerHeight;

    glm::vec3 pMin = m_position + glm::vec3(-kPlayerHalfW, 0.f,  -kPlayerHalfW);
    glm::vec3 pMax = m_position + glm::vec3( kPlayerHalfW, eyeH,  kPlayerHalfW);

    for (auto& rec : world.GetAllRecords())
    {
        if (!rec.active || !rec.collision || !rec.transform) continue;
        if (!rec.collision->solid) continue;

        const glm::vec3& ctr = rec.transform->position;
        glm::vec3 localHalf  = rec.collision->halfExtents * rec.transform->scale;

        // Expand the OBB to an enclosing AABB using the standard formula:
        // halfAABB[i] = sum of |R[col][i]| * localHalf[col].
        // This handles rotated solids (e.g. an open door swung 90 degrees).
        glm::mat3 rot(rec.transform->rotation);
        glm::vec3 half;
        half.x = std::abs(rot[0].x)*localHalf.x + std::abs(rot[1].x)*localHalf.y + std::abs(rot[2].x)*localHalf.z;
        half.y = std::abs(rot[0].y)*localHalf.x + std::abs(rot[1].y)*localHalf.y + std::abs(rot[2].y)*localHalf.z;
        half.z = std::abs(rot[0].z)*localHalf.x + std::abs(rot[1].z)*localHalf.y + std::abs(rot[2].z)*localHalf.z;

        glm::vec3 bMin = ctr - half;
        glm::vec3 bMax = ctr + half;

        // Broad-phase skip.
        if (pMax.x <= bMin.x || pMin.x >= bMax.x) continue;
        if (pMax.y <= bMin.y || pMin.y >= bMax.y) continue;
        if (pMax.z <= bMin.z || pMin.z >= bMax.z) continue;

        // Push out along the axis with the smallest penetration depth.
        float ox = std::min(pMax.x - bMin.x, bMax.x - pMin.x);
        float oy = std::min(pMax.y - bMin.y, bMax.y - pMin.y);
        float oz = std::min(pMax.z - bMin.z, bMax.z - pMin.z);

        if (ox <= oy && ox <= oz)
        {
            if (pMax.x - bMin.x < bMax.x - pMin.x) m_position.x -= ox;
            else                                     m_position.x += ox;
            m_velocity.x = 0.f;
        }
        else if (oz <= ox && oz <= oy)
        {
            if (pMax.z - bMin.z < bMax.z - pMin.z) m_position.z -= oz;
            else                                     m_position.z += oz;
            m_velocity.z = 0.f;
        }
        else
        {
            if (pMax.y - bMin.y < bMax.y - pMin.y)
            {
                m_position.y -= oy;  // Ceiling hit
                if (m_velocity.y > 0.f) m_velocity.y = 0.f;
            }
            else
            {
                m_position.y  += oy;  // Landed on top
                m_velocity.y   = 0.f;
                m_onGround     = true;
                m_jumpConsumed = false;
            }
        }

        // Recompute AABB after each push-out so subsequent boxes see the
        // corrected position.
        pMin = m_position + glm::vec3(-kPlayerHalfW, 0.f,  -kPlayerHalfW);
        pMax = m_position + glm::vec3( kPlayerHalfW, eyeH,  kPlayerHalfW);
    }
}

void Player::UpdateCameraHeight(float dt, bool suppressBob)
{
    float targetH = (m_state == MoveState::Crouching || m_state == MoveState::Sliding)
                    ? m_stats.crouchHeight : m_stats.eyeHeight;

    m_currentEyeH += (targetH - m_currentEyeH) * std::min(dt * 12.f, 1.f);

    // Pass zero speed when a UI popup is blocking movement so the bob damps
    // out instead of freezing mid-cycle.
    float bobSpeed = suppressBob ? 0.f : m_speed;
    m_camera.UpdateHeadBob(bobSpeed, dt);
    m_camera.SetPosition(m_position + glm::vec3(0.f, m_currentEyeH, 0.f));
}

void Player::HandleActions(const Input& input, World& world)
{
    HandleInteraction(input, world);
}

void Player::HandleInteraction(const Input& input, World& world)
{
    glm::vec3 eyePos = m_position + glm::vec3(0.f, m_currentEyeH, 0.f);
    EntityID  near   = world.FindNearestInteractable(eyePos, m_stats.interactRange);

    if (near != kNullEntity)
    {
        auto* ia = world.GetRecord(near)->interactable;
        m_interactPrompt = ia ? ia->promptText : "";

        if (input.IsKeyJustPressed(INTERACT_KEY) && ia && ia->onInteract)
            ia->onInteract();
    }
    else
    {
        m_interactPrompt.clear();
    }
}

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
