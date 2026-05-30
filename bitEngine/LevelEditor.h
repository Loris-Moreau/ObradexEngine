#pragma once

#include "World.h"  // for Item

// ============================================================
//  LevelEditor.h  -  ImGui Level Editor + Save/Load
// ============================================================
//  Provides a visual editor panel for placing, inspecting,
//  and deleting entities at runtime, plus saving/loading
//  levels to a plain-text .lvl file.
//
//  File format (.lvl)
//  ------------------
//  Human-readable, one section per entity.  Example:
//
//    LEVEL_VERSION 1
//
//    ENTITY
//      TYPE cube
//      NAME Crate
//      POS 3.0 0.5 5.0
//      SCALE 1.0 1.0 1.0
//      ALBEDO 0.69 0.39 0.098
//      SPECULAR 0.2
//      ROUGHNESS 0.9
//      COLLISION 0.5 0.5 0.5
//    END
//
//    ENTITY
//      TYPE lamppost
//      NAME Lamppost
//      POS 6.0 0.0 0.0
//      LIGHT_COLOR 1.0 0.85 0.5
//      LIGHT_RADIUS 10.0
//      LIGHT_INTENSITY 1.5
//      LIGHT_FLICKER 1
//    END
//
//  Supported TYPE values
//  ---------------------
//    cube      - static box (mesh + optional collision)
//    plane     - flat plane (floor / ceiling / wall)
//    lamppost  - Interaction::SpawnLamppost factory
//    door      - Interaction::SpawnDoor factory
//    container - Interaction::SpawnContainer factory
//    pickup    - Interaction::SpawnPickup factory
//    alarm     - Interaction::SpawnAlarm factory
//    light     - standalone point light (no mesh)
// ============================================================

#include <string>
#include <glm/glm.hpp>

class Engine;
class World;

class LevelEditor
{
public:
    LevelEditor() = default;

    /// Draw the Level Editor ImGui panel. Called from EditorUI.
    void RenderPanel(Engine& engine);

    /// Serialise the world's current entity list to a .lvl file.
    /// Returns true on success, sets m_statusMsg on failure.
    bool SaveLevel(const World& world, const std::string& path);

    /// Deserialise a .lvl file into the world (calls ClearLevel first).
    /// Returns true on success.
    bool LoadLevel(World& world, const std::string& path);

private:
    // ── Spawn helpers ─────────────────────────────────────────
    void SpawnCurrent(Engine& engine);  ///< Spawn entity of m_spawnType at m_spawnPos

    // ── State ─────────────────────────────────────────────────
    char  m_filenameBuffer[256] = "level.lvl";

    // -- Spawn panel --
    int       m_spawnType     = 0;               // index into kSpawnTypeNames
    float     m_spawnPos[3]   = {0.f, 0.5f, 5.f};
    float     m_spawnScale[3] = {1.f, 1.f, 1.f};
    float     m_spawnColor[3] = {0.7f, 0.7f, 0.7f};
    float     m_spawnSpecular = 0.2f;
    float     m_spawnRoughness= 0.8f;
    bool      m_spawnCollision= true;

    // Light params (used when spawnType == lamppost / light)
    float     m_lightColor[3]     = {1.0f, 0.85f, 0.5f};
    float     m_lightRadius       = 10.f;
    float     m_lightIntensity    = 1.5f;
    bool      m_lightFlicker      = true;

    // Pickup name
    char      m_pickupName[64] = "Item";

    // Container item editor
    std::vector<Item> m_containerItems;           ///< Items for the next spawned container
    char              m_newItemName[64] = "Item"; ///< Staging field for the add-item form
    int               m_newItemQty      = 1;      ///< Staging quantity
    bool              m_clearItemsAfterPlace = true; ///< Clear list after each Place click

    // -- Status message shown after save/load --
    std::string m_statusMsg;
    float       m_statusTimer = 0.f;  // Seconds remaining to display

    static constexpr const char* kSpawnTypeNames[] = {
        "Cube", "Plane", "Sphere", "Lamppost", "Door",
        "Container", "Pickup", "Alarm", "Point Light"
    };
    static constexpr int kSpawnTypeCount = 9;
};
