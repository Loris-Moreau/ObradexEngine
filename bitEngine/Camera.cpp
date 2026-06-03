// Camera.cpp - First-person camera implementation.

#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 position, float fovDeg)
    : m_position(position), m_fov(fovDeg)
{
    RecalcVectors();
}

void Camera::Rotate(float dx, float dy, float sensitivity)
{
    m_yaw   += dx * sensitivity;
    m_pitch -= dy * sensitivity;  // Subtract so mouse-up = look up

    m_pitch = std::clamp(m_pitch, -89.f, 89.f);

    // Keep yaw in [-180, 180] to prevent float drift over time.
    if (m_yaw >  180.f) m_yaw -= 360.f;
    if (m_yaw < -180.f) m_yaw += 360.f;

    RecalcVectors();
}

void Camera::SetLean(float amount)
{
    m_leanTarget = std::clamp(amount, -15.f, 15.f);
}

void Camera::UpdateHeadBob(float speed, float dt)
{
    const float kBobFrequency   = 2.0f;   // Cycles per second
    const float kBobAmplY       = 0.04f;  // Vertical amplitude (m)
    const float kBobAmplX       = 0.02f;  // Horizontal amplitude (m)
    const float kSpeedThreshold = 0.5f;   // Minimum speed before bob activates

    // Lerp lean toward target each frame.
    const float kLeanSpeed = 8.f;
    m_lean += (m_leanTarget - m_lean) * kLeanSpeed * dt;

    if (speed < kSpeedThreshold)
    {
        // Damp bob offsets back to zero when standing still.
        m_bobY += (0.f - m_bobY) * 10.f * dt;
        m_bobX += (0.f - m_bobX) * 10.f * dt;
        return;
    }

    m_bobTime += dt * kBobFrequency * (speed / 5.f);

    // Lissajous figure-8 pattern for a natural walking bob.
    m_bobY = std::sin(m_bobTime * 2.f * 3.14159f) * kBobAmplY;
    m_bobX = std::sin(m_bobTime       * 3.14159f) * kBobAmplX;
}

glm::mat4 Camera::GetView() const
{
    // Bob offsets use world-up and flat-right (not pitched camera vectors)
    // so the eye stays level regardless of pitch direction.
    glm::vec3 worldUp   = {0.f, 1.f, 0.f};
    glm::vec3 flatRight = glm::normalize(glm::cross(m_forward, worldUp));

    // Physical lean: shift the eye sideways so the player can peek around
    // corners, not just tilt the view in place.
    // leanFraction is in [-1, 1], mapping m_lean to the configured distance.
    // A small upward offset (5% of the lateral shift) counters the slight
    // downward drift that a real sideways lean produces — without it the
    // horizon appears to drop noticeably at full lean.
    float leanFraction  = m_lean / 15.f;
    float lateralOffset = leanFraction * m_leanDistance;
    float verticalCorr  = std::abs(lateralOffset) * 0.05f;

    glm::vec3 eye = m_position
                  + worldUp   * (m_bobY + verticalCorr)
                  + flatRight * (m_bobX + lateralOffset);

    // Build the view as V = R * T(-eye) rather than using lookAt, so the
    // lean roll can be applied to R before the eye translation is appended.
    //
    // If the roll were applied after the full V (e.g. V * roll), the rotation
    // axis would pass through the world origin instead of the eye. Far from
    // the origin that swings the eye visibly sideways on each lean frame.
    // Applying roll to R first keeps the axis at the eye at any world position.
    glm::vec3 up_view = glm::normalize(glm::cross(flatRight, m_forward));
    glm::mat4 R(1.f);
    // GLM is column-major: R[col][row]
    R[0][0] =  flatRight.x;  R[0][1] =  up_view.x;  R[0][2] = -m_forward.x;
    R[1][0] =  flatRight.y;  R[1][1] =  up_view.y;  R[1][2] = -m_forward.y;
    R[2][0] =  flatRight.z;  R[2][1] =  up_view.z;  R[2][2] = -m_forward.z;

    // Apply lean roll.
    // glm::rotate(M, angle, axis) rotates M around `axis` in the space M
    // maps FROM (world space here). Passing -m_forward is the world-space
    // equivalent of the camera's into-screen axis, so the roll tracks the
    // camera's actual forward direction at any yaw and pitch.
    // Sign: A (m_lean < 0) tilts screen-top left; E (m_lean > 0) tilts right.
    if (std::abs(m_lean) > 0.001f)
    {
        R = glm::rotate(R, glm::radians(m_lean), -m_forward);
    }

    // Append the eye translation last so rotation is around the eye.
    glm::mat4 T(1.f);
    T[3][0] = -eye.x;
    T[3][1] = -eye.y;
    T[3][2] = -eye.z;

    return R * T;
}

glm::mat4 Camera::GetProjection(float aspectRatio) const
{
    return glm::perspective(glm::radians(m_fov),
                            aspectRatio,
                            m_nearClip,
                            m_farClip);
}

void Camera::RecalcVectors()
{
    // Convert yaw/pitch to a Cartesian forward direction.
    float yawRad   = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    m_forward = glm::normalize(glm::vec3(
        std::cos(pitchRad) * std::cos(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::sin(yawRad)
    ));

    m_right = glm::normalize(glm::cross(m_forward, glm::vec3(0.f, 1.f, 0.f)));
    m_up    = glm::normalize(glm::cross(m_right, m_forward));
}
