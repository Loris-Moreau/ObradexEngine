#pragma once

// ============================================================
//  Interaction.h  -  World Interaction Factories
// ============================================================
//  Factory functions that spawn pre-wired interactive entities.
//  Each returns the root EntityID so the caller can further
//  configure or link entities.
//
//  Supported factory types
//  -----------------------
//    SpawnLamppost   - post + child light entity + interact toggle
//    SpawnDoor       - openable / lockable door panel
//    SpawnContainer  - searchable chest / drawer
//    SpawnPickup     - collectible ground item
//    SpawnAlarm      - armed alarm box with defuse interact
// ============================================================

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include "Input.h"
#include "World.h"

// Item is defined in World.h (included below via World.h)
// so all subsystems share one type without circular includes.

// ── Interaction factory namespace ─────────────────────────────
namespace Interaction
{
    /// Returns "[X] " prefix using the globally configured INTERACT_KEY.
    std::string GetInteractionKey();

    // ── Lamppost ──────────────────────────────────────────────
    /// Spawn a street/room lamp: a post entity + a child light entity.
    /// The post is interactable (toggles the light on/off).
    ///
    /// @param world        Scene to add entities to.
    /// @param basePosition Base (foot) of the post in world space.
    /// @param lightColour  Warm amber by default.
    /// @param lightRadius  Attenuation distance in metres.
    /// @param intensity    Initial brightness scalar.
    /// @param flicker      Enable candle-flicker simulation.
    /// @returns EntityID of the post (root entity).
    EntityID SpawnLamppost(World&           world,
                           const glm::vec3& basePosition,
                           glm::vec3        lightColour  = {1.0f, 0.85f, 0.5f},
                           float            lightRadius  = 10.f,
                           float            intensity    = 1.5f,
                           bool             flicker      = true);

    // ── Door ──────────────────────────────────────────────────
    EntityID SpawnDoor(World&           world,
                       const glm::vec3& position,
                       bool             locked = false,
                       std::function<void()> onOpen = nullptr);

    // ── Container ─────────────────────────────────────────────
    EntityID SpawnContainer(World&            world,
                            const glm::vec3&  position,
                            std::vector<Item> items = {},
                            std::function<void(const std::vector<Item>&)> onOpen = nullptr);

    // ── Pickup ────────────────────────────────────────────────
    EntityID SpawnPickup(World&           world,
                         const glm::vec3& position,
                         const Item&      item,
                         std::function<void(const Item&)> onPickup = nullptr);

    // ── Alarm ─────────────────────────────────────────────────
    EntityID SpawnAlarm(World&           world,
                        const glm::vec3& position,
                        std::function<void()> onTrigger = nullptr,
                        std::function<void()> onDefuse  = nullptr);

} // namespace Interaction
