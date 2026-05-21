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
// The door panel pivots 90° around a hinge at one end.
//
// Geometry (scale = {0.05, 2.2, 1.0}):
//   Closed: panel width (1 m) runs along Z, centred at `position`.
//   Hinge : Z-negative end of the panel → position - {0, 0, 0.5}
//   Open  : panel rotates 90° CCW around Y at the hinge point.
//           New centre = hinge + {+0.5, 0, 0}
//           (right-hand 90° rotation maps local +Z to global +X)
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

    // DoorState stores the hinge and initial centre so the pivot
    // calculation works correctly regardless of where the door is placed.
    struct DoorState
    {
        bool      open      = false;
        bool      locked    = false;
        glm::vec3 closedPos;   ///< Original centre position (closed)
        glm::vec3 openPos;     ///< Centre position when fully open
    };
    auto state       = std::make_shared<DoorState>();
    state->locked    = locked;
    state->closedPos = position;
    // Hinge at Z-negative end of the panel (position.z - 0.5)
    // After 90° CCW rotation around Y: new centre = hinge + {+0.5, 0, 0}
    glm::vec3 hinge  = position - glm::vec3(0.f, 0.f, 0.5f);
    state->openPos   = hinge + glm::vec3(0.5f, 0.f, 0.f);

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.5f;
    ia->promptText = locked ? GetInteractionKey() + "Locked"
                            : GetInteractionKey() + "Open door";

    ia->onInteract = [e, &world, state, onOpen, ia]()
    {
        if (state->locked)
        {
            std::cout << "[Interaction] Door is locked.\n";
            return;
        }

        state->open    = !state->open;
        ia->promptText = state->open ? GetInteractionKey() + "Close door"
                                     : GetInteractionKey() + "Open door";

        auto* tr = world.GetTransform(e);
        if (tr)
        {
            if (state->open)
            {
                // Pivot 90° CCW around Y at the hinge point
                tr->rotation = glm::angleAxis(glm::radians(90.f),
                                              glm::vec3(0.f, 1.f, 0.f));
                tr->position = state->openPos;
            }
            else
            {
                // Return to closed pose
                tr->rotation = glm::identity<glm::quat>();
                tr->position = state->closedPos;
            }
        }

        std::cout << "[Interaction] Door "
                  << (state->open ? "opened" : "closed") << ".\n";
        if (state->open && onOpen) onOpen();
    };

    return e;
}

// ── SpawnContainer ────────────────────────────────────────────
// The onInteract callback just sets container->isOpen = true.
// The actual item grid + grab logic is handled by EditorUI::DrawContainerPopup,
// which renders a 3x3 ImGui grid and removes items as the player grabs them.
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

    // ContainerComponent holds the item list.
    // EditorUI reads isOpen and renders the grid popup.
    auto* container    = world.AddContainer(e);
    container->items   = std::move(items);
    container->isOpen  = false;

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.0f;
    ia->promptText = GetInteractionKey() + "Search";

    ia->onInteract = [e, &world, ia]()
    {
        auto* rec = world.GetRecord(e);
        if (!rec || !rec->container) return;

        if (rec->container->items.empty())
        {
            // Already looted — update prompt so player knows
            ia->promptText = GetInteractionKey() + "Search  (empty)";
            rec->container->isOpen = true;   // still show the empty grid
            return;
        }
        rec->container->isOpen = true;
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
