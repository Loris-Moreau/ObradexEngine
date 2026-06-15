// Interaction.cpp - Factory functions for interactive world entities.

#include "Interaction.h"
#include "World.h"
#include <iostream>

#include "Engine.h"
#include "AudioSystem.h"
#include "Player.h"

namespace Interaction
{

std::string GetInteractionKey()
{
    return std::string("[") + Input::GetKeyName(INTERACT_KEY) + "] ";
}

EntityID SpawnLamppost(World&           world,
                       const glm::vec3& basePosition,
                       glm::vec3        lightColour,
                       float            lightRadius,
                       float            intensity,
                       bool             flicker)
{
    const float kPostHeight = 4.0f;

    EntityID postE = world.CreateEntity("Lamppost");
    auto* t = world.AddTransform(postE);
    t->position = basePosition + glm::vec3(0.f, kPostHeight * 0.5f, 0.f);
    t->scale    = {0.15f, kPostHeight, 0.15f};
    auto* m = world.AddMesh(postE);
    m->albedoColour = {0.45f, 0.44f, 0.43f};
    m->roughness    = 0.6f;
    m->specular     = 0.4f;
    m->mesh         = world.GetCubeMesh();
    auto* col = world.AddCollision(postE);
    col->halfExtents = {0.5f, 0.5f, 0.5f};

    // Child light entity positioned at the top of the post.
    EntityID lightE = world.CreateEntity("LampostLight");
    auto* lt = world.AddTransform(lightE);
    lt->position = basePosition + glm::vec3(0.f, kPostHeight + 0.5f, 0.f);
    auto* lc = world.AddLight(lightE);
    lc->colour    = lightColour;
    lc->radius    = lightRadius;
    lc->intensity = intensity;
    lc->flicker   = flicker;

    auto* ia = world.AddInteractable(postE);
    ia->range      = 3.0f;
    ia->promptText = GetInteractionKey() + "Toggle lamp";

    ia->sounds.onOpen  = "sfx_switch";
    ia->sounds.onClose = "sfx_switch";

    ia->onInteract = [&world, lightE, ia]()
    {
        auto* rec = world.GetRecord(lightE);
        if (!rec || !rec->light) return;
        LightComponent& lc2 = *rec->light;

        if (lc2.intensity > 0.f)
        {
            lc2.intensity  = 0.f;
            ia->promptText = GetInteractionKey() + "Turn on lamp";
            Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onClose);
        }
        else
        {
            lc2.intensity  = 1.5f;
            ia->promptText = GetInteractionKey() + "Turn off lamp";
            Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onOpen);
        }
    };

    return postE;
}

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

    // Stores pivot data so the hinge calculation is independent of spawn position.
    struct DoorState
    {
        bool      open      = false;
        bool      locked    = false;
        glm::vec3 closedPos;
        glm::vec3 openPos;
    };
    auto state       = std::make_shared<DoorState>();
    state->locked    = locked;
    state->closedPos = position;
    // Hinge at the Z-negative end of the panel.
    // 90-degree CCW rotation around Y maps local +Z to global +X,
    // so the new centre is hinge + {+0.5, 0, 0}.
    glm::vec3 hinge = position - glm::vec3(0.f, 0.f, 0.5f);
    state->openPos  = hinge + glm::vec3(0.5f, 0.f, 0.f);

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.5f;
    ia->promptText = locked ? GetInteractionKey() + "Locked"
                            : GetInteractionKey() + "Open door";

    ia->sounds.onOpen  = "sfx_door_open";
    ia->sounds.onClose = "sfx_door_close";

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

        auto* tr  = world.GetTransform(e);
        auto* rec = world.GetRecord(e);
        if (tr)
        {
            if (state->open)
            {
                tr->rotation = glm::angleAxis(glm::radians(90.f),
                                              glm::vec3(0.f, 1.f, 0.f));
                tr->position = state->openPos;
            }
            else
            {
                tr->rotation = glm::identity<glm::quat>();
                tr->position = state->closedPos;
            }
        }

        std::cout << "[Interaction] Door "
                  << (state->open ? "opened" : "closed") << ".\n";
        if (state->open)
        {
            Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onOpen);
            if (onOpen) onOpen();
        }
        else
        {
            Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onClose);
        }
    };

    return e;
}

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

    auto* container = world.AddContainer(e);
    // Enforce the 3x3 grid cap; silently drop items beyond index 8.
    if (items.size() > 9) items.resize(9);
    container->items  = std::move(items);
    container->isOpen = false;

    auto* ia = world.AddInteractable(e);
    ia->range      = 2.25f;
    ia->promptText = GetInteractionKey() + "Search";

    ia->sounds.onOpen = "sfx_container_open";

    ia->onInteract = [e, &world, ia]()
    {
        auto* rec = world.GetRecord(e);
        if (!rec || !rec->container) return;

        if (rec->container->items.empty())
            ia->promptText = GetInteractionKey() + "Search  (empty)";

        rec->container->isOpen = true;
        Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onOpen);
    };

    return e;
}

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
    ia->range      = 2.25f;
    ia->promptText = GetInteractionKey() + "Pick up " + item.name;

    ia->sounds.onOpen = "sfx_pickup";

    ia->onInteract = [e, &world, item, onPickup, ia]()
    {
        std::cout << "[Interaction] Picked up: " << item.name << "\n";
        ia->enabled = false;
        auto* tr = world.GetTransform(e);
        if (tr) tr->scale = {0.f, 0.f, 0.f};
        Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onOpen);
        if (onPickup) onPickup(item);
    };
    return e;
}

EntityID SpawnAlarm(World&                world,
                    const glm::vec3&      position,
                    std::function<void()> onTrigger,  // Reserved: not yet called (no trigger event system)
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
    ia->range      = 2.0f;
    ia->promptText = GetInteractionKey() + "Defuse alarm";

    ia->sounds.onAlarm  = "sfx_alarm";
    ia->sounds.onDefuse = "sfx_defuse";

    ia->onInteract = [armed, &world, lightE, onDefuse, ia]()
    {
        if (!*armed) { std::cout << "[Interaction] Already disarmed.\n"; return; }
        *armed         = false;
        ia->promptText = "(Defused)";
        ia->enabled    = false;
        Engine::Get().GetAudio().PlayFromSet(ia->sounds, &SoundSet::onDefuse);

        auto* rec = world.GetRecord(lightE);
        if (rec && rec->light) rec->light->intensity = 0.f;

        std::cout << "[Interaction] Alarm defused.\n";
        if (onDefuse) onDefuse();
    };

    // Proximity trigger: while armed, deal 25 damage to the player on enter.
    auto* tri        = world.AddTrigger(e);
    tri->halfExtents = {1.5f, 1.5f, 1.5f};
    tri->onEnter     = [armed]()
    {
        if (!*armed) return;
        std::cout << "[Alarm] Player caught in alarm zone, taking damage!\n";
        Engine::Get().GetPlayer().TakeDamage(25);
    };

    return e;
}

EntityID SpawnPoint(World& world, const glm::vec3& position)
{
    world.SetSpawnPos(position);
    EntityID e = world.CreateEntity("SpawnPoint");
    auto* t = world.AddTransform(e);
    t->position = position;
    t->scale    = {0.3f, 0.3f, 0.3f};
    // Spawn points have no visible mesh in-game; shown as a marker
    // in the level editor only.
    return e;
}

} // namespace Interaction
