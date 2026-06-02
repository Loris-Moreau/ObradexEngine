#pragma once

// EditorUI.h - Dear ImGui runtime editor and in-game HUD.
//
// Press F1 to toggle the full editor panel. The HUD (crosshair, interact
// prompt, movement state badge) always renders even when the panel is hidden.
//
// Tabs: Performance, Renderer, Player, World, Level Editor.

#include <memory>
struct GLFWwindow;
class  Engine;
class  LevelEditor;

class EditorUI
{
public:
    // Constructor and destructor are defined in EditorUI.cpp so the compiler
    // sees the complete LevelEditor type when generating unique_ptr<LevelEditor>
    // cleanup. Defaulting them in the header would instantiate the destructor
    // inline with only a forward declaration, which MSVC rejects.
    EditorUI();
    ~EditorUI();

    void Init(GLFWwindow* window);
    void Render(Engine& engine);

    void ToggleEditorPanel() { m_showEditor = !m_showEditor; }
    bool IsEditorVisible()   const { return m_showEditor; }

private:
    void DrawPerformancePanel(Engine& engine);
    void DrawRendererPanel   (Engine& engine);
    void DrawWorldPanel      (Engine& engine);
    void DrawPlayerPanel     (Engine& engine);
    void DrawLevelEditorPanel(Engine& engine);
    void DrawContainerPopup  (Engine& engine);  // 3x3 item grid when a container is open
    void DrawHUD             (Engine& engine);  // Always-on crosshair and interact prompt

    bool m_showEditor       = false;
    int  m_selectedEntity   = -1;
    bool m_wasContainerOpen = false;  // Tracks open state for cursor lock/unlock

    std::unique_ptr<LevelEditor> m_levelEditor;
};
