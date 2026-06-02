// Engine.cpp - ObradexEngine core implementation.

#include "Engine.h"
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Renderer.h"
#include "World.h"
#include "Player.h"
#include "EditorUI.h"
#include "InventorySystem.h"

#include <iostream>
#include <stdexcept>

Engine& Engine::Get()
{
    static Engine instance;
    return instance;
}

bool Engine::Init(const EngineConfig& config)
{
    if (m_state != EngineState::Uninitialized)
    {
        std::cerr << "[Engine] Init() called more than once.\n";
        return false;
    }

    m_config = config;
    std::cout << "[Engine] Initialising ObradexEngine v0.1.0\n";

    try
    {
        m_timer = std::make_unique<Timer>();
        std::cout << "[Engine]  Timer        OK\n";

        m_window = std::make_unique<Window>();
        if (!m_window->Init(config.windowTitle,
                            config.windowWidth,
                            config.windowHeight,
                            config.fullscreen,
                            config.vsync))
        {
            throw std::runtime_error("Window init failed.");
        }
        std::cout << "[Engine]  Window       OK\n";

        m_input = std::make_unique<Input>();
        m_input->Init(m_window->GetGLFWWindow());
        std::cout << "[Engine]  Input        OK\n";

        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->Init(config.renderWidth, config.renderHeight))
        {
            throw std::runtime_error("Renderer init failed.");
        }
        std::cout << "[Engine]  Renderer     OK\n";

        m_world = std::make_unique<World>();
        m_world->Init();
        std::cout << "[Engine]  World        OK\n";

        m_player = std::make_unique<Player>();
        m_player->Init();
        std::cout << "[Engine]  Player       OK\n";

        m_inventory = std::make_unique<InventorySystem>();
        std::cout << "[Engine]  Inventory    OK\n";

        m_editorUI = std::make_unique<EditorUI>();
        m_editorUI->Init(m_window->GetGLFWWindow());
        std::cout << "[Engine]  EditorUI     OK\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Engine] FATAL: " << e.what() << "\n";
        return false;
    }

    m_state = EngineState::Running;
    std::cout << "[Engine] All systems nominal. Entering game loop.\n";
    return true;
}

void Engine::Run()
{
    if (m_state != EngineState::Running)
    {
        std::cerr << "[Engine] Run() called before Init().\n";
        return;
    }

    m_timer->Reset();

    // Fixed-timestep physics with variable rendering.
    // Decoupling physics from frame rate keeps simulation stable.
    const float fixedDt     = 1.0f / static_cast<float>(m_config.targetFPS);
    float       accumulator = 0.0f;

    while (!m_window->ShouldClose() && m_state != EngineState::Shutdown)
    {
        float frameDt = m_timer->Tick();

        // Cap to 250 ms to prevent the spiral-of-death on very slow frames.
        if (frameDt > 0.25f) frameDt = 0.25f;
        accumulator += frameDt;

        // PollEvents must run before Input::Update so glfwGetKey reads
        // the OS key state queued since the last frame, not stale data.
        m_window->PollEvents();

        ProcessInput();

        while (accumulator >= fixedDt)
        {
            if (m_state == EngineState::Running)
                Update(fixedDt);
            accumulator -= fixedDt;
        }

        Render();
        m_window->SwapBuffers();
    }

    Shutdown();
}

void Engine::ProcessInput()
{
    m_input->Update();

    if (m_input->IsKeyJustPressed(Key::F1))
        m_editorUI->ToggleEditorPanel();

    // I key toggles the inventory. Opening pauses the game so physics stops.
    if (m_input->IsKeyJustPressed(Key::I) && !m_world->HasOpenContainer())
    {
        if (m_inventory->IsOpen())
        {
            m_inventory->Toggle();
            m_window->SetCursorLocked(true);
            if (m_state == EngineState::Paused)
                m_state = EngineState::Running;
        }
        else if (m_state == EngineState::Running)
        {
            m_inventory->Toggle();
            m_window->SetCursorLocked(false);
            m_state = EngineState::Paused;
        }
    }

    if (m_input->IsKeyJustPressed(Key::Escape))
    {
        // Priority order: container > inventory > pause toggle.
        if (m_world && m_world->HasOpenContainer())
        {
            m_world->CloseOpenContainer();
        }
        else if (m_inventory && m_inventory->IsOpen())
        {
            m_inventory->Toggle();
            m_window->SetCursorLocked(true);
            m_state = EngineState::Running;
        }
        else if (m_state == EngineState::Running)
        {
            m_state = EngineState::Paused;
            m_window->SetCursorLocked(false);
        }
        else if (m_state == EngineState::Paused)
        {
            m_state = EngineState::Running;
            m_window->SetCursorLocked(true);
        }
    }

    // ProcessEvents is called here, once per real frame, not inside the
    // fixed-step loop. IsKeyJustPressed would be true on every sub-step
    // that shares the same input snapshot, causing interactions (doors,
    // lamps) to fire multiple times and toggle back to their original state.
    if (m_player && m_world && m_state == EngineState::Running)
        m_player->ProcessEvents(*m_input, *m_world);
}

void Engine::Update(float dt)
{
    m_world->Update(dt);
    m_player->Update(dt, *m_input, *m_world);
}

void Engine::Render()
{
    // 1. Render scene into the low-res framebuffer.
    m_renderer->BeginFrame();
    m_renderer->RenderWorld(*m_world, m_player->GetCamera());

    // 2. Apply post-process (dither + palette quantisation).
    m_renderer->ApplyPostProcess();

    // 3. Blit low-res buffer to the window with letterboxing.
    m_renderer->Present(m_window->GetWidth(), m_window->GetHeight());

    // 4. Draw ImGui at native resolution so it stays sharp.
    m_editorUI->Render(*this);
}

void Engine::Shutdown()
{
    std::cout << "[Engine] Shutting down...\n";
    m_state = EngineState::Shutdown;

    // Destroy in reverse initialisation order.
    m_editorUI.reset();
    m_player.reset();
    m_world.reset();
    m_renderer.reset();
    m_input.reset();
    m_window.reset();
    m_timer.reset();

    std::cout << "[Engine] Goodbye.\n";
}

void Engine::RequestShutdown()
{
    m_state = EngineState::Shutdown;
}

Window&          Engine::GetWindow()    const { return *m_window;    }
Renderer&        Engine::GetRenderer()  const { return *m_renderer;  }
Input&           Engine::GetInput()     const { return *m_input;     }
Timer&           Engine::GetTimer()     const { return *m_timer;     }
World&           Engine::GetWorld()     const { return *m_world;     }
Player&          Engine::GetPlayer()    const { return *m_player;    }
EditorUI&        Engine::GetEditorUI()  const { return *m_editorUI;  }
InventorySystem& Engine::GetInventory() const { return *m_inventory; }
