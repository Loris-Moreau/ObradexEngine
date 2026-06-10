#pragma once

// Interaction.h - Factory functions for interactive world entities.
//
// Each factory spawns one or more entities, wires up an onInteract callback,
// and returns the root EntityID for further configuration.
//
// SpawnLamppost   - vertical post + child light, interact toggles on/off
// SpawnDoor       - pivoting door panel, optional locked state
// SpawnContainer  - searchable chest, 3x3 item grid rendered by EditorUI
// SpawnPickup     - collectible item, hides on grab
// SpawnAlarm      - armed alarm box with defuse interact

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "Input.h"
#include "World.h"

namespace Interaction
{
    // Returns "[F] " (or the configured key name) as a prompt prefix.
    std::string GetInteractionKey();

    EntityID SpawnLamppost(World&           world,
                           const glm::vec3& basePosition,
                           glm::vec3        lightColour  = {1.0f, 0.85f, 0.5f},
                           float            lightRadius  = 10.f,
                           float            intensity    = 1.5f,
                           bool             flicker      = true);

    EntityID SpawnDoor(World&                world,
                       const glm::vec3&      position,
                       bool                  locked = false,
                       std::function<void()> onOpen = nullptr);

    // items is capped to 9 at spawn time to match the 3x3 grid.
    EntityID SpawnContainer(World&            world,
                            const glm::vec3&  position,
                            std::vector<Item> items = {},
                            std::function<void(const std::vector<Item>&)> onOpen = nullptr);

    EntityID SpawnPickup(World&                          world,
                         const glm::vec3&                position,
                         const Item&                     item,
                         std::function<void(const Item&)> onPickup = nullptr);

    EntityID SpawnAlarm(World&                world,
                        const glm::vec3&      position,
                        std::function<void()> onTrigger = nullptr,
                        std::function<void()> onDefuse  = nullptr);

    // Place a spawn point entity. Sets World::m_spawnPos.
    // Does not produce a visible mesh; visible only in the level editor.
    EntityID SpawnPoint(World& world, const glm::vec3& position);

} // namespace Interaction
