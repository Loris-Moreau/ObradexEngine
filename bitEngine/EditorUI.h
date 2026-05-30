#pragma once

// ============================================================
//  EditorUI.h  -  ImGui Editor / Debug Overlay
// ============================================================
//  Provides a lightweight runtime editor built with Dear ImGui.
//  Press F1 to toggle the panel in-game.
//
//  Panels available:
//    Performance  - FPS, frame time, render resolution
//    Renderer     - post-process settings (dither, palette, etc.)
//    World        - entity list with component inspector
//    Player       - movement stats, position, eye-height
//    HUD          - interact prompt, crosshair
//
//  The HUD elements (crosshair, interact prompt) always draw,
//  even when the full editor panel is hidden, so the player
//  has feedback during gameplay.
// ============================================================

#include <memory>
struct GLFWwindow;
class  Engine;
class  LevelEditor;

class EditorUI
{
public:
    EditorUI();   ///< Defined in EditorUI.cpp - LevelEditor must be complete
    ~EditorUI();  ///< Defined in EditorUI.cpp - LevelEditor must be complete

    // ── Lifecycle ─────────────────────────────────────────────

    /// Initialise ImGui with the GLFW + OpenGL3 backends.
    void Init(GLFWwindow* window);

    /// Render all ImGui windows + the in-game HUD.
    void Render(Engine& engine);

    /// Show or hide the full editor panel.
    void ToggleEditorPanel() { m_showEditor = !m_showEditor; }
    bool IsEditorVisible()   const { return m_showEditor;    }

private:
    // ── Panel renderers ───────────────────────────────────────
    void DrawPerformancePanel(Engine& engine);
    void DrawRendererPanel   (Engine& engine);
    void DrawWorldPanel      (Engine& engine);
    void DrawPlayerPanel     (Engine& engine);
    void DrawLevelEditorPanel(Engine& engine);
    void DrawContainerPopup  (Engine& engine);   // 3x3 item grid when a container is open
    void DrawHUD             (Engine& engine);   // Always-on

    bool m_showEditor     = false;  ///< F1 toggle
    int  m_selectedEntity = -1;     ///< Entity inspector selection

    std::unique_ptr<LevelEditor> m_levelEditor;  ///< Save/load + placement panel
    bool m_wasContainerOpen = false;             ///< Tracks open state for cursor lock/unlock
};
