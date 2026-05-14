#pragma once

// ============================================================
//  Interaction.h  —  World Interaction System
// ============================================================
//  Higher-level interaction helpers layered on top of the
//  basic InteractableComponent.  Provides ready-made
//  interaction types for common gameplay patterns seen in
//  Deus Ex and Assassin's Creed:
//
//    Door        — swing open / close with optional lock state
//    Container   — open a chest / drawer, reveal item list
//    Terminal    — hacking mini-game entry point (stub)
//    Alarm       — trigger / defuse an alarm circuit
//    Pickup      — collect an item into the player's inventory
//
//  Each factory function creates an Entity with the right
//  components already attached and returns the EntityID.
//  The caller owns any custom onInteract logic via lambdas.
// ============================================================

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>

#include "Input.h"
#include "World.h"

// ── Item (simple inventory POD) ───────────────────────────────
struct Item
{
    std::string name;
    std::string description;
    int         quantity = 1;
};

// ── Interaction factory namespace ─────────────────────────────
namespace Interaction
{
    std::string GetInteractionKey();

    // ── Door ──────────────────────────────────────────────────
    /// Spawn a door entity.
    /// @param world        Scene to add the entity to.
    /// @param position     Hinge position in world space.
    /// @param locked       If true, displays "Locked" prompt until unlocked.
    /// @param onOpen       Optional callback when door opens.
    EntityID SpawnDoor(World&           world,
                       const glm::vec3& position,
                       bool             locked   = false,
                       std::function<void()> onOpen = nullptr);

    // ── Container ─────────────────────────────────────────────
    /// Spawn a container (chest / drawer / cabinet).
    /// @param items        Items revealed when opened.
    EntityID SpawnContainer(World&                 world,
                            const glm::vec3&       position,
                            std::vector<Item>      items,
                            std::function<void(const std::vector<Item>&)> onOpen = nullptr);

    // ── Pickup ────────────────────────────────────────────────
    /// Spawn a collectible item on the ground.
    EntityID SpawnPickup(World&           world,
                         const glm::vec3& position,
                         const Item&      item,
                         std::function<void(const Item&)> onPickup = nullptr);

    // ── Alarm ─────────────────────────────────────────────────
    /// Spawn an alarm box that can be triggered or defused.
    EntityID SpawnAlarm(World&           world,
                        const glm::vec3& position,
                        std::function<void()> onTrigger = nullptr,
                        std::function<void()> onDefuse  = nullptr);

} // namespace Interaction
