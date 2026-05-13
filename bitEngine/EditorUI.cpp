// ============================================================
//  EditorUI.cpp  —  ImGui Editor / Debug Overlay
// ============================================================

#include "EditorUI.h"
#include "Engine.h"
#include "Timer.h"
#include "Window.h"
#include "Renderer.h"
#include "PostProcess.h"
#include "World.h"
#include "Player.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <cstdio>

// ── Init ──────────────────────────────────────────────────────
void EditorUI::Init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // ── Dark theme tuned for the Obra Dinn aesthetic ──────────
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // Palette: near-black background, amber accent
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
    style.WindowRounding  = 4.f;
    style.FrameRounding   = 2.f;
    style.GrabRounding    = 2.f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

// ── Destructor ────────────────────────────────────────────────
EditorUI::~EditorUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ── Render ────────────────────────────────────────────────────
void EditorUI::Render(Engine& engine)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ── Always-on HUD (interact prompt, crosshair) ────────────
    DrawHUD(engine);

    // ── Editor panel (F1 toggle) ──────────────────────────────
    if (m_showEditor)
    {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(340, 520), ImGuiCond_Once);

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
                if (ImGui::BeginTabItem("World"))
                {
                    DrawWorldPanel(engine);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Player"))
                {
                    DrawPlayerPanel(engine);
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

// ── DrawHUD ───────────────────────────────────────────────────
void EditorUI::DrawHUD(Engine& engine)
{
    ImGuiIO& io      = ImGui::GetIO();
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    float cx = io.DisplaySize.x * 0.5f;
    float cy = io.DisplaySize.y * 0.5f;

    // ── Crosshair ─────────────────────────────────────────────
    const float kCrossSize = 8.f;
    const float kCrossGap  = 3.f;
    ImU32 crossCol = IM_COL32(220, 210, 180, 200);

    draw->AddLine(ImVec2(cx - kCrossSize, cy), ImVec2(cx - kCrossGap, cy), crossCol, 1.5f);
    draw->AddLine(ImVec2(cx + kCrossGap,  cy), ImVec2(cx + kCrossSize, cy), crossCol, 1.5f);
    draw->AddLine(ImVec2(cx, cy - kCrossSize), ImVec2(cx, cy - kCrossGap), crossCol, 1.5f);
    draw->AddLine(ImVec2(cx, cy + kCrossGap),  ImVec2(cx, cy + kCrossSize), crossCol, 1.5f);

    // ── Interact prompt ───────────────────────────────────────
    const std::string& prompt = engine.GetPlayer().GetInteractPrompt();
    if (!prompt.empty())
    {
        // Semi-transparent amber badge below the crosshair
        ImVec2 textSize = ImGui::CalcTextSize(prompt.c_str());
        float  px = cx - textSize.x * 0.5f;
        float  py = cy + 30.f;

        draw->AddRectFilled(ImVec2(px - 8, py - 4),
                            ImVec2(px + textSize.x + 8, py + textSize.y + 4),
                            IM_COL32(20, 15, 5, 200), 3.f);
        draw->AddText(ImVec2(px, py), IM_COL32(220, 180, 80, 255), prompt.c_str());
    }

    // ── Movement state badge (top-left corner) ─────────────────
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
    if (!m_showEditor)  // Only show state badge when editor is hidden
    {
        draw->AddText(ImVec2(12, 12), IM_COL32(160, 140, 80, 160), stateStr);
    }
}

// ── DrawPerformancePanel ──────────────────────────────────────
void EditorUI::DrawPerformancePanel(Engine& engine)
{
    const Timer& timer = engine.GetTimer();

    ImGui::SeparatorText("Frame");
    ImGui::Text("FPS        : %.1f", timer.GetFPS());
    ImGui::Text("Frame time : %.2f ms", timer.GetDeltaTime() * 1000.f);
    ImGui::Text("Total time : %.1f s",  timer.GetTotalTime());
    ImGui::Text("Frame #    : %d",      timer.GetFrameCount());

    ImGui::SeparatorText("Resolution");
    Renderer& r = engine.GetRenderer();
    ImGui::Text("Render : %d x %d", r.RenderWidth(), r.RenderHeight());
    ImGui::Text("Window : %d x %d",
                engine.GetWindow().GetWidth(),
                engine.GetWindow().GetHeight());

    // Simple FPS sparkline using a ring buffer
    static float  fpsHistory[90] = {};
    static int    fpsIdx = 0;
    fpsHistory[fpsIdx] = timer.GetFPS();
    fpsIdx = (fpsIdx + 1) % 90;

    char overlay[32];
    std::snprintf(overlay, sizeof(overlay), "%.0f fps", timer.GetFPS());
    ImGui::PlotLines("##fps", fpsHistory, 90, fpsIdx,
                     overlay, 0.f, 200.f, ImVec2(0, 50));
}

// ── DrawRendererPanel ─────────────────────────────────────────
void EditorUI::DrawRendererPanel(Engine& engine)
{
    PostProcessSettings& pp = engine.GetRenderer().GetPostProcess().Settings();

    ImGui::SeparatorText("Dithering");
    ImGui::SliderFloat("Strength##dith",  &pp.ditherStrength, 0.f, 2.f);
    ImGui::SliderInt  ("Palette size",    &pp.paletteSize,    2,   32);

    ImGui::SeparatorText("Vignette");
    ImGui::SliderFloat("Radius##vig",  &pp.vignetteRadius,  0.f, 1.5f);
    ImGui::SliderFloat("Feather##vig", &pp.vignetteFeather, 0.f, 1.f);

    ImGui::SeparatorText("Scanlines");
    ImGui::Checkbox  ("Enable##scan",  &pp.scanlines);
    if (pp.scanlines)
        ImGui::SliderFloat("Alpha##scan", &pp.scanlineAlpha, 0.f, 0.5f);
    
    ImGui::SeparatorText("Color Grading");
    ImGui::SliderFloat("Contrast",   &pp.contrast,    0.5f, 3.f);
    ImGui::SliderFloat("Brightness", &pp.brightness, -0.5f, 0.5f);

    ImGui::SeparatorText("Aesthetic");
    ImGui::Checkbox("Obra Dinn Mode (Monochrome)", &pp.obraDinnMode);
    
    ImGui::SeparatorText("Presets");
    if (ImGui::Button("Obra Dinn"))
    {
        pp.ditherStrength  = 1.45f;
        pp.paletteSize     = 6;
        pp.vignetteRadius  = 0.885f;
        pp.vignetteFeather = 0.325f;
        pp.scanlines       = false;
        pp.scanlineAlpha   = 0.12f;
        pp.contrast        = 1.75f;
        pp.brightness      = 0.32f;
        pp.obraDinnMode    = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("8-bit"))
    {
        pp.ditherStrength  = 1.0f;
        pp.paletteSize     = 16;
        pp.vignetteRadius  = 0.9f;
        pp.vignetteFeather = 0.575f;
        pp.scanlines       = true;
        pp.scanlineAlpha   = 0.09f;
        pp.contrast        = 1.1f;
        pp.brightness      = 0.25f;
        pp.obraDinnMode    = false;
    }
    //ImGui::SameLine();
    if (ImGui::Button("Bright Colored"))
    {
        pp.ditherStrength  = 1.815f;
        pp.paletteSize     = 32;
        pp.vignetteRadius  = 1.025f;
        pp.vignetteFeather = 0.25f;
        pp.scanlines       = true;
        pp.scanlineAlpha   = 0.12f;
        pp.scanlineAlpha   = 0.12f;
        pp.contrast        = 2.155f;
        pp.brightness      = 0.5f;
        pp.obraDinnMode    = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Dark Colored"))
    {
        pp.ditherStrength  = 0.275f;
        pp.paletteSize     = 24;
        pp.vignetteRadius  = 1.025f;
        pp.vignetteFeather = 0.25f;
        pp.scanlines       = true;
        pp.scanlineAlpha   = 0.12f;
        pp.contrast        = 2.13f;
        pp.brightness      = 0.45f;
        pp.obraDinnMode    = false;
    }
    //ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {
        pp = PostProcessSettings{};
    }
}

// ── DrawWorldPanel ────────────────────────────────────────────
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

    // ── Component Inspector ───────────────────────────────────
    if (m_selectedEntity >= 0)
    {
        auto* rec = world.GetRecord((EntityID)m_selectedEntity);
        if (rec)
        {
            ImGui::SeparatorText(rec->name.c_str());

            if (rec->transform)
            {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::DragFloat3("Position##t", &rec->transform->position.x, 0.05f);
                    ImGui::DragFloat3("Scale##t",    &rec->transform->scale.x,    0.05f);
                }
            }
            if (rec->mesh)
            {
                if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::ColorEdit3("Albedo",    &rec->mesh->albedoColour.x);
                    ImGui::SliderFloat("Specular", &rec->mesh->specular, 0.f, 1.f);
                    ImGui::SliderFloat("Roughness",&rec->mesh->roughness, 0.f, 1.f);
                }
            }
            if (rec->light)
            {
                if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::ColorEdit3("Colour",    &rec->light->colour.x);
                    ImGui::SliderFloat("Radius",   &rec->light->radius,   0.1f, 30.f);
                    ImGui::SliderFloat("Intensity",&rec->light->intensity, 0.f,  5.f);
                    ImGui::Checkbox   ("Flicker",  &rec->light->flicker);
                }
            }
            if (rec->interactable)
            {
                if (ImGui::CollapsingHeader("Interactable", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("Prompt: %s", rec->interactable->promptText.c_str());
                    ImGui::SliderFloat("Range", &rec->interactable->range, 0.5f, 10.f);
                    ImGui::Checkbox("Enabled", &rec->interactable->enabled);
                }
            }
        }
    }
}

