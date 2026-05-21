// ============================================================
//  LevelEditor.cpp  —  ImGui Level Editor + Save/Load
// ============================================================

#include "LevelEditor.h"
#include "Engine.h"
#include "World.h"
#include "Player.h"
#include "Interaction.h"
#include "Input.h"

#include <imgui.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace fs = std::filesystem;
static constexpr const char* kLevelsDir = "Levels";

// ── Build the full path: Levels/<filename> ───────────────────
static std::string LevelPath(const char* filename)
{
    // Strip any directory component the user may have typed
    // so the file always lands in the Levels/ folder.
    fs::path p(filename);
    return (fs::path(kLevelsDir) / p.filename()).string();
}

// ── Save format helpers ───────────────────────────────────────
static void Wf3(std::ofstream& f, const char* k, float x, float y, float z)
{ f << "  " << k << " " << x << " " << y << " " << z << "\n"; }

static void Wf1(std::ofstream& f, const char* k, float v)
{ f << "  " << k << " " << v << "\n"; }

static void Wi1(std::ofstream& f, const char* k, int v)
{ f << "  " << k << " " << v << "\n"; }

static void Ws(std::ofstream& f, const char* k, const std::string& v)
{ f << "  " << k << " " << v << "\n"; }

// ── SaveLevel ─────────────────────────────────────────────────
bool LevelEditor::SaveLevel(const World& world, const std::string& path)
{
    // Ensure the Levels/ directory exists
    fs::create_directories(kLevelsDir);
    std::string fullPath = LevelPath(path.c_str());
    std::ofstream file(fullPath);
    if (!file.is_open())
    {
        m_statusMsg   = "ERROR: could not open " + fullPath;
        m_statusTimer = 4.f;
        return false;
    }

    file << "LEVEL_VERSION 1\n\n";

    for (const auto& rec : world.GetAllRecords())
    {
        if (!rec.active || !rec.transform) continue;

        // ── Skip child entities FIRST (before type detection) ───────
        // LampostLight and AlarmLight are created by their parent factories
        // (SpawnLamppost / SpawnAlarm) when the parent entity is loaded.
        // If we save them as standalone "light" entities they get duplicated
        // because the factory also re-creates them on load.
        bool isFactoryChild =
            (rec.name == "LampostLight" || rec.name == "AlarmLight") && !rec.mesh;
        if (isFactoryChild) continue;

        // ── Determine entity type string ───────────────────────
        std::string type = "cube";
        if (!rec.mesh && rec.light)
            type = "light";
        else if (rec.name == "Ground" || (rec.mesh && rec.mesh->mesh == const_cast<World&>(world).GetPlaneMesh()))
            type = "plane";
        else if (rec.name.find("Lamppost") != std::string::npos ||
                 rec.name.find("LampPost") != std::string::npos)
            type = "lamppost";
        else if (rec.name.find("Door") != std::string::npos)
            type = "door";
        else if (rec.name.find("Container") != std::string::npos)
            type = "container";
        else if (rec.name.find("Pickup") != std::string::npos)
            type = "pickup";
        else if (rec.name.find("Alarm") != std::string::npos)
            type = "alarm";

        file << "ENTITY\n";
        Ws(file, "TYPE", type);
        Ws(file, "NAME", rec.name);

        // Transform
        auto& t = *rec.transform;
        // For lamppost the transform is at the post CENTER (base + halfHeight).
        // SpawnLamppost expects the BASE, so we subtract halfHeight on save
        // so the round-trip is: save base → load base → SpawnLamppost(base) → center stored.
        float saveY = (type == "lamppost") ? t.position.y - t.scale.y * 0.5f
                                           : t.position.y;
        Wf3(file, "POS",   t.position.x, saveY, t.position.z);
        Wf3(file, "SCALE", t.scale.x,    t.scale.y,    t.scale.z);

        // Mesh
        if (rec.mesh)
        {
            Wf3(file, "ALBEDO", rec.mesh->albedoColour.x,
                                rec.mesh->albedoColour.y,
                                rec.mesh->albedoColour.z);
            Wf1(file, "SPECULAR",  rec.mesh->specular);
            Wf1(file, "ROUGHNESS", rec.mesh->roughness);
        }

        // Collision
        if (rec.collision)
            Wf3(file, "COLLISION",
                rec.collision->halfExtents.x,
                rec.collision->halfExtents.y,
                rec.collision->halfExtents.z);

        // Light (for standalone lights or lamppost overrides)
        if (rec.light)
        {
            Wf3(file, "LIGHT_COLOR",
                rec.light->colour.x, rec.light->colour.y, rec.light->colour.z);
            Wf1(file, "LIGHT_RADIUS",    rec.light->radius);
            Wf1(file, "LIGHT_INTENSITY", rec.light->intensity);
            Wi1(file, "LIGHT_FLICKER",   rec.light->flicker ? 1 : 0);
        }

        // Container items — one ITEM line per entry
        // Format: ITEM <name> <quantity>
        // (names must not contain spaces; replace with _ if needed)
        if (rec.container)
        {
            for (const auto& item : rec.container->items)
            {
                // Replace spaces with underscores so the parser can split on whitespace
                std::string safeName = item.name;
                for (char& c : safeName) if (c == ' ') c = '_';
                file << "  ITEM " << safeName << " " << item.quantity << "\n";
            }
        }

        file << "END\n\n";
    }

    m_statusMsg   = "Saved: " + fullPath;
    m_statusTimer = 3.f;
    std::cout << "[LevelEditor] Saved " << world.GetAllRecords().size()
              << " entities to " << fullPath << "\n";
    return true;
}

