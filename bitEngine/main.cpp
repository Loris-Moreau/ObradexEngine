// main.cpp - Entry point.
//
// Configure the engine, call Init(), then hand control to Run().
// All cleanup is automatic via RAII (unique_ptr, Window destructor).
//
// To start a new project:
//   1. Adjust EngineConfig below (resolution, title, FPS cap).
//   2. Populate World::LoadTestLevel() with your level content,
//      or replace it with a proper save-file or procedural loader.
//   3. Extend Player::Update() or add new systems to Engine as needed.

#include "Engine.h"
#include "Window.h"
#include <iostream>

int main()
{
    EngineConfig config;
    config.windowTitle  = "Obradex - Pre-Alpha";
    config.windowWidth  = 1280;
    config.windowHeight = 720;
    config.fullscreen   = false;
    config.vsync        = true;
    config.targetFPS    = 60;

    // Internal render resolution. 320x180 gives a 4:1 pixel ratio on a
    // 1280x720 window. Use 640x360 for a crisper look with less dithering.
    config.renderWidth  = 320;
    config.renderHeight = 180;

    Engine& engine = Engine::Get();

    if (!engine.Init(config))
    {
        std::cerr << "[main] Engine init failed.\n";
        return 1;
    }

    engine.GetWindow().SetCursorLocked(true);
    engine.Run();

    return 0;
}