// ── DrawPlayerPanel ───────────────────────────────────────────
void EditorUI::DrawPlayerPanel(Engine& engine)
{
    Player& player = engine.GetPlayer();
    PlayerStats& stats = player.GetStats();
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
    ImGui::Text("State     : %s", stateStr);
    ImGui::Text("Position  : %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Speed     : %.2f m/s", player.GetSpeed());

    ImGui::SeparatorText("Stats");
    ImGui::SliderFloat("Walk speed",   &stats.walkSpeed,        1.0f, 10.0f);
    ImGui::SliderFloat("Sprint speed", &stats.sprintSpeed,      4.0f, 20.0f);
    ImGui::SliderFloat("Crouch speed", &stats.crouchSpeed,      0.5f, 5.0f);
    ImGui::SliderFloat("Slide Speed",  &stats.slideSpeed,       0.25f, 20.0f);
    ImGui::SliderFloat("Jump height",  &stats.jumpHeight,       0.3f, 4.0f);
    ImGui::SliderFloat("Gravity",      &stats.gravity,          0.2f, -20.0f);
    ImGui::SliderFloat("Mouse sens.",  &stats.mouseSensitivity, 0.02f, 0.5f);
    ImGui::SliderFloat("Eye height",   &stats.eyeHeight,        0.5f, 2.5f);
    ImGui::SliderFloat("Crouch height",&stats.crouchHeight,     0.3f, 1.5f);
    ImGui::SliderFloat("Interact range",&stats.interactRange,   0.5f, 6.0f);

    ImGui::SeparatorText("Camera");
    Camera& cam = player.GetCamera();
    float fov = cam.GetFOV();
    if (ImGui::SliderFloat("FOV", &fov, 50.f, 110.f))
        cam.SetFOV(fov);
    ImGui::Text("Yaw / Pitch: %.1f° / %.1f°", cam.GetYaw(), cam.GetPitch());

    ImGui::SeparatorText("Presets");
    if (ImGui::Button("Base"))
    {
        stats.walkSpeed = 4.0f;
        stats.sprintSpeed = 8.0f;
        stats.crouchSpeed = 1.8f;
        stats.slideSpeed = 10.0f;
        stats.jumpHeight = 1.2f;
        stats.gravity = -15.0f;
        stats.mouseSensitivity = 0.12f;
        stats.eyeHeight = 1.75f;
        stats.crouchHeight = 0.85f;
        stats.interactRange = 2.5f;
    }
    ImGui::SameLine();
    if (ImGui::Button("SpeedRun"))
    {
        stats.walkSpeed = 3.0f;
        stats.sprintSpeed = 8.0f;
        stats.crouchSpeed = 1.75f;
        stats.slideSpeed = 15.0f;
        stats.jumpHeight = 1.25f;
        stats.gravity = -12.0f;
        stats.mouseSensitivity = 0.24f;
        stats.eyeHeight = 1.9f;
        stats.crouchHeight = 0.9f;
        stats.interactRange = 2.75f;
    }
    
    if (ImGui::Button("Reset"))
    {
        stats = PlayerStats{};
    }
}
