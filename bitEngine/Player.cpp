// ============================================================
//  Player.cpp  —  First-Person Player Controller
// ============================================================

#include "Player.h"
#include "Input.h"
#include "World.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

void Player::Init()
{
    m_position    = {0.f, 0.f, 3.f};
    m_velocity    = {0.f, 0.f, 0.f};
    m_currentEyeH = m_stats.eyeHeight;
    m_state       = MoveState::Standing;
    m_camera      = Camera(m_position + glm::vec3(0.f, m_currentEyeH, 0.f));
}

void Player::Update(float dt, const Input& input, World& world)
{
    m_interactPrompt.clear();
    HandleMouseLook(input, dt);
    UpdateMoveState(input, dt);
    HandleMovement(input, dt);
    ApplyGravity(dt);
    ResolveCollision(world);
    UpdateCameraHeight(dt);
    HandleInteraction(input, world);
    CheckTriggers(world);

    float horizSpeed = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
    m_speed = horizSpeed;
    m_camera.UpdateHeadBob(horizSpeed, dt);
}

void Player::HandleMouseLook(const Input& input, float /*dt*/)
{
    glm::vec2 delta = input.GetMouseDelta();
    m_camera.Rotate(delta.x, delta.y, m_stats.mouseSensitivity);
}

void Player::UpdateMoveState(const Input& input, float dt)
{
    bool sprint = input.IsKeyHeld(Key::LShift);
    bool crouch = input.IsKeyHeld(Key::LCtrl);
    bool moving = input.IsKeyHeld(Key::W) || input.IsKeyHeld(Key::S)
               || input.IsKeyHeld(Key::A) || input.IsKeyHeld(Key::D);

    // Slide: sprinting + crouch while moving on ground
    if (m_state == MoveState::Sprinting && crouch && moving && m_onGround)
    {
        m_state      = MoveState::Sliding;
        m_slideTimer = 0.6f;
        glm::vec3 fwd = m_camera.GetForward(); fwd.y = 0.f;
        if (glm::length(fwd) > 0.001f) fwd = glm::normalize(fwd);
        m_velocity.x = fwd.x * m_stats.slideSpeed;
        m_velocity.z = fwd.z * m_stats.slideSpeed;
        return;
    }

    // Slide countdown
    if (m_state == MoveState::Sliding)
    {
        m_slideTimer -= dt;
        if (m_slideTimer <= 0.f)
            m_state = crouch ? MoveState::Crouching : MoveState::Standing;
        return;
    }

    if (!m_onGround)      { m_state = MoveState::InAir;    return; }
    if (crouch)           { m_state = MoveState::Crouching; return; }
    if (sprint && moving) { m_state = MoveState::Sprinting; return; }
    m_state = MoveState::Standing;
}

