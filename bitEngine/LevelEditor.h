#pragma once

// LevelEditor.h - Runtime level editor and save/load.
//
// Provides an ImGui panel for placing and deleting entities, saving the
// current scene to a .lvl file, and loading it back.
//
// File format: plain-text, one ENTITY ... END block per entity.
// All files go in the Levels/ subdirectory. LevelPath() strips any
// directory the user types so the location is always enforced.
//
// Example entity block:
//
//   ENTITY
//     TYPE   cube
//     NAME   Crate
//     POS    3.0 0.5 5.0
//     SCALE  1.0 1.0 1.0
//     ALBEDO 0.69 0.39 0.098
//     SPECULAR  0.2
//     ROUGHNESS 0.9
//     COLLISION 0.5 0.5 0.5
//   END
//
// Supported TYPE values:
//   cube, plane, sphere, lamppost, door, container, pickup, alarm, light, spawn

#include "World.h"
#include <string>
#include <glm/glm.hpp>

class Engine;
class World;

class LevelEditor
{
public:
    LevelEditor() = default;

    void RenderPanel(Engine& engine);

    bool SaveLevel(const World& world, const std::string& path);
    bool LoadLevel(World& world,       const std::string& path);

private:
    void SpawnCurrent(Engine& engine);

    char  m_filenameBuffer[256] = "level.lvl";

    int   m_spawnType      = 0;
    float m_spawnPos[3]    = {0.f, 0.5f, 5.f};
    float m_spawnScale[3]  = {1.f, 1.f,  1.f};
    float m_spawnColor[3]  = {0.7f, 0.7f, 0.7f};
    float m_spawnSpecular  = 0.2f;
    float m_spawnRoughness = 0.8f;
    bool  m_spawnCollision = true;
    bool  m_spawnSlippery  = false;

    float m_lightColor[3]  = {1.0f, 0.85f, 0.5f};
    float m_lightRadius    = 10.f;
    float m_lightIntensity = 1.5f;
    bool  m_lightFlicker   = true;

    char m_pickupName[64] = "Item";

    std::vector<Item> m_containerItems;
    char              m_newItemName[64] = "Item";
    int               m_newItemQty      = 1;
    bool              m_clearItemsAfterPlace = true;

    std::string m_statusMsg;
    float       m_statusTimer = 0.f;

    static constexpr const char* kSpawnTypeNames[] = {
        "Cube", "Plane", "Sphere", "Lamppost", "Door",
        "Container", "Pickup", "Alarm", "Point Light", "Spawn Point"
    };
    static constexpr int kSpawnTypeCount = 10;
};