// ── LoadLevel ─────────────────────────────────────────────────
bool LevelEditor::LoadLevel(World& world, const std::string& path)
{
    std::string fullPath = LevelPath(path.c_str());
    std::ifstream file(fullPath);
    if (!file.is_open())
    {
        m_statusMsg   = "ERROR: could not open " + fullPath;
        m_statusTimer = 4.f;
        return false;
    }

    world.ClearLevel();

    std::string line;
    // Per-entity state
    struct EData {
        std::string type, name;
        glm::vec3   pos{0,0,0}, scale{1,1,1};
        glm::vec3   albedo{0.7f,0.7f,0.7f};
        float       specular=0.2f, roughness=0.8f;
        bool        hasCollision=false;
        glm::vec3   collHalf{0.5f,0.5f,0.5f};
        bool        hasLight=false;
        glm::vec3   lightColor{1,0.85f,0.5f};
        float       lightRadius=10.f, lightIntensity=1.5f;
        bool        lightFlicker=true;
        std::vector<Item> items;  // for container entities
    };

    bool inEntity = false;
    EData cur;
    int entityCount = 0;

    auto flush = [&]()
    {
        if (cur.type == "lamppost")
        {
            Interaction::SpawnLamppost(world,
                // base pos — the POS field stores base (foot) in the file
                cur.pos,
                cur.lightColor, cur.lightRadius, cur.lightIntensity, cur.lightFlicker);
        }
        else if (cur.type == "door")
        {
            Interaction::SpawnDoor(world, cur.pos, false);
        }
        else if (cur.type == "container")
        {
            Interaction::SpawnContainer(world, cur.pos, cur.items);
        }
        else if (cur.type == "pickup")
        {
            Item it;
            // Entity name is "Pickup_ItemName" — strip prefix to recover the actual item name.
            // Also restore underscores-as-spaces (same convention used on save for item names).
            static const std::string kPrefix = "Pickup_";
            it.name = (cur.name.rfind(kPrefix, 0) == 0)
                      ? cur.name.substr(kPrefix.size())
                      : cur.name;
            for (char& c : it.name) if (c == '_') c = ' ';
            it.quantity = 1;
            Interaction::SpawnPickup(world, cur.pos, it);
        }
        else if (cur.type == "alarm")
        {
            Interaction::SpawnAlarm(world, cur.pos);
        }
        else
        {
            // cube / plane / light — build manually
            EntityID e = world.CreateEntity(cur.name);

            auto* t = world.AddTransform(e);
            t->position = cur.pos;
            t->scale    = cur.scale;

            if (cur.type != "light")
            {
                auto* m = world.AddMesh(e);
                m->mesh         = (cur.type == "plane") ? world.GetPlaneMesh()
                                                        : world.GetCubeMesh();
                m->albedoColour = cur.albedo;
                m->specular     = cur.specular;
                m->roughness    = cur.roughness;

                if (cur.hasCollision)
                {
                    auto* col = world.AddCollision(e);
                    col->halfExtents = cur.collHalf;
                }
            }

            if (cur.hasLight || cur.type == "light")
            {
                auto* lc = world.AddLight(e);
                lc->colour    = cur.lightColor;
                lc->radius    = cur.lightRadius;
                lc->intensity = cur.lightIntensity;
                lc->flicker   = cur.lightFlicker;
            }
        }
        ++entityCount;
    };

    while (std::getline(file, line))
    {
        // Strip carriage return (Windows line endings)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "ENTITY")
        {
            inEntity = true;
            cur = EData{};
        }
        else if (token == "END" && inEntity)
        {
            flush();
            inEntity = false;
        }
        else if (inEntity)
        {
            if      (token == "TYPE")    ss >> cur.type;
            else if (token == "NAME")    ss >> cur.name;
            else if (token == "POS")     ss >> cur.pos.x >> cur.pos.y >> cur.pos.z;
            else if (token == "SCALE")   ss >> cur.scale.x >> cur.scale.y >> cur.scale.z;
            else if (token == "ALBEDO")  ss >> cur.albedo.x >> cur.albedo.y >> cur.albedo.z;
            else if (token == "SPECULAR")  ss >> cur.specular;
            else if (token == "ROUGHNESS") ss >> cur.roughness;
            else if (token == "COLLISION")
            {
                cur.hasCollision = true;
                ss >> cur.collHalf.x >> cur.collHalf.y >> cur.collHalf.z;
            }
            else if (token == "LIGHT_COLOR")
            {
                cur.hasLight = true;
                ss >> cur.lightColor.x >> cur.lightColor.y >> cur.lightColor.z;
            }
            else if (token == "LIGHT_RADIUS")    { cur.hasLight=true; ss>>cur.lightRadius; }
            else if (token == "LIGHT_INTENSITY")  { cur.hasLight=true; ss>>cur.lightIntensity; }
            else if (token == "LIGHT_FLICKER")    { int v=0; ss>>v; cur.lightFlicker=(v!=0); cur.hasLight=true; }
            else if (token == "ITEM")
            {
                // Format: ITEM <name_no_spaces> <quantity>
                Item it;
                ss >> it.name >> it.quantity;
                // Restore underscores-as-spaces convention
                for (char& c : it.name) if (c == '_') c = ' ';
                cur.items.push_back(it);
            }
        }
    }

    m_statusMsg   = "Loaded: " + fullPath + " (" + std::to_string(entityCount) + " entities)";
    m_statusTimer = 3.f;
    std::cout << "[LevelEditor] Loaded " << entityCount << " entities from " << fullPath << "\n";
    return true;
}