void Player::HandleMovement(const Input& input, float dt)
{
    if (m_state == MoveState::Sliding) return; // velocity already set

    float targetSpeed = m_stats.walkSpeed;
    if (m_state == MoveState::Sprinting) targetSpeed = m_stats.sprintSpeed;
    if (m_state == MoveState::Crouching) targetSpeed = m_stats.crouchSpeed;

    glm::vec3 fwd   = m_camera.GetForward();  fwd.y   = 0.f;
    glm::vec3 right = m_camera.GetRight();    right.y = 0.f;
    if (glm::length(fwd)   > 0.001f) fwd   = glm::normalize(fwd);
    if (glm::length(right) > 0.001f) right = glm::normalize(right);

    glm::vec3 moveDir = {0.f, 0.f, 0.f};
    if (input.IsKeyHeld(Key::W)) moveDir += fwd;
    if (input.IsKeyHeld(Key::S)) moveDir -= fwd;
    if (input.IsKeyHeld(Key::D)) moveDir += right;
    if (input.IsKeyHeld(Key::A)) moveDir -= right;
    if (glm::length(moveDir) > 0.001f) moveDir = glm::normalize(moveDir);

    // Lean: Q = left, E = right (only when stationary to disambiguate from interact)
    float lean = 0.f;
    if (m_state != MoveState::Sprinting)
    {
        if (input.IsKeyHeld(Key::Q)) lean = -1.f;
        if (input.IsKeyHeld(Key::E) && glm::length(moveDir) < 0.001f) lean = 1.f;
    }
    m_camera.SetLean(lean * 12.f);

    // Smooth acceleration / deceleration
    const float accel = 30.f, decel = 20.f;
    glm::vec3 targetVel = moveDir * targetSpeed;
    float factor = (glm::length(moveDir) > 0.001f) ? accel : decel;
    m_velocity.x += (targetVel.x - m_velocity.x) * factor * dt;
    m_velocity.z += (targetVel.z - m_velocity.z) * factor * dt;

    // Jump
    if (input.IsKeyJustPressed(Key::Space) && m_onGround
        && m_state != MoveState::Crouching)
    {
        m_velocity.y = std::sqrt(2.f * std::abs(m_stats.gravity) * m_stats.jumpHeight);
        m_onGround   = false;
        m_state      = MoveState::InAir;
    }

    m_position += m_velocity * dt;
}

void Player::ApplyGravity(float dt)
{
    if (!m_onGround)
        m_velocity.y += m_stats.gravity * dt;
    else
        m_velocity.y = 0.f;
}

void Player::ResolveCollision(World& /*world*/)
{
    // Simple floor plane at y = 0; replace with mesh ray-cast for real levels
    if (m_position.y < 0.f)
    {
        m_position.y = 0.f;
        m_velocity.y = 0.f;
        m_onGround   = true;
    }
    else
    {
        m_onGround = (m_position.y < 0.02f && m_velocity.y <= 0.f);
    }
}

void Player::UpdateCameraHeight(float dt)
{
    bool low = (m_state == MoveState::Crouching || m_state == MoveState::Sliding);
    float targetH = low ? m_stats.crouchHeight : m_stats.eyeHeight;
    m_currentEyeH += (targetH - m_currentEyeH) * 12.f * dt;
    m_camera.SetPosition(m_position + glm::vec3(0.f, m_currentEyeH, 0.f));
}

void Player::HandleInteraction(const Input& input, World& world)
{
    glm::vec3 origin = m_camera.GetPosition();
    glm::vec3 fwd    = m_camera.GetForward();

    EntityID target = world.FindNearestInteractable(origin, m_stats.interactRange);
    if (target == kNullEntity) return;

    auto* ia = world.GetInteractable(target);
    auto* tr = world.GetTransform(target);
    if (!ia || !tr) return;

    // Only interact if the object is roughly in front of the camera
    glm::vec3 toObj = glm::normalize(tr->position - origin);
    if (glm::dot(toObj, fwd) < 0.4f) return;

    m_interactPrompt = ia->promptText;
    if (input.IsKeyJustPressed(Key::E) && ia->onInteract)
        ia->onInteract();
}

void Player::CheckTriggers(World& world)
{
    for (auto& rec : const_cast<std::vector<EntityRecord>&>(world.GetAllRecords()))
    {
        if (!rec.active || !rec.trigger || !rec.transform) continue;
        auto& tri = *rec.trigger;
        auto& pos = rec.transform->position;
        auto& ext = tri.halfExtents;

        bool inside = (m_position.x >= pos.x - ext.x && m_position.x <= pos.x + ext.x)
                   && (m_position.y >= pos.y - ext.y && m_position.y <= pos.y + ext.y)
                   && (m_position.z >= pos.z - ext.z && m_position.z <= pos.z + ext.z);

        if (inside && !tri.triggered)
        { tri.triggered = true;  if (tri.onEnter) tri.onEnter(); }
        else if (!inside && tri.triggered)
        { tri.triggered = false; if (tri.onExit)  tri.onExit();  }
    }
}
