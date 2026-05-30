#pragma once

// ============================================================
//  Camera.h  -  First-Person Camera
// ============================================================
//  Manages a view matrix via yaw/pitch Euler angles.
//  Designed for first-person movement (Deus Ex / Syndicate):
//    - Unclamped yaw (full 360° horizontal)
//    - Clamped pitch (-89° … +89°) to avoid gimbal lock
//    - Lean offset (left/right tilt, Thief-style)
//    - Head-bob animation driven by the player's speed
//    - Projection matrix built from FOV + aspect ratio
// ============================================================

#include <glm/glm.hpp>

class Camera
{
public:
    // ── Construction ─────────────────────────────────────────
    Camera() = default;
    explicit Camera(glm::vec3 position, float fovDeg = 75.f);

    // ── Orientation ──────────────────────────────────────────

    /// Rotate the camera by mouse delta (pixels).
    /// @param dx      Horizontal mouse delta (positive = right).
    /// @param dy      Vertical mouse delta   (positive = down in screen space).
    /// @param sensitivity  Mouse sensitivity multiplier.
    void Rotate(float dx, float dy, float sensitivity = 0.1f);

    // ── Lean (Thief / Deus Ex style) ─────────────────────────
    /// @param amount  Target lean angle in degrees (-15 to +15).
    void SetLean(float amount);

    // ── Head bob (driven by Player) ───────────────────────────
    /// @param speed     Current player speed (m/s).
    /// @param dt        Delta time (s).
    void UpdateHeadBob(float speed, float dt);

    // ── Matrices ─────────────────────────────────────────────
    glm::mat4 GetView()       const;
    glm::mat4 GetProjection(float aspectRatio) const;

    // ── Accessors ─────────────────────────────────────────────
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
    /// Rebuild m_forward, m_right, m_up from current yaw/pitch.
    void RecalcVectors();

    glm::vec3 m_position = {0.f, 1.7f, 0.f};  // Default eye height 1.7m
    glm::vec3 m_forward  = {0.f, 0.f, -1.f};
    glm::vec3 m_right    = {1.f, 0.f,  0.f};
    glm::vec3 m_up       = {0.f, 1.f,  0.f};

    float m_yaw   = -90.f;  // Facing -Z by default
    float m_pitch =   0.f;

    float m_lean      = 0.f;   // Current lean angle (degrees)
    float m_leanTarget= 0.f;   // Target lean (lerped toward)

    float m_bobTime   = 0.f;   // Accumulated time for sin-wave bob
    float m_bobY      = 0.f;   // Current vertical bob offset
    float m_bobX      = 0.f;   // Current horizontal bob offset

    float m_fov       = 75.f;  // Vertical FOV in degrees
    float m_nearClip  = 0.05f;
    float m_farClip   = 500.f;
};
