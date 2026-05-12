// ============================================================
//  World.cpp  —  Scene / Level Container
// ============================================================

#include "World.h"
#include "Shader.h"
#include "Mesh.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

// ── TransformComponent::GetMatrix ────────────────────────────
glm::mat4 TransformComponent::GetMatrix() const
{
    glm::mat4 m = glm::mat4_cast(rotation);
    m = glm::scale(m, scale);
    m[3] = glm::vec4(position, 1.f);
    return m;
}

// ── Init ──────────────────────────────────────────────────────
void World::Init()
{
    // ── Reserve component storage BEFORE populating the scene ─────
    // EntityRecord stores raw pointers into these vectors (rec->mesh,
    // rec->transform, etc.).  If any vector reallocates during
    // push_back, every previously stored pointer becomes dangling —
    // silently corrupting the scene and crashing in Mesh::Draw().
    //
    // Reserving upfront prevents reallocation for scenes up to N
    // entities of each component type.  For a proper fix, replace raw
    // pointers with indices (m_meshes[rec.meshIdx]) so reallocation
    // is harmless — but reserve is the safe, simple solution here.
    constexpr size_t MAX_ENTITIES = 512;
    m_records.reserve(MAX_ENTITIES);
    m_transforms.reserve(MAX_ENTITIES);
    m_meshes.reserve(MAX_ENTITIES);
    m_lights.reserve(MAX_ENTITIES);
    m_interactables.reserve(MAX_ENTITIES);
    m_triggers.reserve(MAX_ENTITIES);

    // Pre-allocate shared primitive meshes (uploaded to GPU once).
    // All entities that need a cube/plane/sphere share the same Mesh
    // object — no duplicated GPU memory.
    m_cubeMesh   = std::make_unique<Mesh>(Mesh::MakeCube());
    m_planeMesh  = std::make_unique<Mesh>(Mesh::MakePlane(1.f, 1));
    m_sphereMesh = std::make_unique<Mesh>(Mesh::MakeSphere(1.f, 16));

    LoadTestLevel();
}

// ── LoadTestLevel ─────────────────────────────────────────────
// Populates the scene with a simple environment to validate
// that all subsystems are wired up correctly.
// Replace this with a proper level-loading system (JSON/binary).
void World::LoadTestLevel()
{
    // ── Ground plane ──────────────────────────────────────────
    {
        EntityID e = CreateEntity("Ground");
        auto* t = AddTransform(e);
        t->position = {0.f, 0.f, 0.f};
        t->scale    = {40.f, 1.f, 40.f};

        auto* m = AddMesh(e);
        m->mesh         = m_planeMesh.get();
        m->albedoColour = {0.22f, 0.21f, 0.20f};  // Dark cobblestone
        m->roughness    = 0.95f;
    }

    // ── Some crates (world-space objects to navigate around) ──
    const glm::vec3 cratePositions[] = {
        {3.f,0.5f,5.f}, {-2.f,0.5f,8.f}, {5.f,0.5f,2.f},
        {-5.f,0.5f,3.f},{0.f,0.5f,10.f}
    };
    for (auto& pos : cratePositions)
    {
        EntityID e = CreateEntity("Crate");
        auto* t = AddTransform(e);
        t->position = pos;

        auto* m = AddMesh(e);
        m->mesh         = m_cubeMesh.get();
        m->albedoColour = {0.45f, 0.35f, 0.22f};  // Weathered wood
        m->roughness    = 0.9f;
    }

    // ── A lamp post (interactable — can be toggled) ───────────
    {
        EntityID lamp = CreateEntity("LampPost");
        auto* t = AddTransform(lamp);
        t->position = {6.f, 2.f, 0.f};
        t->scale    = {0.15f, 4.f, 0.15f};

        auto* m = AddMesh(lamp);
        m->mesh         = m_cubeMesh.get();
        m->albedoColour = {0.15f, 0.14f, 0.13f};  // Dark iron

        // Light source at the top of the post
        EntityID lightE = CreateEntity("LampLight");
        auto* lt = AddTransform(lightE);
        lt->position = {6.f, 4.5f, 0.f};

        auto* lc = AddLight(lightE);
        lc->colour    = {1.f, 0.85f, 0.5f};
        lc->radius    = 10.f;
        lc->intensity = 1.5f;
        lc->flicker   = true;

        // Make the lamp interactable (toggle the light)
        auto* ia = AddInteractable(lamp);
        ia->promptText = "[E] Toggle lamp";
        ia->range      = 3.f;
        ia->onInteract = [this, lightE]()
        {
            auto* lc2 = GetRecord(lightE)->light;
            if (lc2) lc2->intensity = (lc2->intensity > 0.f) ? 0.f : 1.5f;
            std::cout << "[World] Lamp toggled.\n";
        };
    }

    // ── Exit trigger (demonstrates gameplay-logic hookup) ─────
    {
        EntityID trigger = CreateEntity("ExitZone");
        auto* t = AddTransform(trigger);
        t->position = {0.f, 1.f, -15.f};

        auto* tri = AddTrigger(trigger);
        tri->halfExtents = {3.f, 2.f, 1.f};
        tri->onEnter = []()
        {
            std::cout << "[World] Player reached the exit zone!\n";
        };
    }

    std::cout << "[World] Test level loaded (" << m_records.size() << " entities).\n";
}