// ── SpawnCurrent ─────────────────────────────────────────────
void LevelEditor::SpawnCurrent(Engine& engine)
{
    World& world = engine.GetWorld();
    glm::vec3 pos(m_spawnPos[0], m_spawnPos[1], m_spawnPos[2]);
    glm::vec3 scale(m_spawnScale[0], m_spawnScale[1], m_spawnScale[2]);
    glm::vec3 color(m_spawnColor[0], m_spawnColor[1], m_spawnColor[2]);
    glm::vec3 lcolor(m_lightColor[0], m_lightColor[1], m_lightColor[2]);

    switch (m_spawnType)
    {
        case 0: // Cube
        {
            EntityID e = world.CreateEntity("Cube");
            auto* t = world.AddTransform(e); t->position=pos; t->scale=scale;
            auto* m = world.AddMesh(e);
            m->mesh=world.GetCubeMesh(); m->albedoColour=color;
            m->specular=m_spawnSpecular; m->roughness=m_spawnRoughness;
            if (m_spawnCollision) {
                auto* col = world.AddCollision(e);
                col->halfExtents = {0.5f,0.5f,0.5f};
            }
            break;
        }
        case 1: // Plane
        {
            EntityID e = world.CreateEntity("Plane");
            auto* t = world.AddTransform(e); t->position=pos; t->scale=scale;
            auto* m = world.AddMesh(e);
            m->mesh=world.GetPlaneMesh(); m->albedoColour=color;
            m->roughness=m_spawnRoughness;
            break;
        }
        case 2: // Lamppost
            Interaction::SpawnLamppost(world, pos, lcolor,
                                       m_lightRadius, m_lightIntensity, m_lightFlicker);
            break;
        case 3: // Door
            Interaction::SpawnDoor(world, pos);
            break;
        case 4: // Container
            Interaction::SpawnContainer(world, pos, m_containerItems);
            if (m_clearItemsAfterPlace) m_containerItems.clear();
            break;
        case 5: // Pickup
        {
            Item it; it.name=m_pickupName; it.quantity=1;
            Interaction::SpawnPickup(world, pos, it);
            break;
        }
        case 6: // Alarm
            Interaction::SpawnAlarm(world, pos);
            break;
        case 7: // Point Light
        {
            EntityID e = world.CreateEntity("Light");
            auto* t = world.AddTransform(e); t->position=pos;
            auto* lc = world.AddLight(e);
            lc->colour=lcolor; lc->radius=m_lightRadius;
            lc->intensity=m_lightIntensity; lc->flicker=m_lightFlicker;
            break;
        }
    }
}

