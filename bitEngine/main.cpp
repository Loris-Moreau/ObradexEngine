// ============================================================
//  main.cpp  —  ObradexEngine Entry Point
// ============================================================
//  Configures the engine, calls Init(), then hands control
//  to the game loop via Run(). All cleanup is automatic
//  (RAII — smart pointers, GLFW terminate in Window dtor).
//
//  To start a new project:
//    1. Edit EngineConfig below (resolution, title, FPS cap).
//    2. Add your own level-loading code in World::LoadTestLevel()
//       or replace it with a proper JSON/binary loader.
//    3. Extend Player::Update() or add new systems as needed.
// ============================================================

#include "engine/Engine.h"
#include <iostream>

int main()
{
    // ── Engine configuration ──────────────────────────────────
    EngineConfig config;
    config.windowTitle  = "Obradex — Pre-Alpha";
    config.windowWidth  = 1280;
    config.windowHeight = 720;
    config.fullscreen   = false;
    config.vsync        = true;
    config.targetFPS    = 60;

    // Internal render resolution — the low-res pixel-art buffer.
    // 320×180 gives exactly 4:1 pixel ratio on a 1280×720 window.
    // Increase to 640×360 for a crisper look with less pixelation.
    config.renderWidth  = 320;
    config.renderHeight = 180;

    // ── Bootstrap ─────────────────────────────────────────────
    Engine& engine = Engine::Get();

    if (!engine.Init(config))
    {
        std::cerr << "[main] Engine initialisation failed. Exiting.\n";
        return 1;
    }

    // Capture the mouse immediately (FPS mode)
    engine.GetWindow().SetCursorLocked(true);

    // ── Game loop (blocking until window is closed) ────────────
    engine.Run();

    return 0;
}
