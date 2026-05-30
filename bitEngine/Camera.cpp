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
    // ── Eye position ──────────────────────────────────────────
    // Bob offsets in world space using world-up and flat-right so they are
    // always purely vertical/horizontal regardless of pitch.
    glm::vec3 worldUp   = {0.f, 1.f, 0.f};
    glm::vec3 flatRight = glm::normalize(glm::cross(m_forward, worldUp));
    glm::vec3 eye = m_position
                  + worldUp   * m_bobY
                  + flatRight * m_bobX;

    // ── View matrix = R * T(-eye) ─────────────────────────────
    //
    // A view matrix decomposes as:   V = R * T(-eye)
    //   T(-eye)  translates world so the camera eye is at the origin.
    //   R        is the camera orientation (rotation only).
    //
    // Lean must rotate the camera around its OWN forward axis (local Z).
    // If we apply the roll AFTER assembling the full V (i.e. V * R_roll or
    // R_roll * V), the rotation axis passes through the WORLD ORIGIN, not
    // the eye.  When the eye is far from the world origin this swings it
    // through a visible arc — the distortion seen far from (0,0,0).
    //
    // The fix: apply the lean roll to R BEFORE appending T(-eye).
    //   V_final = R_lean * R_yawpitch * T(-eye)
    // Because T(-eye) is applied last, the rotation axis always passes
    // through the eye regardless of where it is in the world.

    // Step 1 – orientation rows from yaw/pitch basis vectors
    //   (identical to the 3×3 rotation block that glm::lookAt produces)
    glm::vec3 up_view = glm::normalize(glm::cross(flatRight, m_forward));
    glm::mat4 R(1.f);
    // Column-major storage: R[col][row]
    R[0][0] =  flatRight.x;  R[0][1] =  up_view.x;  R[0][2] = -m_forward.x;
    R[1][0] =  flatRight.y;  R[1][1] =  up_view.y;  R[1][2] = -m_forward.y;
    R[2][0] =  flatRight.z;  R[2][1] =  up_view.z;  R[2][2] = -m_forward.z;

    // Step 2 – lean roll
    //
    // glm::rotate(M, angle, axis) computes  M * Rw(axis, angle)
    // where Rw is a rotation about `axis` expressed in the space M maps FROM
    // — i.e. world space for our pure-rotation R.
    //
    // We want to roll around the camera's OWN into-screen axis.
    // In view convention, camera local +Z points OUT of the screen, so the
    // into-screen (forward) axis in world space is -m_forward.
    //
    // Passing -m_forward makes the roll axis track the camera's actual forward
    // direction at any yaw and pitch — the tilt is always a pure screen-space
    // roll, identical at every look direction.
    //
    // Sign: A → m_lean < 0 → top of screen tilts left  → lean left  ✓
    //       E → m_lean > 0 → top of screen tilts right → lean right ✓
    if (std::abs(m_lean) > 0.001f)
    {
        R = glm::rotate(R, glm::radians(m_lean), -m_forward);
    }

    // Step 3 – eye translation (column-major: last column = translation)
    glm::mat4 T(1.f);
    T[3][0] = -eye.x;
    T[3][1] = -eye.y;
    T[3][2] = -eye.z;

    return R * T;
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
