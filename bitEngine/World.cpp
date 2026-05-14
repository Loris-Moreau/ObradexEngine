// ============================================================
//  World.cpp
// ============================================================

#include "World.h"
#include "Shader.h"
#include "Mesh.h"
#include "Input.h"          // for INTERACT_KEY + Input::GetKeyName
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

// ── TransformComponent::GetMatrix ────────────────────────────
glm::mat4 TransformComponent::GetMatrix() const
{
    // Build TRS:  T * R * S
    // mat4_cast(rotation) gives a pure rotation matrix.
    // glm::scale scales each basis vector in local space.
    // Column 3 is then overwritten with the world-space position.
    glm::mat4 m = glm::mat4_cast(rotation);
    m = glm::scale(m, scale);
    m[3] = glm::vec4(position, 1.f);
    return m;
}

// ── Init ──────────────────────────────────────────────────────
void World::Init()
{
    // Reserve ALL component vectors before populating the scene.
    // EntityRecord stores raw pointers into these vectors — any
    // reallocation (push_back past capacity) would make them dangle.
    constexpr size_t MAX = 512;
    m_records.reserve(MAX);
    m_transforms.reserve(MAX);
    m_meshes.reserve(MAX);
    m_lights.reserve(MAX);
    m_interactables.reserve(MAX);
    m_triggers.reserve(MAX);
    m_collisions.reserve(MAX);

    m_cubeMesh   = std::make_unique<Mesh>(Mesh::MakeCube());
    m_planeMesh  = std::make_unique<Mesh>(Mesh::MakePlane(1.f, 1));
    m_sphereMesh = std::make_unique<Mesh>(Mesh::MakeSphere(1.f, 16));

    LoadTestLevel();
}

// ── LoadTestLevel ─────────────────────────────────────────────
void World::LoadTestLevel()
{
    // Helper: build the "[X] " interact prefix from the global key binding
    auto interactPrefix = []() -> std::string {
        return std::string("[") + Input::GetKeyName(INTERACT_KEY) + "] ";
    };

    // ── Ground plane (no collision — floor at y=0 handled in Player) ──
    {
        EntityID e = CreateEntity("Ground");
        auto* t = AddTransform(e);
        t->position = {0.f, 0.f, 0.f};
        t->scale    = {40.f, 1.f, 40.f};

        auto* m = AddMesh(e);
        m->mesh         = m_planeMesh.get();
        m->albedoColour = {0.0f, 0.7f, 0.7f};
        m->roughness    = 0.95f;
    }

    // ── Crates (solid AABB collision) ─────────────────────────
    const glm::vec3 cratePositions[] = {
        {3.f,0.5f,5.f}, {-2.f,0.5f,8.f}, {5.f,0.5f,2.f},
        {-5.f,0.5f,3.f},{0.f,0.5f,10.f}
    };
    for (auto& pos : cratePositions)
    {
        EntityID e = CreateEntity("Crate");
        auto* t = AddTransform(e);
        t->position = pos;
        t->scale    = {1.f, 1.f, 1.f};

        auto* m = AddMesh(e);
        m->mesh         = m_cubeMesh.get();
        m->albedoColour = {0.69f, 0.39f, 0.098f};
        m->roughness    = 0.9f;

        // Solid 1×1×1 box (halfExtents in local space, scaled at runtime)
        auto* col = AddCollision(e);
        col->halfExtents = {0.5f, 0.5f, 0.5f};
    }

    // ── Lamp post (solid + interactable) ──────────────────────
    {
        EntityID lamp = CreateEntity("LampPost");
        auto* t = AddTransform(lamp);
        t->position = {6.f, 2.f, 0.f};
        t->scale    = {0.15f, 4.0f, 0.15f};

        auto* m = AddMesh(lamp);
        m->mesh         = m_cubeMesh.get();
        m->albedoColour = {0.45f, 0.44f, 0.43f};

        // Thin post — small collision box
        auto* col = AddCollision(lamp);
        col->halfExtents = {0.5f, 0.5f, 0.5f};  // scaled by t->scale at runtime

        // Light at the top
        EntityID lightE = CreateEntity("LampLight");
        auto* lt = AddTransform(lightE);
        lt->position = {6.f, 4.5f, 0.f};

        auto* lc = AddLight(lightE);
        lc->colour    = {1.0f, 0.85f, 0.5f};
        lc->radius    = 10.f;
        lc->intensity = 3.0f;
        lc->flicker   = true;

        // Interact prompt uses the globally configured interact key
        auto* ia = AddInteractable(lamp);
        ia->promptText = interactPrefix() + "Toggle lamp";
        ia->range      = 3.0f;
        ia->onInteract = [this, lightE]()
        {
            auto* lc2 = GetRecord(lightE)->light;
            if (lc2) lc2->intensity = (lc2->intensity > 0.f) ? 0.f : 1.5f;
            std::cout << "[World] Lamp toggled.\n";
        };
    }

    // ── Exit marker box (solid) ───────────────────────────────
    {
        EntityID e = CreateEntity("EndMarker");
        auto* t = AddTransform(e);
        t->position = {0.f, 1.f, -15.f};
        t->scale    = {0.5f, 0.5f, 0.5f};

        auto* m = AddMesh(e);
        m->mesh         = m_cubeMesh.get();
        m->albedoColour = {1.0f, 0.0f, 0.0f};
        m->roughness    = 0.75f;

        auto* col = AddCollision(e);
        col->halfExtents = {0.25f, 0.25f, 0.25f};
    }

    // ── Exit trigger zone ─────────────────────────────────────
    {
        EntityID trigger = CreateEntity("ExitZone");
        auto* t = AddTransform(trigger);
        t->position = {0.f, 1.f, -15.f};

        auto* tri = AddTrigger(trigger);
        tri->halfExtents = {3.f, 2.f, 1.f};
        tri->onEnter     = []() {
            std::cout << "[World] Player reached the exit zone!\n";
        };
    }

    std::cout << "[World] Test level loaded (" << m_records.size() << " entities).\n";
}

