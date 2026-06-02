// EditorUI.cpp - Dear ImGui runtime editor and HUD.

#include "EditorUI.h"
#include "Engine.h"
#include "Timer.h"
#include "Window.h"
#include "Renderer.h"
#include "PostProcess.h"
#include "World.h"
#include "Player.h"
#include "InventorySystem.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "LevelEditor.h"
#include "Input.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>
#include <iostream>

EditorUI::EditorUI()  = default;
EditorUI::~EditorUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void EditorUI::Init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Dark theme with amber accents matching the engine aesthetic.
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg]       = ImVec4(0.06f, 0.05f, 0.04f, 0.92f);
    style.Colors[ImGuiCol_TitleBg]        = ImVec4(0.10f, 0.08f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]  = ImVec4(0.20f, 0.15f, 0.05f, 1.00f);
    style.Colors[ImGuiCol_Header]         = ImVec4(0.25f, 0.18f, 0.05f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered]  = ImVec4(0.40f, 0.28f, 0.08f, 0.90f);
    style.Colors[ImGuiCol_Button]         = ImVec4(0.22f, 0.16f, 0.04f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]  = ImVec4(0.38f, 0.26f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.12f, 0.10f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]     = ImVec4(0.60f, 0.42f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]      = ImVec4(0.80f, 0.60f, 0.18f, 1.00f);
    style.WindowRounding = 4.f;
    style.FrameRounding  = 2.f;
    style.GrabRounding   = 2.f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    m_levelEditor = std::make_unique<LevelEditor>();
}

