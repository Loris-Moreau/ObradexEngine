#pragma once

// Camera.h - First-person camera.
//
// Manages orientation via yaw/pitch Euler angles and builds a view matrix
// each frame. Supports lean (side-tilt peek), head-bob, and FOV control.
//
// Yaw is unclamped (full 360 rotation). Pitch clamps to +-89 degrees.
// Lean is a screen-space roll applied to the rotation matrix before the
// eye translation, so the roll axis always passes through the eye.

#include <glm/glm.hpp>

class Camera
{
public:
    Camera() = default;
    explicit Camera(glm::vec3 position, float fovDeg = 75.f);

    // Rotate by mouse delta (pixels). Positive dx = look right, positive dy = look down.
    void Rotate(float dx, float dy, float sensitivity = 0.1f);

    // Set the target lean angle in degrees. Clamped to +-15. Lerped each frame.
    void SetLean(float amount);

    // Advance the head-bob simulation. Call every physics tick with current speed.
    void UpdateHeadBob(float speed, float dt);

    // Build and return the view matrix for this frame.
    glm::mat4 GetView() const;

    // Build a perspective projection matrix from the given aspect ratio.
    glm::mat4 GetProjection(float aspectRatio) const;

    glm::vec3 GetPosition()  const { return m_position;  }
    glm::vec3 GetForward()   const { return m_forward;   }
    glm::vec3 GetRight()     const { return m_right;     }
    glm::vec3 GetUp()        const { return m_up;        }
    float     GetYaw()       const { return m_yaw;       }
    float     GetPitch()     const { return m_pitch;     }
    float     GetFOV()       const { return m_fov;       }

    void SetPosition(const glm::vec3& pos) { m_position = pos; }
    void SetFOV(float fov)                 { m_fov = fov;      }

private:
    // Recompute m_forward, m_right, and m_up from the current yaw and pitch.
    void RecalcVectors();

    glm::vec3 m_position = {0.f, 1.7f, 0.f};
    glm::vec3 m_forward  = {0.f, 0.f, -1.f};
    glm::vec3 m_right    = {1.f, 0.f,  0.f};
    glm::vec3 m_up       = {0.f, 1.f,  0.f};

    float m_yaw   = -90.f;  // Default facing -Z
    float m_pitch =   0.f;

    float m_lean       = 0.f;  // Current lean angle (degrees), lerped
    float m_leanTarget = 0.f;  // Requested lean angle

    float m_bobTime = 0.f;
    float m_bobY    = 0.f;  // Vertical bob offset (m)
    float m_bobX    = 0.f;  // Horizontal bob offset (m)

    float m_fov      = 75.f;
    float m_nearClip = 0.05f;
    float m_farClip  = 500.f;
};