// ── Update ────────────────────────────────────────────────────
void World::Update(float dt)
{
    UpdateLights(dt);
}

void World::UpdateLights(float dt)
{
    for (auto& rec : m_records)
    {
        if (!rec.active || !rec.light) continue;
        auto& lc = *rec.light;
        if (!lc.flicker || lc.intensity <= 0.f) continue;

        lc.flickerAccum += dt;
        float noise = std::sin(lc.flickerAccum * 7.3f)
                    + std::sin(lc.flickerAccum * 17.1f) * 0.3f
                    + std::sin(lc.flickerAccum * 3.7f)  * 0.6f;
        lc.intensity = 1.5f + noise * 0.15f;
    }
}

void World::UpdateTriggers(float dt) { (void)dt; }

// ── Render ────────────────────────────────────────────────────
void World::Render(Shader& sh) const
{
    for (const auto& rec : m_records)
    {
        if (!rec.active || !rec.mesh || !rec.transform) continue;
        const auto& mc = *rec.mesh;
        if (!mc.mesh) continue;

        sh.SetMat4("u_Model",        rec.transform->GetMatrix());
        sh.SetVec3("u_AlbedoColour", mc.albedoColour);
        sh.SetFloat("u_Specular",    mc.specular);
        sh.SetFloat("u_Roughness",   mc.roughness);
        sh.SetInt  ("u_HasTexture",  0);
        mc.mesh->Draw();
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
    for (auto& r : m_records)
        if (r.id == id) { r.active = false; return; }
}

EntityRecord* World::GetRecord(EntityID id)
{
    for (auto& r : m_records)
        if (r.id == id) return &r;
    return nullptr;
}

// ── Component attachment ──────────────────────────────────────
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
CollisionComponent* World::AddCollision(EntityID id)
{
    m_collisions.push_back({});
    auto* c = &m_collisions.back();
    if (auto* r = GetRecord(id)) r->collision = c;
    return c;
}

// ── Component query ───────────────────────────────────────────
TransformComponent*    World::GetTransform   (EntityID id) { auto* r=GetRecord(id); return r?r->transform   :nullptr; }
MeshComponent*         World::GetMesh        (EntityID id) { auto* r=GetRecord(id); return r?r->mesh        :nullptr; }
InteractableComponent* World::GetInteractable(EntityID id) { auto* r=GetRecord(id); return r?r->interactable:nullptr; }

// ── FindNearestInteractable ───────────────────────────────────
EntityID World::FindNearestInteractable(const glm::vec3& point, float range) const
{
    EntityID best     = kNullEntity;
    float    bestDist = range * range;
    for (const auto& rec : m_records)
    {
        if (!rec.active || !rec.interactable || !rec.transform) continue;
        if (!rec.interactable->enabled) continue;
        float limit = std::min(range, rec.interactable->range);
        glm::vec3 diff = rec.transform->position - point;
        float d2 = glm::dot(diff, diff);
        if (d2 < limit * limit && d2 < bestDist)
        {
            bestDist = d2;
            best     = rec.id;
        }
    }
    return best;
}