void EditorUI::Render(Engine& engine)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Unlock/lock cursor when a container popup opens or closes.
    bool containerNowOpen = engine.GetWorld().HasOpenContainer();
    if (containerNowOpen != m_wasContainerOpen)
    {
        engine.GetWindow().SetCursorLocked(!containerNowOpen);
        m_wasContainerOpen = containerNowOpen;
    }

    DrawContainerPopup(engine);
    DrawHUD(engine);

    // Inventory overlay driven by InventorySystem::DrawUI.
    ImGuiIO& io = ImGui::GetIO();
    engine.GetInventory().DrawUI(io.DisplaySize.x, io.DisplaySize.y);

    if (m_showEditor)
    {
        ImGui::SetNextWindowPos (ImVec2(10,  10),       ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(340, 520),      ImGuiCond_Once);

        if (ImGui::Begin("Obradex Engine  [F1]", &m_showEditor,
                         ImGuiWindowFlags_NoSavedSettings))
        {
            if (ImGui::BeginTabBar("EditorTabs"))
            {
                if (ImGui::BeginTabItem("Perf"))
                {
                    DrawPerformancePanel(engine);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Renderer"))
                {
                    DrawRendererPanel(engine);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Player"))
                {
                    DrawPlayerPanel(engine);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("World"))
                {
                    DrawWorldPanel(engine);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Level Editor"))
                {
                    DrawLevelEditorPanel(engine);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void EditorUI::DrawHUD(Engine& engine)
{
    ImGuiIO& io      = ImGui::GetIO();
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    float cx = io.DisplaySize.x * 0.5f;
    float cy = io.DisplaySize.y * 0.5f;

    // Crosshair
    const float kCrossSize = 8.f;
    const float kCrossGap  = 3.f;
    ImU32 crossCol = IM_COL32(220, 210, 180, 200);
    draw->AddLine(ImVec2(cx - kCrossSize, cy), ImVec2(cx - kCrossGap,  cy), crossCol, 1.5f);
    draw->AddLine(ImVec2(cx + kCrossGap,  cy), ImVec2(cx + kCrossSize, cy), crossCol, 1.5f);
    draw->AddLine(ImVec2(cx, cy - kCrossSize), ImVec2(cx, cy - kCrossGap),  crossCol, 1.5f);
    draw->AddLine(ImVec2(cx, cy + kCrossGap),  ImVec2(cx, cy + kCrossSize), crossCol, 1.5f);

    // Interact prompt: amber badge centred below the crosshair.
    const std::string& prompt = engine.GetPlayer().GetInteractPrompt();
    if (!prompt.empty())
    {
        ImVec2 textSize = ImGui::CalcTextSize(prompt.c_str());
        float  px = cx - textSize.x * 0.5f;
        float  py = cy + 30.f;
        draw->AddRectFilled(ImVec2(px - 8, py - 4),
                            ImVec2(px + textSize.x + 8, py + textSize.y + 4),
                            IM_COL32(20, 15, 5, 200), 3.f);
        draw->AddText(ImVec2(px, py), IM_COL32(220, 180, 80, 255), prompt.c_str());
    }

    // Movement state badge in the top-left corner; hidden when the editor is open.
    if (!m_showEditor)
    {
        const char* stateStr = "STANDING";
        switch (engine.GetPlayer().GetMoveState())
        {
            case MoveState::Crouching: stateStr = "CROUCHING"; break;
            case MoveState::Sprinting: stateStr = "SPRINTING"; break;
            case MoveState::Sliding:   stateStr = "SLIDING";   break;
            case MoveState::InAir:     stateStr = "IN AIR";    break;
            case MoveState::Vaulting:  stateStr = "VAULTING";  break;
            default: break;
        }
        draw->AddText(ImVec2(12, 12), IM_COL32(160, 140, 80, 160), stateStr);
    }
}

void EditorUI::DrawPerformancePanel(Engine& engine)
{
    const Timer& timer = engine.GetTimer();

    ImGui::SeparatorText("Frame");
    ImGui::Text("FPS        : %.1f",    timer.GetFPS());
    ImGui::Text("Frame time : %.2f ms", timer.GetDeltaTime() * 1000.f);
    ImGui::Text("Total time : %.1f s",  timer.GetTotalTime());
    ImGui::Text("Frame #    : %d",      timer.GetFrameCount());

    ImGui::SeparatorText("Resolution");
    Renderer& r = engine.GetRenderer();
    ImGui::Text("Render : %d x %d", r.RenderWidth(), r.RenderHeight());
    ImGui::Text("Window : %d x %d",
                engine.GetWindow().GetWidth(),
                engine.GetWindow().GetHeight());

    // Rolling FPS sparkline using a fixed-size ring buffer.
    static float fpsHistory[90] = {};
    static int   fpsIdx = 0;
    fpsHistory[fpsIdx] = timer.GetFPS();
    fpsIdx = (fpsIdx + 1) % 90;

    char overlay[32];
    std::snprintf(overlay, sizeof(overlay), "%.0f fps", timer.GetFPS());
    ImGui::PlotLines("##fps", fpsHistory, 90, fpsIdx,
                     overlay, 0.f, 200.f, ImVec2(0, 50));
}

void EditorUI::DrawRendererPanel(Engine& engine)
{
    PostProcessSettings& pp = engine.GetRenderer().GetPostProcess().Settings();

    ImGui::SeparatorText("Dithering");
    ImGui::SliderFloat("Strength##dith", &pp.ditherStrength, 0.f, 2.f);
    ImGui::SliderInt  ("Palette size",   &pp.paletteSize,    2,   32);

    ImGui::SeparatorText("Vignette");
    ImGui::SliderFloat("Radius##vig",  &pp.vignetteRadius,  0.f, 1.5f);
    ImGui::SliderFloat("Feather##vig", &pp.vignetteFeather, 0.f, 1.f);

    ImGui::SeparatorText("Scanlines");
    ImGui::Checkbox("Enable##scan", &pp.scanlines);
    if (pp.scanlines)
        ImGui::SliderFloat("Alpha##scan", &pp.scanlineAlpha, 0.f, 0.5f);

    ImGui::SeparatorText("Color Grading");
    ImGui::SliderFloat("Contrast",   &pp.contrast,    0.5f,  3.f);
    ImGui::SliderFloat("Brightness", &pp.brightness, -0.5f,  0.5f);

    ImGui::SeparatorText("Aesthetic");
    ImGui::Checkbox("Obra Dinn Mode (Monochrome)", &pp.obraDinnMode);

    ImGui::SeparatorText("Presets");
    if (ImGui::Button("Obra Dinn"))
    {
        pp.ditherStrength  = 1.45f;  pp.paletteSize     = 6;
        pp.vignetteRadius  = 0.885f; pp.vignetteFeather = 0.325f;
        pp.scanlines       = false;  pp.scanlineAlpha   = 0.12f;
        pp.contrast        = 1.75f;  pp.brightness      = 0.32f;
        pp.obraDinnMode    = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("8-bit"))
    {
        pp.ditherStrength  = 1.0f;   pp.paletteSize     = 16;
        pp.vignetteRadius  = 0.9f;   pp.vignetteFeather = 0.575f;
        pp.scanlines       = true;   pp.scanlineAlpha   = 0.09f;
        pp.contrast        = 1.1f;   pp.brightness      = 0.25f;
        pp.obraDinnMode    = false;
    }
    if (ImGui::Button("Bright Colored"))
    {
        pp.ditherStrength  = 1.815f; pp.paletteSize     = 32;
        pp.vignetteRadius  = 1.025f; pp.vignetteFeather = 0.25f;
        pp.scanlines       = true;   pp.scanlineAlpha   = 0.12f;
        pp.contrast        = 2.155f; pp.brightness      = 0.5f;
        pp.obraDinnMode    = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Dark Colored"))
    {
        pp.ditherStrength  = 0.275f; pp.paletteSize     = 24;
        pp.vignetteRadius  = 1.025f; pp.vignetteFeather = 0.25f;
        pp.scanlines       = true;   pp.scanlineAlpha   = 0.12f;
        pp.contrast        = 2.13f;  pp.brightness      = 0.45f;
        pp.obraDinnMode    = false;
    }
    if (ImGui::Button("Reset"))
        pp = PostProcessSettings{};
}

void EditorUI::DrawPlayerPanel(Engine& engine)
{
    Player&      player = engine.GetPlayer();
    PlayerStats& stats  = player.GetStats();
    const glm::vec3& pos = player.GetPosition();

    ImGui::SeparatorText("State");
    const char* stateStr = "Standing";
    switch (player.GetMoveState())
    {
        case MoveState::Crouching: stateStr = "Crouching"; break;
        case MoveState::Sprinting: stateStr = "Sprinting"; break;
        case MoveState::Sliding:   stateStr = "Sliding";   break;
        case MoveState::InAir:     stateStr = "In Air";    break;
        case MoveState::Vaulting:  stateStr = "Vaulting";  break;
        default: break;
    }
    ImGui::Text("State    : %s",              stateStr);
    ImGui::Text("Position : %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Speed    : %.2f m/s",         player.GetSpeed());

    ImGui::SeparatorText("Stats");
    ImGui::SliderFloat("Walk speed",     &stats.walkSpeed,        1.0f,  10.0f);
    ImGui::SliderFloat("Sprint speed",   &stats.sprintSpeed,      4.0f,  20.0f);
    ImGui::SliderFloat("Crouch speed",   &stats.crouchSpeed,      0.5f,   5.0f);
    ImGui::SliderFloat("Slide Speed",    &stats.slideSpeed,       0.25f, 20.0f);
    ImGui::SliderFloat("Jump height",    &stats.jumpHeight,       0.3f,   4.0f);
    ImGui::SliderFloat("Gravity",        &stats.gravity,         -20.0f,  0.2f);
    ImGui::SliderFloat("Mouse sens.",    &stats.mouseSensitivity, 0.02f,  0.5f);
    ImGui::SliderFloat("Eye height",     &stats.eyeHeight,        0.5f,   2.5f);
    ImGui::SliderFloat("Crouch height",  &stats.crouchHeight,     0.3f,   1.5f);
    ImGui::SliderFloat("Interact range", &stats.interactRange,    0.5f,   6.0f);

    ImGui::SeparatorText("Air Movement");
    ImGui::SliderFloat("Air control",      &stats.airControl,         0.0f, 20.0f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("m/s^2 horizontal nudge while airborne");
    ImGui::SliderFloat("Jump vel. retain", &stats.jumpVelocityRetain, 0.0f,  1.0f);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Fraction of horizontal speed kept at jump\n0 = stop, 0.85 = default, 1 = full");

    ImGui::SeparatorText("Camera");
    Camera& cam = player.GetCamera();
    float fov = cam.GetFOV();
    if (ImGui::SliderFloat("FOV", &fov, 50.f, 110.f))
        cam.SetFOV(fov);
    ImGui::Text("Yaw / Pitch: %.1f / %.1f deg", cam.GetYaw(), cam.GetPitch());

    ImGui::SeparatorText("Presets");
    if (ImGui::Button("Base"))
    {
        stats.walkSpeed          = 4.0f;   stats.sprintSpeed       = 8.0f;
        stats.crouchSpeed        = 1.8f;   stats.slideSpeed        = 10.0f;
        stats.jumpHeight         = 1.2f;   stats.gravity           = -15.0f;
        stats.mouseSensitivity   = 0.12f;  stats.eyeHeight         = 1.75f;
        stats.crouchHeight       = 0.85f;  stats.interactRange     = 2.5f;
        stats.airControl         = 4.0f;   stats.jumpVelocityRetain = 0.75f;
    }
    ImGui::SameLine();
    if (ImGui::Button("SpeedRun"))
    {
        stats.walkSpeed          = 3.0f;   stats.sprintSpeed       = 8.0f;
        stats.crouchSpeed        = 1.75f;  stats.slideSpeed        = 15.0f;
        stats.jumpHeight         = 1.25f;  stats.gravity           = -12.0f;
        stats.mouseSensitivity   = 0.24f;  stats.eyeHeight         = 1.9f;
        stats.crouchHeight       = 0.9f;   stats.interactRange     = 2.75f;
        stats.airControl         = 6.0f;   stats.jumpVelocityRetain = 0.95f;
    }
    if (ImGui::Button("Reset"))
        stats = PlayerStats{};
}

void EditorUI::DrawWorldPanel(Engine& engine)
{
    World& world = engine.GetWorld();
    const auto& records = world.GetAllRecords();

    ImGui::SeparatorText("Entities");
    ImGui::Text("%d entities in scene", (int)records.size());

    ImGui::BeginChild("EntityList", ImVec2(0, 160), true);
    for (const auto& rec : records)
    {
        if (!rec.active) continue;
        char label[64];
        std::snprintf(label, sizeof(label), "[%u] %s", rec.id, rec.name.c_str());
        bool selected = (m_selectedEntity == (int)rec.id);
        if (ImGui::Selectable(label, selected))
            m_selectedEntity = (int)rec.id;
    }
    ImGui::EndChild();

    if (m_selectedEntity >= 0)
    {
        auto* rec = world.GetRecord((EntityID)m_selectedEntity);
        if (rec)
        {
            ImGui::SeparatorText(rec->name.c_str());

            if (rec->transform && ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat3("Position##t", &rec->transform->position.x, 0.05f);
                ImGui::DragFloat3("Scale##t",    &rec->transform->scale.x,    0.05f);
            }
            if (rec->mesh && ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit3 ("Albedo",    &rec->mesh->albedoColour.x);
                ImGui::SliderFloat("Specular",  &rec->mesh->specular,  0.f, 1.f);
                ImGui::SliderFloat("Roughness", &rec->mesh->roughness, 0.f, 1.f);
            }
            if (rec->light && ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit3 ("Colour",    &rec->light->colour.x);
                ImGui::SliderFloat("Radius",    &rec->light->radius,    0.1f, 30.f);
                ImGui::SliderFloat("Intensity", &rec->light->intensity, 0.f,   5.f);
                ImGui::Checkbox   ("Flicker",   &rec->light->flicker);
            }
            if (rec->interactable && ImGui::CollapsingHeader("Interactable", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text       ("Prompt: %s", rec->interactable->promptText.c_str());
                ImGui::SliderFloat("Range",      &rec->interactable->range, 0.5f, 10.f);
                ImGui::Checkbox   ("Enabled",    &rec->interactable->enabled);
            }
        }
    }
}

void EditorUI::DrawLevelEditorPanel(Engine& engine)
{
    if (m_levelEditor)
        m_levelEditor->RenderPanel(engine);
}

// 3x3 item grid shown whenever a ContainerComponent has isOpen == true.
// Clicking a slot moves the item to the player's inventory.
// "Grab All" moves every item at once and closes the popup.
void EditorUI::DrawContainerPopup(Engine& engine)
{
    World& world = engine.GetWorld();

    ContainerComponent* container   = nullptr;
    EntityID            containerID = kNullEntity;
    std::string         containerName;

    for (auto& rec : world.GetAllRecords())
    {
        if (!rec.active || !rec.container || !rec.container->isOpen) continue;
        container     = rec.container;
        containerID   = rec.id;
        containerName = rec.name;
        break;
    }
    if (!container) return;

    ImGui::PushStyleColor(ImGuiCol_WindowBg,     ImVec4(0.08f,0.08f,0.08f,0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border,       ImVec4(0.55f,0.40f,0.10f,0.90f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,      ImVec4(0.18f,0.12f,0.03f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,ImVec4(0.28f,0.18f,0.04f,1.00f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(12.f,12.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(6.f,6.f));

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(330.f, 370.f), ImGuiCond_Always);

    char title[64];
    std::snprintf(title, sizeof(title), "Container  -  %s", containerName.c_str());

    bool windowOpen = true;
    ImGui::Begin(title, &windowOpen,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

    auto& items = container->items;

    const float kSlotSize = 82.f;
    const float kSlotPad  =  6.f;
    ImDrawList* dl        = ImGui::GetWindowDrawList();
    ImVec2      winPos    = ImGui::GetWindowPos();

    int grabbedIdx = -1;

    for (int row = 0; row < 3; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            if (col > 0) ImGui::SameLine(0.f, kSlotPad);
            int  idx     = row * 3 + col;
            bool hasItem = (idx < (int)items.size());
            ImGui::PushID(idx);

            ImVec2 slotCursor = ImGui::GetCursorPos();
            ImVec2 abs = ImVec2(winPos.x + slotCursor.x, winPos.y + slotCursor.y);

            ImVec4 slotBg = hasItem ? ImVec4(0.22f,0.16f,0.06f,1.f)
                                    : ImVec4(0.10f,0.10f,0.10f,0.8f);
            ImGui::PushStyleColor(ImGuiCol_Button,       slotBg);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                hasItem ? ImVec4(0.38f,0.25f,0.05f,1.f) : ImVec4(0.15f,0.15f,0.15f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f,0.35f,0.05f,1.f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);

            bool clicked = ImGui::Button("##slot", ImVec2(kSlotSize, kSlotSize));

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);

            if (hasItem)
            {
                const Item& item = items[idx];
                float textW = ImGui::CalcTextSize(item.name.c_str()).x;
                float textX = abs.x + (kSlotSize - std::min(textW, kSlotSize - 8.f)) * 0.5f;
                dl->AddText(ImVec2(textX, abs.y + 10.f),
                            IM_COL32(240,195,80,255), item.name.c_str());

                char qty[16];
                std::snprintf(qty, sizeof(qty), "x%d", item.quantity);
                ImVec2 qSize = ImGui::CalcTextSize(qty);
                dl->AddText(ImVec2(abs.x + kSlotSize - qSize.x - 6.f,
                                   abs.y + kSlotSize - qSize.y - 6.f),
                            IM_COL32(180,180,180,200), qty);

                if (clicked) grabbedIdx = idx;
            }

            dl->AddRect(abs, ImVec2(abs.x + kSlotSize, abs.y + kSlotSize),
                        hasItem ? IM_COL32(140,100,20,180) : IM_COL32(60,60,60,120),
                        4.f, 0, 1.5f);

            ImGui::PopID();
        }
    }

    // Handle grab outside the loop to avoid iterator invalidation.
    if (grabbedIdx >= 0 && grabbedIdx < (int)items.size())
    {
        engine.GetInventory().AddItem(items[grabbedIdx]);
        items.erase(items.begin() + grabbedIdx);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (items.empty())
        ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1.f), "Container is empty.");
    else
        ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1.f), "%d item(s) remaining", (int)items.size());

    ImGui::Spacing();

    float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.30f,0.20f,0.04f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f,0.32f,0.06f,1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.70f,0.45f,0.08f,1.f));

    if (ImGui::Button("Grab All", ImVec2(btnW, 0.f)) && !items.empty())
    {
        for (auto& it : items)
            engine.GetInventory().AddItem(it);
        items.clear();

        auto* rec = world.GetRecord(containerID);
        if (rec && rec->interactable)
            rec->interactable->promptText =
                std::string("[") + Input::GetKeyName(INTERACT_KEY) + "] Search  (empty)";
        windowOpen = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("Close", ImVec2(btnW, 0.f)))
        windowOpen = false;

    // Escape closes the container (Engine::ProcessInput also handles this,
    // but checking here too prevents a one-frame delay on the popup close).
    ImGuiIO& escIO = ImGui::GetIO();
    if (escIO.KeysDown[256] && !escIO.WantCaptureKeyboard)
        windowOpen = false;

    ImGui::PopStyleColor(3);
    ImGui::End();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);

    if (!windowOpen)
        container->isOpen = false;
}
