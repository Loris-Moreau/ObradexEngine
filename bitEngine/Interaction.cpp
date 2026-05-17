// ============================================================
//  Interaction.cpp  —  World Interaction Factories
// ============================================================

#include "Interaction.h"
#include "World.h"
#include <iostream>

namespace Interaction
{

// ── GetInteractionKey ─────────────────────────────────────────
std::string GetInteractionKey()
{
    return std::string("[") + Input::GetKeyName(INTERACT_KEY) + "] ";
}

// ── SpawnLamppost ─────────────────────────────────────────────
// Creates two entities:
//   1. The post (visible mesh + interactable + collision)
//   2. A child light entity at the top
// The post's onInteract toggles the child light on/off.
EntityID SpawnLamppost(World&           world,
                       const glm::vec3& basePosition,
                       glm::vec3        lightColour,
                       float            lightRadius,
                       float            intensity,
                       bool             flicker)
{
    const float kPostHeight = 4.0f;

    // ── Post entity ───────────────────────────────────────────
    EntityID postE = world.CreateEntity("Lamppost");

    auto* t = world.AddTransform(postE);
    t->position = basePosition + glm::vec3(0.f, kPostHeight * 0.5f, 0.f);
    t->scale    = {0.15f, kPostHeight, 0.15f};

    auto* m = world.AddMesh(postE);
    m->albedoColour = {0.45f, 0.44f, 0.43f};  // Gunmetal grey
    m->roughness    = 0.6f;
    m->specular     = 0.4f;
    m->mesh         = world.GetCubeMesh();     // assigned by World

    auto* col = world.AddCollision(postE);
    col->halfExtents = {0.5f, 0.5f, 0.5f};    // scaled by t->scale at runtime

    // ── Light entity (child of the post) ─────────────────────
    EntityID lightE = world.CreateEntity("LampostLight");

    auto* lt = world.AddTransform(lightE);
    lt->position = basePosition + glm::vec3(0.f, kPostHeight + 0.5f, 0.f);

    auto* lc = world.AddLight(lightE);
    lc->colour    = lightColour;
    lc->radius    = lightRadius;
    lc->intensity = intensity;
    lc->flicker   = flicker;

    // ── Interact: toggle light on / off ───────────────────────
    auto* ia = world.AddInteractable(postE);
    ia->range      = 3.0f;
    ia->promptText = GetInteractionKey() + "Toggle lamp";

    ia->onInteract = [&world, lightE, ia]()
    {
        auto* rec = world.GetRecord(lightE);
        if (!rec || !rec->light) return;
        LightComponent& lc2 = *rec->light;

        if (lc2.intensity > 0.f)
        {
            lc2.intensity = 0.f;
            ia->promptText = GetInteractionKey() + "Turn on lamp";
        }
        else
        {
            lc2.intensity = 1.5f;
            ia->promptText = GetInteractionKey() + "Turn off lamp";
        }
    };

    return postE;
}

// ── SpawnDoor ─────────────────────────────────────────────────
EntityID SpawnDoor(World&            world,
                   const glm::vec3&  position,
                   bool              locked,
                   std::function<void()> onOpen)
{
    EntityID e = world.CreateEntity("Door");

    auto* t = world.AddTransform(e);
    t->position = position;
    t->scale    = {0.05f, 2.2f, 1.0f};

    auto* m = world.AddMesh(e);
    m->albedoColour = {0.30f, 0.22f, 0.10f};
    m->roughness    = 0.85f;
    m->mesh         = world.GetCubeMesh();

    auto* col = world.AddCollision(e);
    col->halfExtents = {0.5f, 0.5f, 0.5f};

    struct DoorState { bool open = false; bool locked = false; };
    auto state     = std::make_shared<DoorState>();
    state->locked  = locked;

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.5f;
    ia->promptText = locked ? GetInteractionKey() + "Locked"
                            : GetInteractionKey() + "Open door";

    ia->onInteract = [e, &world, state, onOpen, ia]()
    {
        if (state->locked) { std::cout << "[Interaction] Door is locked.\n"; return; }

        state->open    = !state->open;
        ia->promptText = state->open ? GetInteractionKey() + "Close door"
                                     : GetInteractionKey() + "Open door";

        auto* tr = world.GetTransform(e);
        if (tr) tr->scale.z = state->open ? 0.05f : 1.0f;

        std::cout << "[Interaction] Door " << (state->open ? "opened" : "closed") << ".\n";
        if (state->open && onOpen) onOpen();
    };

    return e;
}

// ── SpawnContainer ────────────────────────────────────────────
EntityID SpawnContainer(World&                world,
                        const glm::vec3&      position,
                        std::vector<Item>     items,
                        std::function<void(const std::vector<Item>&)> onOpen)
{
    EntityID e = world.CreateEntity("Container");

    auto* t = world.AddTransform(e);
    t->position = position;
    t->scale    = {0.6f, 0.4f, 0.4f};

    auto* m = world.AddMesh(e);
    m->albedoColour = {0.35f, 0.25f, 0.12f};
    m->roughness    = 0.9f;
    m->mesh         = world.GetCubeMesh();

    auto opened = std::make_shared<bool>(false);
    auto* ia    = world.AddInteractable(e);
    ia->range      = 2.0f;
    ia->promptText = GetInteractionKey() + "Search";

    ia->onInteract = [opened, items, onOpen, ia]()
    {
        if (*opened) { std::cout << "[Interaction] Already searched.\n"; return; }
        *opened = true;
        ia->promptText = "(Empty)";
        ia->enabled    = false;

        std::cout << "[Interaction] Container opened.\n";
        for (const auto& item : items)
            std::cout << "  - " << item.name << " x" << item.quantity << "\n";

        if (onOpen) onOpen(items);
    };

    return e;
}

// ── SpawnPickup ───────────────────────────────────────────────
EntityID SpawnPickup(World&           world,
                     const glm::vec3& position,
                     const Item&      item,
                     std::function<void(const Item&)> onPickup)
{
    EntityID e = world.CreateEntity("Pickup_" + item.name);

    auto* t = world.AddTransform(e);
    t->position = position;
    t->scale    = {0.15f, 0.15f, 0.15f};

    auto* m = world.AddMesh(e);
    m->albedoColour = {0.75f, 0.65f, 0.25f};
    m->specular     = 0.7f;
    m->roughness    = 0.3f;
    m->mesh         = world.GetCubeMesh();

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.0f;
    ia->promptText = GetInteractionKey() + "Pick up " + item.name;

    ia->onInteract = [e, &world, item, onPickup, ia]()
    {
        std::cout << "[Interaction] Picked up: " << item.name << "\n";
        ia->enabled = false;
        auto* tr = world.GetTransform(e);
        if (tr) tr->scale = {0.f, 0.f, 0.f};
        if (onPickup) onPickup(item);
    };

    return e;
}

// ── SpawnAlarm ────────────────────────────────────────────────
EntityID SpawnAlarm(World&            world,
                    const glm::vec3&  position,
                    std::function<void()> onTrigger,
                    std::function<void()> onDefuse)
{
    EntityID e = world.CreateEntity("AlarmBox");

    auto* t = world.AddTransform(e);
    t->position = position;
    t->scale    = {0.2f, 0.2f, 0.1f};

    auto* m = world.AddMesh(e);
    m->albedoColour = {0.7f, 0.07f, 0.07f};
    m->roughness    = 0.5f;
    m->specular     = 0.4f;
    m->mesh         = world.GetCubeMesh();

    EntityID lightE = world.CreateEntity("AlarmLight");
    auto* lt = world.AddTransform(lightE);
    lt->position = position + glm::vec3(0.f, 0.1f, 0.f);
    auto* lc = world.AddLight(lightE);
    lc->colour    = {1.f, 0.1f, 0.1f};
    lc->radius    = 3.f;
    lc->intensity = 0.6f;

    auto armed = std::make_shared<bool>(true);
    auto* ia   = world.AddInteractable(e);
    ia->range      = 1.5f;
    ia->promptText = GetInteractionKey() + "Defuse alarm";

    ia->onInteract = [armed, &world, lightE, onDefuse, ia]()
    {
        if (!*armed) { std::cout << "[Interaction] Already disarmed.\n"; return; }
        *armed         = false;
        ia->promptText = "(Defused)";
        ia->enabled    = false;

        auto* rec = world.GetRecord(lightE);
        if (rec && rec->light) rec->light->intensity = 0.f;

        std::cout << "[Interaction] Alarm defused.\n";
        if (onDefuse) onDefuse();
    };

    return e;
}

} // namespace Interaction
