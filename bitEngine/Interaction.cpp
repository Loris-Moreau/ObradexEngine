// ============================================================
//  Interaction.cpp  —  World Interaction System
// ============================================================

#include "Interaction.h"
#include "World.h"
#include <iostream>

namespace Interaction
{

// ── SpawnDoor ─────────────────────────────────────────────────
EntityID SpawnDoor(World&            world,
                   const glm::vec3&  position,
                   bool              locked,
                   std::function<void()> onOpen)
{
    EntityID e = world.CreateEntity("Door");

    auto* t = world.AddTransform(e);
    t->position = position;
    t->scale    = {0.05f, 2.2f, 1.0f};  // Thin, tall door panel

    auto* m = world.AddMesh(e);
    m->albedoColour = {0.30f, 0.22f, 0.10f};  // Dark wood
    m->roughness    = 0.85f;

    // Mutable door state captured by the lambda.
    // Note: MSVC does not allow anonymous structs as template arguments,
    // so we define a named struct here instead.
    struct DoorState { bool open = false; bool locked = false; };
    auto state = std::make_shared<DoorState>();
    state->locked = locked;

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.5f;
    ia->promptText = locked ? "[F] Locked" : "[F] Open door";

    ia->onInteract = [e, &world, state, onOpen, ia]()
    {
        if (state->locked)
        {
            std::cout << "[Interaction] Door is locked.\n";
            return;
        }
        state->open = !state->open;
        ia->promptText = state->open ? "[F] Close door" : "[F] Open door";

        // Visual feedback: rotate the door 90° by adjusting transform scale
        // (In a full engine, you'd animate the rotation via a tween system)
        auto* tr = world.GetTransform(e);
        if (tr) tr->scale.z = state->open ? 0.05f : 1.0f;  // Stub: swap depth

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
    t->scale    = {0.6f, 0.4f, 0.4f};  // Chest-sized box

    auto* m = world.AddMesh(e);
    m->albedoColour = {0.35f, 0.25f, 0.12f};
    m->roughness    = 0.9f;

    auto opened = std::make_shared<bool>(false);
    auto* ia    = world.AddInteractable(e);
    ia->range      = 2.0f;
    ia->promptText = "[F] Search";

    ia->onInteract = [opened, items, onOpen, ia]()
    {
        if (*opened) { std::cout << "[Interaction] Already searched.\n"; return; }
        *opened = true;
        ia->promptText = "(Empty)";
        ia->enabled    = false;

        std::cout << "[Interaction] Container opened. Contents:\n";
        for (const auto& item : items)
            std::cout << "  - " << item.name
                      << " x" << item.quantity << "\n";

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
    m->albedoColour = {0.75f, 0.65f, 0.25f};  // Golden tint
    m->specular     = 0.7f;
    m->roughness    = 0.3f;

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.0f;
    ia->promptText = "[F] Pick up " + item.name;

    ia->onInteract = [e, &world, item, onPickup, ia]()
    {
        std::cout << "[Interaction] Picked up: " << item.name << "\n";
        ia->enabled = false;

        // Hide the pickup by scaling to zero (deactivate entity)
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
    m->albedoColour = {0.7f, 0.07f, 0.07f};  // Red alarm box
    m->roughness    = 0.5f;
    m->specular     = 0.4f;

    // Small red point light — glows when armed
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
    ia->promptText = "[F] Defuse alarm";

    ia->onInteract = [armed, &world, lightE, onTrigger, onDefuse, ia]()
    {
        if (*armed)
        {
            *armed = false;
            ia->promptText = "(Defused)";
            ia->enabled    = false;

            // Dim the alarm light
            auto* lc2 = world.GetRecord(lightE)->light;
            if (lc2) lc2->intensity = 0.f;

            std::cout << "[Interaction] Alarm defused.\n";
            if (onDefuse) onDefuse();
        }
        else
        {
            std::cout << "[Interaction] Alarm already disarmed.\n";
        }
    };

    return e;
}

} // namespace Interaction
