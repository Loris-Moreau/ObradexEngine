#pragma once

// ============================================================
//  World.h  —  Scene / Level Container
// ============================================================
//  The World holds all game entities and drives their update
//  and render cycles. Uses a simple component-based design:
//
//    Entity  — an integer handle (just an ID)
//    Component structs — plain data attached to entities
//    World   — owns component arrays and update logic
//
//  This intentionally avoids a full ECS framework; the engine
//  is small enough that contiguous struct arrays + O(n) loops
//  are perfectly fast at the target entity count (~500).
//
//  Component types included:
//    TransformComponent  — position, rotation, scale
//    MeshComponent       — which mesh + material to render
//    InteractableComponent — can the player interact?
//    LightComponent      — point light source
//    TriggerComponent    — axis-aligned bounding-box trigger
// ============================================================

#include <vector>
#include <string>
#include <functional>
#include <memory>      // std::unique_ptr
#include "Mesh.h"       // full type needed for unique_ptr<Mesh> dtor
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Shader;
class Mesh;

// ── Entity handle ─────────────────────────────────────────────
using EntityID = uint32_t;
constexpr EntityID kNullEntity = 0xFFFFFFFF;

// ── Components ───────────────────────────────────────────────

struct TransformComponent
{
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale    = {1.f, 1.f, 1.f};

    /// Build a model matrix from this transform.
    glm::mat4 GetMatrix() const;
};

struct MeshComponent
{
    Mesh*     mesh          = nullptr;  // Non-owning (World caches meshes)
    glm::vec3 albedoColour  = {0.8f, 0.8f, 0.8f};
    float     specular      = 0.2f;
    float     roughness     = 0.8f;
    bool      castsShadow   = true;     // Reserved for future shadow pass
};

struct InteractableComponent
{
    std::string           promptText;     ///< Shown in HUD (e.g. "Open door")
    float                 range     = 2.f;///< Reach distance in metres
    bool                  enabled   = true;
    std::function<void()> onInteract;     ///< Callback on E-press
};

struct LightComponent
{
    glm::vec3 colour    = {1.f, 0.9f, 0.6f};
    float     radius    = 5.f;
    float     intensity = 1.f;
    bool      flicker   = false;   ///< Simulate a candle / fire
    float     flickerAccum = 0.f;  // Internal accumulator
};

struct TriggerComponent
{
    glm::vec3             halfExtents = {0.5f, 0.5f, 0.5f};
    bool                  triggered   = false;
    std::function<void()> onEnter;   ///< Called once on first overlap
    std::function<void()> onExit;    ///< Called once on leave
};

// ── EntityRecord (internal) ───────────────────────────────────
/// One record per entity; component pointers are null if absent.
struct EntityRecord
{
    EntityID               id         = kNullEntity;
    std::string            name;
    bool                   active     = true;

    // Optional components (nullptr if not added)
    TransformComponent*    transform  = nullptr;
    MeshComponent*         mesh       = nullptr;
    InteractableComponent* interactable = nullptr;
    LightComponent*        light      = nullptr;
    TriggerComponent*      trigger    = nullptr;
};

// ── World ─────────────────────────────────────────────────────
class World
{
public:
    World()  = default;
    ~World() = default;

    // ── Lifecycle ─────────────────────────────────────────────
    void Init();                    ///< Load default test level
    void Update(float dt);          ///< Update all entities
    void Render(Shader& sh) const;  ///< Draw all mesh entities

    // ── Entity management ─────────────────────────────────────
    EntityID CreateEntity(const std::string& name = "Entity");
    void     DestroyEntity(EntityID id);
    EntityRecord* GetRecord(EntityID id);  ///< Null if invalid

    // ── Component attachment ──────────────────────────────────
    TransformComponent*    AddTransform    (EntityID id);
    MeshComponent*         AddMesh         (EntityID id);
    InteractableComponent* AddInteractable (EntityID id);
    LightComponent*        AddLight        (EntityID id);
    TriggerComponent*      AddTrigger      (EntityID id);

    // ── Component query ───────────────────────────────────────
    TransformComponent*    GetTransform    (EntityID id);
    MeshComponent*         GetMesh         (EntityID id);
    InteractableComponent* GetInteractable (EntityID id);

    // ── Scene queries ─────────────────────────────────────────

    /// Find the nearest enabled interactable within [range] of [point].
    EntityID FindNearestInteractable(const glm::vec3& point, float range) const;

    /// All records (for editor iteration)
    const std::vector<EntityRecord>& GetAllRecords() const { return m_records; }

private:
    void LoadTestLevel();  ///< Populate scene with placeholder geometry
    void UpdateLights(float dt);
    void UpdateTriggers(float dt);

    std::vector<EntityRecord> m_records;

    // Component storage pools (indexed by EntityID slot)
    std::vector<TransformComponent>    m_transforms;
    std::vector<MeshComponent>         m_meshes;
    std::vector<InteractableComponent> m_interactables;
    std::vector<LightComponent>        m_lights;
    std::vector<TriggerComponent>      m_triggers;

    // Shared mesh cache (primitives created once, reused)
    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_planeMesh;
    std::unique_ptr<Mesh> m_sphereMesh;

    EntityID m_nextID = 0;
};
