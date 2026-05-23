// ============================================================
//  Engine.cpp  —  ObradexEngine Core Implementation
// ============================================================

#include "Engine.h"
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Renderer.h"
#include "World.h"
#include "Player.h"
#include "EditorUI.h"

#include <iostream>
#include <stdexcept>

// ── Singleton ─────────────────────────────────────────────────
Engine& Engine::Get()
{
    // Meyer's singleton — thread-safe in C++11+
    static Engine instance;
    return instance;
}

// ── Init ──────────────────────────────────────────────────────
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
        // ── 1. Timer (no deps) ────────────────────────────────
        m_timer = std::make_unique<Timer>();
        std::cout << "[Engine]  Timer        OK\n";

        // ── 2. Window (needs GLFW) ────────────────────────────
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

        // ── 3. Input (needs Window) ───────────────────────────
        m_input = std::make_unique<Input>();
        m_input->Init(m_window->GetGLFWWindow());
        std::cout << "[Engine]  Input        OK\n";

        // ── 4. Renderer (needs OpenGL context from Window) ────
        m_renderer = std::make_unique<Renderer>();
        if (!m_renderer->Init(config.renderWidth, config.renderHeight))
        {
            throw std::runtime_error("Renderer init failed.");
        }
        std::cout << "[Engine]  Renderer     OK\n";

        // ── 5. World ──────────────────────────────────────────
        m_world = std::make_unique<World>();
        m_world->Init();
        std::cout << "[Engine]  World        OK\n";

        // ── 6. Player ─────────────────────────────────────────
        m_player = std::make_unique<Player>();
        m_player->Init();
        std::cout << "[Engine]  Player       OK\n";

        // ── 7. Editor UI (ImGui; needs Window + OpenGL) ───────
        m_editorUI = std::make_unique<EditorUI>();
        m_editorUI->Init(m_window->GetGLFWWindow());
        std::cout << "[Engine]  EditorUI     OK\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Engine] FATAL — " << e.what() << "\n";
        return false;
    }

    m_state = EngineState::Running;
    std::cout << "[Engine] All systems nominal. Entering game loop.\n";
    return true;
}

// ── Run (main game loop) ──────────────────────────────────────
void Engine::Run()
{
    if (m_state != EngineState::Running)
    {
        std::cerr << "[Engine] Run() called before Init().\n";
        return;
    }

    m_timer->Reset();

    // ── Game Loop ─────────────────────────────────────────────
    // Fixed-timestep physics with variable rendering.
    // This decouples physics stability from frame rate.
    const float fixedDt      = 1.0f / static_cast<float>(m_config.targetFPS);
    float       accumulator  = 0.0f;
    
    while (!m_window->ShouldClose() && m_state != EngineState::Shutdown)
    {
        // Measure real elapsed time this frame
        float frameDt = m_timer->Tick();

        // Guard against spiral-of-death on very slow frames
        if (frameDt > 0.25f) frameDt = 0.25f;
        accumulator += frameDt;

        // ── Poll OS events FIRST ──────────────────────────────
        // glfwPollEvents() processes the WM_KEYDOWN / WM_KEYUP /
        // WM_MOUSEMOVE messages that the OS queued since the last
        // frame.  It must run BEFORE Input::Update() reads
        // glfwGetKey() — otherwise the keyboard state is always
        // one frame stale and keys appear unresponsive.
        m_window->PollEvents();

        // ── Input snapshot (once per real frame) ──────────────
        ProcessInput();

        // ── Fixed Update (physics / game logic) ───────────────
        while (accumulator >= fixedDt)
        {
            if (m_state == EngineState::Running)
                Update(fixedDt);
            accumulator -= fixedDt;
        }

        // ── Render (once per real frame) ──────────────────────
        // Alpha = how far we are into the next fixed step;
        // used for sub-frame interpolation if desired.
        // const float alpha = accumulator / fixedDt;
        Render();

        m_window->SwapBuffers();
    }

    Shutdown();
}

// ── ProcessInput ──────────────────────────────────────────────
void Engine::ProcessInput()
{
    m_input->Update();

    // Engine-level hotkeys (bypass game-logic layer)
    if (m_input->IsKeyJustPressed(Key::F1))
    {
        m_editorUI->ToggleEditorPanel();
    }

    if (m_input->IsKeyJustPressed(Key::Escape))
    {
        // Toggle pause or ask for quit confirmation via UI
        if (m_state == EngineState::Running)
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

    // Interaction events — called here (once per real frame) so that
    // IsKeyJustPressed fires exactly once per key press.
    // Calling this inside the fixed-timestep Update loop would cause
    // onInteract to fire on every physics sub-step that shares the same
    // input snapshot, making doors/lamps toggle back to their original
    // state and appear unresponsive.
    if (m_player && m_world && m_state == EngineState::Running)
        m_player->ProcessEvents(*m_input, *m_world);
}

// ── Update ────────────────────────────────────────────────────
void Engine::Update(float dt)
{
    m_world->Update(dt);
    m_player->Update(dt, *m_input, *m_world);
}

// ── Render ────────────────────────────────────────────────────
void Engine::Render()
{
    // 1. Draw the 3-D scene into the low-res framebuffer
    m_renderer->BeginFrame();
    m_renderer->RenderWorld(*m_world, m_player->GetCamera());

    // 2. Apply post-processing (dither + palette quantisation)
    m_renderer->ApplyPostProcess();

    // 3. Blit low-res framebuffer to the window at full resolution
    m_renderer->Present(m_window->GetWidth(), m_window->GetHeight());

    // 4. Render ImGui on top (at native resolution — stays crisp)
    m_editorUI->Render(*this);
}

// ── Shutdown ──────────────────────────────────────────────────
void Engine::Shutdown()
{
    std::cout << "[Engine] Shutting down...\n";
    m_state = EngineState::Shutdown;

    // Reverse-order destruction (unique_ptr auto-destructs)
    m_editorUI.reset();
    m_player.reset();
    m_world.reset();
    m_renderer.reset();
    m_input.reset();
    m_window.reset();
    m_timer.reset();

    std::cout << "[Engine] Goodbye.\n";
}

// ── RequestShutdown ───────────────────────────────────────────
void Engine::RequestShutdown()
{
    m_state = EngineState::Shutdown;
}

// ── Subsystem accessors ───────────────────────────────────────
Window&   Engine::GetWindow()   const { return *m_window;   }
Renderer& Engine::GetRenderer() const { return *m_renderer; }
Input&    Engine::GetInput()    const { return *m_input;    }
Timer&    Engine::GetTimer()    const { return *m_timer;    }
World&    Engine::GetWorld()    const { return *m_world;    }
Player&   Engine::GetPlayer()   const { return *m_player;   }
EditorUI& Engine::GetEditorUI() const { return *m_editorUI; }
