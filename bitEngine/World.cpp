// ============================================================
//  World.cpp
// ============================================================

#include "World.h"
#include "Shader.h"
#include "Mesh.h"
#include "Input.h"
#include "Interaction.h"
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

// ── ReserveComponentStorage ───────────────────────────────────
void World::ReserveComponentStorage()
{
    // EntityRecord stores raw pointers into these vectors.
    // Reserving up-front prevents reallocation, which would dangle them.
    constexpr size_t MAX = 1024;
    m_records.reserve(MAX);
    m_transforms.reserve(MAX);
    m_meshes.reserve(MAX);
    m_lights.reserve(MAX);
    m_interactables.reserve(MAX);
    m_triggers.reserve(MAX);
    m_collisions.reserve(MAX);
}

// ── Init ──────────────────────────────────────────────────────
void World::Init()
{
    ReserveComponentStorage();

    m_cubeMesh   = std::make_unique<Mesh>(Mesh::MakeCube());
    m_planeMesh  = std::make_unique<Mesh>(Mesh::MakePlane(1.f, 1));
    m_sphereMesh = std::make_unique<Mesh>(Mesh::MakeSphere(1.f, 16));

    LoadTestLevel();
}

// ── ClearLevel ────────────────────────────────────────────────
void World::ClearLevel()
{
    m_records.clear();
    m_transforms.clear();
    m_meshes.clear();
    m_lights.clear();
    m_interactables.clear();
    m_triggers.clear();
    m_collisions.clear();
    m_nextID = 0;

    // Primitive meshes are kept — they are GPU resources that persist
    // across level loads.  Re-reserve so we don't reallocate next time.
    ReserveComponentStorage();

    std::cout << "[World] Level cleared.\n";
}

// ── LoadTestLevel ─────────────────────────────────────────────
void World::LoadTestLevel()
{
    // ── Ground ────────────────────────────────────────────────
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

    // ── Crates ────────────────────────────────────────────────
    const glm::vec3 cratePos[] = {
        {3.f,0.5f,5.f}, {-2.f,0.5f,8.f}, {5.f,0.5f,2.f},
        {-5.f,0.5f,3.f},{0.f,0.5f,10.f}
    };
    for (auto& pos : cratePos)
    {
        EntityID e = CreateEntity("Crate");
        auto* t = AddTransform(e);
        t->position = pos;
        t->scale    = {1.f, 1.f, 1.f};

        auto* m = AddMesh(e);
        m->mesh         = m_cubeMesh.get();
        m->albedoColour = {0.69f, 0.39f, 0.098f};
        m->roughness    = 0.9f;

        auto* col = AddCollision(e);
        col->halfExtents = {0.5f, 0.5f, 0.5f};
    }

    // ── Lamppost — via Interaction factory ────────────────────
    Interaction::SpawnLamppost(*this, {6.f, 0.f, 0.f});

    // ── Exit marker ───────────────────────────────────────────
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

    // ── Exit trigger ──────────────────────────────────────────
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
