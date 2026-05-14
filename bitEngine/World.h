#pragma once

// ============================================================
//  World.h  —  Scene / Level Container
// ============================================================
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Shader;
class Mesh;

using EntityID = uint32_t;
constexpr EntityID kNullEntity = 0xFFFFFFFF;

// ── Components ───────────────────────────────────────────────

struct TransformComponent
{
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale    = {1.f, 1.f, 1.f};
    glm::mat4 GetMatrix() const;
};

struct MeshComponent
{
    Mesh*     mesh          = nullptr;
    glm::vec3 albedoColour  = {0.8f, 0.8f, 0.8f};
    float     specular      = 0.2f;
    float     roughness     = 0.8f;
    bool      castsShadow   = true;
};

struct InteractableComponent
{
    std::string           promptText;
    float                 range     = 2.f;
    bool                  enabled   = true;
    std::function<void()> onInteract;
};

struct LightComponent
{
    glm::vec3 colour       = {1.f, 0.9f, 0.6f};
    float     radius       = 5.f;
    float     intensity    = 1.f;
    bool      flicker      = false;
    float     flickerAccum = 0.f;
};

struct TriggerComponent
{
    glm::vec3             halfExtents = {0.5f, 0.5f, 0.5f};
    bool                  triggered   = false;
    std::function<void()> onEnter;
    std::function<void()> onExit;
};

/// Solid AABB that blocks player movement.
/// halfExtents are in LOCAL space — multiplied by transform->scale at runtime.
struct CollisionComponent
{
    glm::vec3 halfExtents = {0.5f, 0.5f, 0.5f};
    bool      solid       = true;   ///< If false, is ghost/trigger only
};

// ── EntityRecord ─────────────────────────────────────────────
struct EntityRecord
{
    EntityID               id           = kNullEntity;
    std::string            name;
    bool                   active       = true;

    TransformComponent*    transform    = nullptr;
    MeshComponent*         mesh         = nullptr;
    InteractableComponent* interactable = nullptr;
    LightComponent*        light        = nullptr;
    TriggerComponent*      trigger      = nullptr;
    CollisionComponent*    collision    = nullptr;  ///< nullptr = no solid collision
};

// ── World ─────────────────────────────────────────────────────
class World
{
public:
    World()  = default;
    ~World() = default;

    void Init();
    void Update(float dt);
    void Render(Shader& sh) const;

    EntityID      CreateEntity (const std::string& name = "Entity");
    void          DestroyEntity(EntityID id);
    EntityRecord* GetRecord    (EntityID id);

    TransformComponent*    AddTransform    (EntityID id);
    MeshComponent*         AddMesh         (EntityID id);
    InteractableComponent* AddInteractable (EntityID id);
    LightComponent*        AddLight        (EntityID id);
    TriggerComponent*      AddTrigger      (EntityID id);
    CollisionComponent*    AddCollision    (EntityID id);   ///< Add solid AABB

    TransformComponent*    GetTransform    (EntityID id);
    MeshComponent*         GetMesh         (EntityID id);
    InteractableComponent* GetInteractable (EntityID id);

    EntityID FindNearestInteractable(const glm::vec3& point, float range) const;

    const std::vector<EntityRecord>& GetAllRecords() const { return m_records; }

private:
    void LoadTestLevel();
    void UpdateLights  (float dt);
    void UpdateTriggers(float dt);

    std::vector<EntityRecord>          m_records;
    std::vector<TransformComponent>    m_transforms;
    std::vector<MeshComponent>         m_meshes;
    std::vector<InteractableComponent> m_interactables;
    std::vector<LightComponent>        m_lights;
    std::vector<TriggerComponent>      m_triggers;
    std::vector<CollisionComponent>    m_collisions;

    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_planeMesh;
    std::unique_ptr<Mesh> m_sphereMesh;

    EntityID m_nextID = 0;
};
