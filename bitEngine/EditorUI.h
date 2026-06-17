#pragma once

// EditorUI.h - Dear ImGui runtime editor and in-game HUD.
//
// F1 toggles the full editor panel. The HUD (crosshair, interact prompt,
// health bar, movement state badge) always renders during gameplay.
//
// State overlays (main menu, pause, game over, level complete) are drawn
// by DrawStateOverlay() every frame based on Engine::GetState().

#include <memory>
struct GLFWwindow;
class  Engine;
class  LevelEditor;

class EditorUI
{
public:
    EditorUI();
    ~EditorUI();

    void Init   (GLFWwindow* window);
    void Render (Engine& engine);

    void ToggleEditorPanel() { m_showEditor = !m_showEditor; }
    bool IsEditorVisible()   const { return m_showEditor; }

private:
    void DrawPerformancePanel(Engine& engine);
    void DrawRendererPanel   (Engine& engine);
    void DrawWorldPanel      (Engine& engine);
    void DrawPlayerPanel     (Engine& engine);
    void DrawLevelEditorPanel(Engine& engine);
    void DrawContainerPopup  (Engine& engine);
    void DrawHUD             (Engine& engine);
    void DrawStateOverlay    (Engine& engine);

    bool m_showEditor       = false;
    int  m_selectedEntity   = -1;
    bool m_wasContainerOpen = false;
    
    float fogDensity = 0.005f;
    float fogCol[3]  = {0.06f, 0.06f, 0.16f};

    std::unique_ptr<LevelEditor> m_levelEditor;
};
