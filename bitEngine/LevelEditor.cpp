// LevelEditor.cpp - Runtime level editor and save/load.

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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#pragma comment(lib, "comdlg32.lib")
#endif

namespace fs = std::filesystem;
static constexpr const char* kLevelsDir = "Levels";

// Resolve filename to Levels/<basename>, stripping any directory the user typed.
static std::string LevelPath(const char* filename)
{
    return (fs::path(kLevelsDir) / fs::path(filename).filename()).string();
}

// Compact helpers for writing key-value pairs to the .lvl file.
static void Wf3(std::ofstream& f, const char* k, float x, float y, float z)
{ f << "  " << k << " " << x << " " << y << " " << z << "\n"; }
static void Wf1(std::ofstream& f, const char* k, float v)
{ f << "  " << k << " " << v << "\n"; }
static void Wi1(std::ofstream& f, const char* k, int v)
{ f << "  " << k << " " << v << "\n"; }
static void Ws(std::ofstream& f,  const char* k, const std::string& v)
{ f << "  " << k << " " << v << "\n"; }

bool LevelEditor::SaveLevel(const World& world, const std::string& path)
{
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

        // LampostLight and AlarmLight are child entities created by their parent
        // factory on load. Saving them separately would duplicate them at load time.
        bool isFactoryChild =
            (rec.name == "LampostLight" || rec.name == "AlarmLight") && !rec.mesh;
        if (isFactoryChild) continue;

        std::string type = "cube";
        if (!rec.mesh && rec.light)
            type = "light";
        else if (rec.name == "Ground" ||
                 (rec.mesh && rec.mesh->mesh == const_cast<World&>(world).GetPlaneMesh()))
            type = "plane";
        else if (rec.mesh && rec.mesh->mesh == const_cast<World&>(world).GetSphereMesh())
            type = "sphere";
        else if (rec.name.find("Lamppost") != std::string::npos ||
                 rec.name.find("LampPost") != std::string::npos)
            type = "lamppost";
        else if (rec.name.find("Door")      != std::string::npos) type = "door";
        else if (rec.name.find("Container") != std::string::npos) type = "container";
        else if (rec.name.find("Pickup")    != std::string::npos) type = "pickup";
        else if (rec.name.find("Alarm")     != std::string::npos) type = "alarm";

        file << "ENTITY\n";
        Ws(file, "TYPE", type);
        Ws(file, "NAME", rec.name);

        auto& t = *rec.transform;
        // Lamppost transform is stored at the post centre (base + halfHeight).
        // SpawnLamppost takes the foot position, so subtract halfHeight on save
        // so the round-trip is lossless.
        float saveY = (type == "lamppost") ? t.position.y - t.scale.y * 0.5f
                                           : t.position.y;
        Wf3(file, "POS",   t.position.x, saveY, t.position.z);
        Wf3(file, "SCALE", t.scale.x, t.scale.y, t.scale.z);

        if (rec.mesh)
        {
            Wf3(file, "ALBEDO",
                rec.mesh->albedoColour.x,
                rec.mesh->albedoColour.y,
                rec.mesh->albedoColour.z);
            Wf1(file, "SPECULAR",  rec.mesh->specular);
            Wf1(file, "ROUGHNESS", rec.mesh->roughness);
        }

        if (rec.collision)
        {
            Wf3(file, "COLLISION",
                rec.collision->halfExtents.x,
                rec.collision->halfExtents.y,
                rec.collision->halfExtents.z);
            if (rec.collision->slippery)
                Wi1(file, "SLIPPERY", 1);
        }

        if (rec.light)
        {
            Wf3(file, "LIGHT_COLOR",
                rec.light->colour.x, rec.light->colour.y, rec.light->colour.z);
            Wf1(file, "LIGHT_RADIUS",    rec.light->radius);
            Wf1(file, "LIGHT_INTENSITY", rec.light->intensity);
            Wi1(file, "LIGHT_FLICKER",   rec.light->flicker ? 1 : 0);
        }

        if (rec.container)
        {
            // Spaces in item names are stored as underscores so the parser
            // can split on whitespace. Restored on load.
            for (const auto& item : rec.container->items)
            {
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

    struct EData
    {
        std::string type, name;
        glm::vec3   pos{0,0,0}, scale{1,1,1};
        glm::vec3   albedo{0.7f,0.7f,0.7f};
        float       specular = 0.2f, roughness = 0.8f;
        bool        hasCollision  = false;
        glm::vec3   collHalf{0.5f,0.5f,0.5f};
        bool        collSlippery  = false;
        bool        hasLight = false;
        glm::vec3   lightColor{1.f,0.85f,0.5f};
        float       lightRadius = 10.f, lightIntensity = 1.5f;
        bool        lightFlicker = true;
        std::vector<Item> items;
    };

    bool inEntity  = false;
    EData cur;
    int entityCount = 0;

    auto flush = [&]()
    {
        if (cur.type == "lamppost")
        {
            Interaction::SpawnLamppost(world, cur.pos,
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
            // Entity name is "Pickup_ItemName"; strip prefix and restore spaces.
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
            EntityID e = world.CreateEntity(cur.name);
            auto* t = world.AddTransform(e);
            t->position = cur.pos;
            t->scale    = cur.scale;

            if (cur.type != "light")
            {
                auto* m = world.AddMesh(e);
                if      (cur.type == "plane")  m->mesh = world.GetPlaneMesh();
                else if (cur.type == "sphere") m->mesh = world.GetSphereMesh();
                else                           m->mesh = world.GetCubeMesh();
                m->albedoColour = cur.albedo;
                m->specular     = cur.specular;
                m->roughness    = cur.roughness;

                if (cur.hasCollision)
                {
                    auto* col = world.AddCollision(e);
                    col->halfExtents = cur.collHalf;
                    col->slippery    = cur.collSlippery;
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

    std::string line;
    while (std::getline(file, line))
    {
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
            if      (token == "TYPE")     ss >> cur.type;
            else if (token == "NAME")     ss >> cur.name;
            else if (token == "POS")      ss >> cur.pos.x   >> cur.pos.y   >> cur.pos.z;
            else if (token == "SCALE")    ss >> cur.scale.x >> cur.scale.y >> cur.scale.z;
            else if (token == "ALBEDO")   ss >> cur.albedo.x >> cur.albedo.y >> cur.albedo.z;
            else if (token == "SPECULAR") ss >> cur.specular;
            else if (token == "ROUGHNESS")ss >> cur.roughness;
            else if (token == "COLLISION")
            {
                cur.hasCollision = true;
                ss >> cur.collHalf.x >> cur.collHalf.y >> cur.collHalf.z;
            }
            else if (token == "SLIPPERY")
            {
                int v = 0; ss >> v;
                cur.collSlippery = (v != 0);
            }
            else if (token == "LIGHT_COLOR")
            {
                cur.hasLight = true;
                ss >> cur.lightColor.x >> cur.lightColor.y >> cur.lightColor.z;
            }
            else if (token == "LIGHT_RADIUS")    { cur.hasLight=true; ss>>cur.lightRadius;    }
            else if (token == "LIGHT_INTENSITY") { cur.hasLight=true; ss>>cur.lightIntensity; }
            else if (token == "LIGHT_FLICKER")   { int v=0; ss>>v; cur.lightFlicker=(v!=0); cur.hasLight=true; }
            else if (token == "ITEM")
            {
                Item it;
                ss >> it.name >> it.quantity;
                for (char& c : it.name) if (c == '_') c = ' ';
                cur.items.push_back(it);
            }
        }
    }

    m_statusMsg   = "Loaded: " + fullPath + " (" + std::to_string(entityCount) + " entities)";
    m_statusTimer = 3.f;
    std::cout << "[LevelEditor] Loaded " << entityCount
              << " entities from " << fullPath << "\n";
    return true;
}

void LevelEditor::SpawnCurrent(Engine& engine)
{
    World& world = engine.GetWorld();
    glm::vec3 pos   (m_spawnPos[0],   m_spawnPos[1],   m_spawnPos[2]);
    glm::vec3 scale (m_spawnScale[0], m_spawnScale[1], m_spawnScale[2]);
    glm::vec3 color (m_spawnColor[0], m_spawnColor[1], m_spawnColor[2]);
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
            if (m_spawnCollision)
            {
                auto* col = world.AddCollision(e);
                col->halfExtents = {0.5f,0.5f,0.5f};
                col->slippery    = m_spawnSlippery;
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
        case 2: // Sphere
        {
            EntityID e = world.CreateEntity("Sphere");
            auto* t = world.AddTransform(e); t->position=pos; t->scale=scale;
            auto* m = world.AddMesh(e);
            m->mesh=world.GetSphereMesh(); m->albedoColour=color;
            m->specular=m_spawnSpecular; m->roughness=m_spawnRoughness;
            if (m_spawnCollision)
            {
                auto* col = world.AddCollision(e);
                col->halfExtents = {0.5f,0.5f,0.5f};
                col->slippery    = m_spawnSlippery;
            }
            break;
        }
        case 3: Interaction::SpawnLamppost(world, pos, lcolor, m_lightRadius, m_lightIntensity, m_lightFlicker); break;
        case 4: Interaction::SpawnDoor    (world, pos); break;
        case 5:
            Interaction::SpawnContainer(world, pos, m_containerItems);
            if (m_clearItemsAfterPlace) m_containerItems.clear();
            break;
        case 6:
        {
            Item it; it.name=m_pickupName; it.quantity=1;
            Interaction::SpawnPickup(world, pos, it);
            break;
        }
        case 7: Interaction::SpawnAlarm(world, pos); break;
        case 8:
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

void LevelEditor::RenderPanel(Engine& engine)
{
    World&  world  = engine.GetWorld();
    Player& player = engine.GetPlayer();

    if (m_statusTimer > 0.f)
    {
        m_statusTimer -= ImGui::GetIO().DeltaTime;
        bool isErr = m_statusMsg.rfind("ERROR", 0) == 0;
        ImGui::TextColored(isErr ? ImVec4(1,0.3f,0.3f,1) : ImVec4(0.4f,1,0.4f,1),
                           "%s", m_statusMsg.c_str());
    }

    ImGui::SeparatorText("Level File  [Levels/ folder]");
    ImGui::TextDisabled("Files are stored in:  Levels/<filename>");
    ImGui::SetNextItemWidth(160.f);
    ImGui::InputText("##filename", m_filenameBuffer, sizeof(m_filenameBuffer));
    ImGui::SameLine();

    // Browse button opens the OS native file picker filtered to *.lvl.
    // On Windows: GetOpenFileNameA. On Linux/macOS: zenity (if installed).
    // Either way, only the basename is stored; LevelPath() enforces Levels/.
    if (ImGui::Button("Browse"))
    {
#ifdef _WIN32
        char path[MAX_PATH] = {};
        OPENFILENAMEA ofn = {};
        ofn.lStructSize  = sizeof(ofn);
        ofn.lpstrFilter  = "Level Files (*.lvl)\0*.lvl\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile    = path;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrInitialDir = kLevelsDir;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameA(&ofn))
        {
            std::string name = fs::path(path).filename().string();
            strncpy_s(m_filenameBuffer, sizeof(m_filenameBuffer),
                      name.c_str(), sizeof(m_filenameBuffer) - 1);
        }
#else
        FILE* f = popen("zenity --file-selection --file-filter='Level files | *.lvl' "
                        "--filename=Levels/ 2>/dev/null", "r");
        if (f)
        {
            char buf[512] = {};
            if (fgets(buf, sizeof(buf), f))
            {
                size_t len = strlen(buf);
                if (len && buf[len-1] == '\n') buf[len-1] = '\0';
                std::string name = fs::path(buf).filename().string();
                strncpy_s(m_filenameBuffer, sizeof(m_filenameBuffer),
                          name.c_str(), sizeof(m_filenameBuffer) - 1);
            }
            pclose(f);
        }
#endif
    }
    ImGui::SameLine();

    if (ImGui::Button("Save")) SaveLevel(world, m_filenameBuffer);
    ImGui::SameLine();
    if (ImGui::Button("Load")) LoadLevel(world, m_filenameBuffer);
    ImGui::SameLine();
    if (ImGui::Button("New"))
    {
        world.ClearLevel();
        m_statusMsg   = "Level cleared.";
        m_statusTimer = 2.f;
    }

    ImGui::SeparatorText("Place Entity");
    ImGui::Combo("Type", &m_spawnType, kSpawnTypeNames, kSpawnTypeCount);
    ImGui::DragFloat3("Position##sp", m_spawnPos, 0.1f);

    if (m_spawnType == 0 || m_spawnType == 1 || m_spawnType == 2)
    {
        ImGui::DragFloat3("Scale##sp",      m_spawnScale,       0.05f, 0.01f, 100.f);
        ImGui::ColorEdit3("Colour##sp",     m_spawnColor);
        ImGui::SliderFloat("Roughness##sp", &m_spawnRoughness,  0.f, 1.f);
        ImGui::SliderFloat("Specular##sp",  &m_spawnSpecular,   0.f, 1.f);
        if (m_spawnType == 0 || m_spawnType == 2)
        {
            ImGui::Checkbox("Solid collision", &m_spawnCollision);
            if (m_spawnCollision)
            {
                ImGui::SameLine();
                ImGui::Checkbox("Slippery", &m_spawnSlippery);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Allows the player to slide off on landing (off = floor-like default)");
            }
        }
    }
    if (m_spawnType == 3 || m_spawnType == 8)
    {
        ImGui::ColorEdit3("Light colour",  m_lightColor);
        ImGui::SliderFloat("Radius",    &m_lightRadius,    0.5f, 40.f);
        ImGui::SliderFloat("Intensity", &m_lightIntensity, 0.f,   5.f);
        ImGui::Checkbox   ("Flicker",   &m_lightFlicker);
    }
    if (m_spawnType == 5)
    {
        ImGui::SeparatorText("Container Contents");

        if (m_containerItems.empty())
            ImGui::TextDisabled("(empty)");

        int removeIdx = -1;
        for (int i = 0; i < (int)m_containerItems.size(); ++i)
        {
            ImGui::PushID(i);
            ImGui::Text("%-20s x%d", m_containerItems[i].name.c_str(),
                                     m_containerItems[i].quantity);
            ImGui::SameLine();
            if (ImGui::SmallButton("x")) removeIdx = i;
            ImGui::PopID();
        }
        if (removeIdx >= 0)
            m_containerItems.erase(m_containerItems.begin() + removeIdx);

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
            Item it; it.name=m_newItemName; it.quantity=m_newItemQty;
            m_containerItems.push_back(it);
        }
        if (!m_containerItems.empty())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear All")) m_containerItems.clear();
        }
        ImGui::Checkbox("Clear list after placing", &m_clearItemsAfterPlace);
    }
    if (m_spawnType == 6)
        ImGui::InputText("Item name", m_pickupName, sizeof(m_pickupName));

    if (ImGui::Button("Use Player Pos"))
    {
        const glm::vec3& pp  = player.GetPosition();
        const glm::vec3& fwd = player.GetCamera().GetForward();
        glm::vec3 target = pp + fwd * 3.f;
        m_spawnPos[0] = target.x;
        m_spawnPos[1] = 0.5f;
        m_spawnPos[2] = target.z;
    }
    ImGui::SameLine();
    if (ImGui::Button("Place")) SpawnCurrent(engine);

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
        if (ImGui::SmallButton("Del")) world.DestroyEntity(rec.id);
        ImGui::PopID();
    }
    ImGui::EndChild();
}