// ── RenderPanel ───────────────────────────────────────────────
void LevelEditor::RenderPanel(Engine& engine)
{
    World&  world  = engine.GetWorld();
    Player& player = engine.GetPlayer();

    // ── Status message ────────────────────────────────────────
    if (m_statusTimer > 0.f)
    {
        m_statusTimer -= ImGui::GetIO().DeltaTime;
        bool isErr = m_statusMsg.rfind("ERROR", 0) == 0;
        ImGui::TextColored(isErr ? ImVec4(1,0.3f,0.3f,1) : ImVec4(0.4f,1,0.4f,1),
                           "%s", m_statusMsg.c_str());
    }

    // ── File I/O ──────────────────────────────────────────────
    ImGui::SeparatorText("Level File  [Levels/ folder]");
    ImGui::TextDisabled("Files are stored in:  Levels/<filename>");
    ImGui::SetNextItemWidth(200.f);
    ImGui::InputText("##filename", m_filenameBuffer, sizeof(m_filenameBuffer));
    ImGui::SameLine();

    if (ImGui::Button("Save"))
        SaveLevel(world, m_filenameBuffer);

    ImGui::SameLine();
    if (ImGui::Button("Load"))
        LoadLevel(world, m_filenameBuffer);

    ImGui::SameLine();
    if (ImGui::Button("New"))
    {
        world.ClearLevel();
        m_statusMsg   = "Level cleared.";
        m_statusTimer = 2.f;
    }

    // ── Spawn entity ──────────────────────────────────────────
    ImGui::SeparatorText("Place Entity");

    ImGui::Combo("Type", &m_spawnType, kSpawnTypeNames, kSpawnTypeCount);
    ImGui::DragFloat3("Position##sp", m_spawnPos,   0.1f);

    // Show relevant params per type
    if (m_spawnType == 0 || m_spawnType == 1) // cube / plane
    {
        ImGui::DragFloat3("Scale##sp",    m_spawnScale,  0.05f, 0.01f, 100.f);
        ImGui::ColorEdit3("Colour##sp",   m_spawnColor);
        ImGui::SliderFloat("Roughness##sp",&m_spawnRoughness, 0.f, 1.f);
        ImGui::SliderFloat("Specular##sp", &m_spawnSpecular,  0.f, 1.f);
        if (m_spawnType == 0)
            ImGui::Checkbox("Solid collision", &m_spawnCollision);
    }
    if (m_spawnType == 2 || m_spawnType == 7) // lamppost / light
    {
        ImGui::ColorEdit3("Light colour", m_lightColor);
        ImGui::SliderFloat("Radius",    &m_lightRadius,    0.5f, 40.f);
        ImGui::SliderFloat("Intensity", &m_lightIntensity, 0.f,  5.f);
        ImGui::Checkbox   ("Flicker",   &m_lightFlicker);
    }
    if (m_spawnType == 4) // Container — item editor
    {
        ImGui::SeparatorText("Container Contents");

        // ── Item list ─────────────────────────────────────────
        if (m_containerItems.empty())
            ImGui::TextDisabled("(empty — no items)");

        int removeIdx = -1;
        for (int i = 0; i < (int)m_containerItems.size(); ++i)
        {
            ImGui::PushID(i);
            auto& it = m_containerItems[i];
            ImGui::Text("%-20s x%d", it.name.c_str(), it.quantity);
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) removeIdx = i;
            ImGui::PopID();
        }
        if (removeIdx >= 0)
            m_containerItems.erase(m_containerItems.begin() + removeIdx);

        // ── Add item form ─────────────────────────────────────
        ImGui::Spacing();
        ImGui::SetNextItemWidth(130.f);
        ImGui::InputText("##iname", m_newItemName, sizeof(m_newItemName));
        ImGui::SameLine();
        ImGui::SetNextItemWidth(75.f);
        ImGui::InputInt("##iqty", &m_newItemQty);
        m_newItemQty = std::max(1, m_newItemQty);
        ImGui::SameLine();
        if (ImGui::SmallButton("Add Item"))
        {
            Item it;
            it.name     = m_newItemName;
            it.quantity = m_newItemQty;
            m_containerItems.push_back(it);
        }

        if (!m_containerItems.empty())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear All"))
                m_containerItems.clear();
        }

        ImGui::Checkbox("Clear list after placing", &m_clearItemsAfterPlace);
    }

    if (m_spawnType == 5) // pickup
    {
        ImGui::InputText("Item name", m_pickupName, sizeof(m_pickupName));
    }

    // Snap to player position helper
    if (ImGui::Button("Use Player Pos"))
    {
        const glm::vec3& pp = player.GetPosition();
        const glm::vec3& fwd = player.GetCamera().GetForward();
        glm::vec3 target = pp + fwd * 3.f;
        m_spawnPos[0] = target.x;
        m_spawnPos[1] = 0.5f;     // sensible default Y
        m_spawnPos[2] = target.z;
    }
    ImGui::SameLine();
    if (ImGui::Button("Place"))
        SpawnCurrent(engine);

    // ── Entity list ───────────────────────────────────────────
    ImGui::SeparatorText("Scene Entities");
    ImGui::Text("%d active", (int)world.GetAllRecords().size());

    ImGui::BeginChild("LEEntityList", ImVec2(0, 200), true);
    for (auto& rec : world.GetAllRecords())
    {
        if (!rec.active) continue;

        char label[80];
        std::snprintf(label, sizeof(label), "[%u] %s", rec.id, rec.name.c_str());

        ImGui::Text("%s", label);
        ImGui::SameLine();

        ImGui::PushID((int)rec.id);

        // Inline position display / edit
        if (rec.transform)
        {
            char posLabel[32];
            std::snprintf(posLabel, sizeof(posLabel), "(%.1f %.1f %.1f)",
                          rec.transform->position.x,
                          rec.transform->position.y,
                          rec.transform->position.z);
            ImGui::TextDisabled("%s", posLabel);
            ImGui::SameLine();
        }

        if (ImGui::SmallButton("Del"))
            world.DestroyEntity(rec.id);

        ImGui::PopID();
    }
    ImGui::EndChild();

    // ── Quick inline transform editor for hovered entity ──────
    ImGui::SeparatorText("Quick Edit");
    ImGui::TextDisabled("Select an entity in the World tab to edit it");
}
