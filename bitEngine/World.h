#pragma once

// World.h - Scene and level container.
//
// Stores all entities as EntityRecord values, each holding optional raw
// pointers to the corresponding component slots in pre-reserved vectors.
// Pointers stay valid as long as no reallocation occurs; Init() and
// ClearLevel() both reserve capacity to 1024 to prevent this.

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

// Components

struct TransformComponent
{
    glm::vec3 position = {0.f, 0.f, 0.f};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale    = {1.f, 1.f, 1.f};
    glm::mat4 GetMatrix() const;
};

// How the texture maps onto the surface.
//   Stretch - texture spans the full 0..1 UV range once, ignoring aspect ratio
//   Tile    - texture repeats; repeat count scales with the entity's world
//             size so texel density stays constant as the entity is resized
//   Fit     - texture is scaled uniformly (no stretch) to match the surface's
//             aspect ratio; the shorter axis is tiled to avoid distortion
enum class UVMode { Stretch, Tile, Fit };

struct MeshComponent
{
    Mesh*        mesh         = nullptr;
    glm::vec3    albedoColour = {0.8f, 0.8f, 0.8f};
    float        specular     = 0.2f;
    float        roughness    = 0.8f;
    bool         castsShadow  = true;
    // useTexture=true multiplies the sampled texel by albedoColour in the
    // shader; leave albedoColour white (1,1,1) to show the texture unmodified.
    bool         useTexture   = false;
    unsigned int textureID    = 0;
    std::string  texturePath;
    UVMode       uvMode       = UVMode::Stretch;
    // Repeat count used directly in Tile mode; ignored in Stretch and Fit.
    glm::vec2    uvTiling     = {1.f, 1.f};
};

struct InteractableComponent
{
    std::string           promptText;
    float                 range   = 2.f;
    bool                  enabled = true;
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

// halfExtents are in local space; multiplied by transform->scale at runtime.
struct CollisionComponent
{
    glm::vec3 halfExtents = {0.5f, 0.5f, 0.5f};
    bool      solid       = true;
    bool      slippery    = false;  // If false (default), zeroes horizontal velocity on landing like the floor
};

// Defined here (not Interaction.h) so World, EditorUI, and Interaction
// can all use Item without circular includes.
struct Item
{
    std::string name;
    std::string description;
    int         quantity = 1;
};

// Used by searchable entities. UI reads isOpen to render the grid popup.
struct ContainerComponent
{
    std::vector<Item> items;
    bool              isOpen = false;
};

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
    CollisionComponent*    collision    = nullptr;
    ContainerComponent*    container    = nullptr;
};

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

    // Remove all entities and component data; keep primitive GPU meshes.
    void ClearLevel();

    TransformComponent*    AddTransform    (EntityID id);
    MeshComponent*         AddMesh         (EntityID id);
    InteractableComponent* AddInteractable (EntityID id);
    LightComponent*        AddLight        (EntityID id);
    TriggerComponent*      AddTrigger      (EntityID id);
    CollisionComponent*    AddCollision    (EntityID id);
    ContainerComponent*    AddContainer    (EntityID id);

    TransformComponent*    GetTransform    (EntityID id);
    MeshComponent*         GetMesh         (EntityID id);
    InteractableComponent* GetInteractable (EntityID id);

    // Returns the nearest enabled interactable within range of point.
    EntityID FindNearestInteractable(const glm::vec3& point, float range) const;

    const std::vector<EntityRecord>& GetAllRecords() const { return m_records; }

    // Returns true if any container currently has isOpen == true.
    bool HasOpenContainer() const;

    // Set isOpen = false on the first open container found.
    void CloseOpenContainer();

    glm::vec3 GetSpawnPos()  const             { return m_spawnPos; }
    bool      IsLevelComplete() const { return m_levelComplete; }
    void      ResetLevelComplete()    { m_levelComplete = false; }
    void      SetSpawnPos(const glm::vec3& p) { m_spawnPos = p;    }
    void LoadDefaultLevel(); // Public alias to LoadTestLevel

    Mesh* GetCubeMesh()   { return m_cubeMesh.get();   }
    Mesh* GetPlaneMesh()  { return m_planeMesh.get();  }
    Mesh* GetSphereMesh() { return m_sphereMesh.get(); }

private:
    void LoadTestLevel();
    void UpdateLights(float dt);
    void ReserveComponentStorage();

    glm::vec3 m_spawnPos      = {0.f, 0.f, 0.f};
    bool      m_levelComplete = false;

    std::vector<EntityRecord>          m_records;
    std::vector<TransformComponent>    m_transforms;
    std::vector<MeshComponent>         m_meshes;
    std::vector<InteractableComponent> m_interactables;
    std::vector<LightComponent>        m_lights;
    std::vector<TriggerComponent>      m_triggers;
    std::vector<CollisionComponent>    m_collisions;
    std::vector<ContainerComponent>    m_containers;

    std::unique_ptr<Mesh> m_cubeMesh;
    std::unique_ptr<Mesh> m_planeMesh;
    std::unique_ptr<Mesh> m_sphereMesh;

    EntityID m_nextID = 0;
};
