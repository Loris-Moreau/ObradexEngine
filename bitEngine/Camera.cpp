// ============================================================
//  Camera.cpp  —  First-Person Camera
// ============================================================

#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

// ── Constructor ───────────────────────────────────────────────
Camera::Camera(glm::vec3 position, float fovDeg)
    : m_position(position), m_fov(fovDeg)
{
    RecalcVectors();
}

// ── Rotate ────────────────────────────────────────────────────
void Camera::Rotate(float dx, float dy, float sensitivity)
{
    m_yaw   += dx * sensitivity;
    m_pitch -= dy * sensitivity;  // Invert Y so mouse-up = look up

    // Clamp pitch to avoid flipping over the top or bottom
    m_pitch = std::clamp(m_pitch, -89.f, 89.f);

    // Wrap yaw to [-180, 180] to avoid float overflow over time
    if (m_yaw >  180.f) m_yaw -= 360.f;
    if (m_yaw < -180.f) m_yaw += 360.f;

    RecalcVectors();
}

// ── Lean ──────────────────────────────────────────────────────
void Camera::SetLean(float amount)
{
    // Clamp to ±15° — enough to peek around corners
    m_leanTarget = std::clamp(amount, -15.f, 15.f);
}

// ── Head Bob ──────────────────────────────────────────────────
void Camera::UpdateHeadBob(float speed, float dt)
{
    const float kBobFrequency = 2.0f;   // Cycles per second
    const float kBobAmplY     = 0.04f;  // Vertical amplitude (m)
    const float kBobAmplX     = 0.02f;  // Horizontal amplitude (m)
    const float kSpeedThreshold = 0.5f; // Minimum speed before bob kicks in

    // Lerp lean toward target (smooth lean-in / lean-out)
    const float kLeanSpeed = 8.f;
    m_lean += (m_leanTarget - m_lean) * kLeanSpeed * dt;

    if (speed < kSpeedThreshold)
    {
        // Damp out the bob when the player stops
        m_bobY += (0.f - m_bobY) * 10.f * dt;
        m_bobX += (0.f - m_bobX) * 10.f * dt;
        return;
    }

    m_bobTime += dt * kBobFrequency * (speed / 5.f);  // Scale with speed

    // Lissajous-style figure-8 bob (feels natural for a walking human)
    m_bobY = std::sin(m_bobTime * 2.f * 3.14159f) * kBobAmplY;
    m_bobX = std::sin(m_bobTime       * 3.14159f) * kBobAmplX;
}

// ── GetView ───────────────────────────────────────────────────
glm::mat4 Camera::GetView() const
{
    // Eye position includes head-bob offsets
    glm::vec3 eye = m_position
                  + m_up    * m_bobY
                  + m_right * m_bobX;

    // Always use world-up (0,1,0) for lookAt so that the view matrix
    // never has an implicit roll component from pitch.  When the camera
    // pitches near ±90° glm::lookAt handles the singularity gracefully.
    glm::mat4 view = glm::lookAt(eye, eye + m_forward, glm::vec3(0.f, 1.f, 0.f));

    // Lean: roll around view-space Z (axis pointing into the screen).
    // Applied post-lookAt so the axis is in view-space, giving a stable
    // screen-space tilt that does not change with yaw or pitch.
    if (std::abs(m_lean) > 0.001f)
    {
        view = glm::rotate(view,
                           glm::radians(-m_lean),
                           glm::vec3(0.f, 0.f, 1.f));
    }

    return view;
}

// ── GetProjection ─────────────────────────────────────────────
glm::mat4 Camera::GetProjection(float aspectRatio) const
{
    return glm::perspective(glm::radians(m_fov),
                            aspectRatio,
                            m_nearClip,
                            m_farClip);
}

// ── RecalcVectors ─────────────────────────────────────────────
void Camera::RecalcVectors()
{
    // Convert spherical (yaw, pitch) to Cartesian direction vector
    float yawRad   = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    m_forward = glm::normalize(glm::vec3(
        std::cos(pitchRad) * std::cos(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::sin(yawRad)
    ));

    // Right is perpendicular to forward in the horizontal plane
    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.f, 1.f, 0.f)));

    // Up is perpendicular to both (handles slight pitch-induced tilt)
    m_up    = glm::normalize(glm::cross(m_right, m_forward));
}