// ── Update ────────────────────────────────────────────────────
void World::Update(float dt)
{
    UpdateLights(dt);
    // UpdateTriggers is called by Player after its position update
}

void World::UpdateLights(float dt)
{
    for (auto& rec : m_records)
    {
        if (!rec.active || !rec.light) continue;
        auto& lc = *rec.light;
        if (!lc.flicker || lc.intensity <= 0.f) continue;

        // Perlin-style flicker using multiple sine waves
        lc.flickerAccum += dt;
        float noise = std::sin(lc.flickerAccum * 7.3f)
                    + std::sin(lc.flickerAccum * 17.1f) * 0.3f
                    + std::sin(lc.flickerAccum * 3.7f)  * 0.6f;
        lc.intensity = 1.5f + noise * 0.15f;  // Varies ±10% around base
    }
}

void World::UpdateTriggers(float dt)
{
    (void)dt;
    // NOTE: Trigger checking is done by Player::Update because it
    // needs the player position. See gameplay/Player.cpp.
}

// ── Render ────────────────────────────────────────────────────
void World::Render(Shader& sh) const
{
    for (const auto& rec : m_records)
    {
        if (!rec.active || !rec.mesh || !rec.transform) continue;
        const auto& mesh = *rec.mesh;
        if (!mesh.mesh) continue;

        glm::mat4 model = rec.transform->GetMatrix();
        sh.SetMat4("u_Model",        model);
        sh.SetVec3("u_AlbedoColour", mesh.albedoColour);
        sh.SetFloat("u_Specular",    mesh.specular);
        sh.SetFloat("u_Roughness",   mesh.roughness);
        sh.SetInt  ("u_HasTexture",  0);  // Texture system stub

        mesh.mesh->Draw();
    }
}

// ── Entity management ─────────────────────────────────────────
EntityID World::CreateEntity(const std::string& name)
{
    EntityID id = m_nextID++;
    auto& rec   = m_records.emplace_back();
    rec.id   = id;
    rec.name = name;
    return id;
}

void World::DestroyEntity(EntityID id)
{
    auto it = std::find_if(m_records.begin(), m_records.end(),
                           [id](const EntityRecord& r){ return r.id == id; });
    if (it != m_records.end()) it->active = false;
}

EntityRecord* World::GetRecord(EntityID id)
{
    for (auto& r : m_records)
        if (r.id == id) return &r;
    return nullptr;
}

// ── Component attachment ──────────────────────────────────────
// Each Add* function appends to the relevant pool and stores
// the pointer in the EntityRecord. Simple and cache-friendly
// for this entity count.

TransformComponent* World::AddTransform(EntityID id)
{
    m_transforms.push_back({});
    auto* c = &m_transforms.back();
    if (auto* r = GetRecord(id)) r->transform = c;
    return c;
}

MeshComponent* World::AddMesh(EntityID id)
{
    m_meshes.push_back({});
    auto* c = &m_meshes.back();
    if (auto* r = GetRecord(id)) r->mesh = c;
    return c;
}

InteractableComponent* World::AddInteractable(EntityID id)
{
    m_interactables.push_back({});
    auto* c = &m_interactables.back();
    if (auto* r = GetRecord(id)) r->interactable = c;
    return c;
}

LightComponent* World::AddLight(EntityID id)
{
    m_lights.push_back({});
    auto* c = &m_lights.back();
    if (auto* r = GetRecord(id)) r->light = c;
    return c;
}

TriggerComponent* World::AddTrigger(EntityID id)
{
    m_triggers.push_back({});
    auto* c = &m_triggers.back();
    if (auto* r = GetRecord(id)) r->trigger = c;
    return c;
}

// ── Component query ───────────────────────────────────────────
TransformComponent*    World::GetTransform   (EntityID id) { auto* r = GetRecord(id); return r ? r->transform    : nullptr; }
MeshComponent*         World::GetMesh        (EntityID id) { auto* r = GetRecord(id); return r ? r->mesh         : nullptr; }
InteractableComponent* World::GetInteractable(EntityID id) { auto* r = GetRecord(id); return r ? r->interactable : nullptr; }

// ── FindNearestInteractable ───────────────────────────────────
EntityID World::FindNearestInteractable(const glm::vec3& point, float range) const
{
    EntityID best = kNullEntity;
    float    bestDist = range * range;  // Work in squared distance

    for (const auto& rec : m_records)
    {
        if (!rec.active || !rec.interactable || !rec.transform) continue;
        if (!rec.interactable->enabled) continue;

        float r     = rec.interactable->range;
        float limit = std::min(range, r);
        float d2    = glm::dot(rec.transform->position - point,
                               rec.transform->position - point);
        if (d2 < limit * limit && d2 < bestDist)
        {
            bestDist = d2;
            best     = rec.id;
        }
    }
    return best;
}
