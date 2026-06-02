#pragma once

// Engine.h - Root singleton that owns every subsystem and drives the game loop.
//
// Lifecycle:
//   Engine::Get().Init(config)  - allocate and initialise all systems in order
//   Engine::Get().Run()         - enter the blocking game loop
//   Engine::Get().Shutdown()    - release resources (called automatically by Run)
//
// Systems are stored as unique_ptr for RAII cleanup and to keep headers lean
// (forward declarations are enough in most callers).

#include <memory>
#include <string>

class Window;
class Renderer;
class Input;
class Timer;
class World;
class Player;
class EditorUI;
class InventorySystem;

enum class EngineState
{
    Uninitialized,  // Before Init() is called
    Running,        // Normal game-loop execution
    Paused,         // Game logic frozen; editor still ticks
    Shutdown        // Cleanup in progress
};

// Passed to Engine::Init() to configure startup behaviour.
struct EngineConfig
{
    std::string windowTitle   = "Obradex";
    int         windowWidth   = 1280;
    int         windowHeight  = 720;
    bool        fullscreen    = false;
    bool        vsync         = true;
    int         targetFPS     = 60;

    // Resolution of the internal low-res render buffer. The image is
    // upscaled to the window size for the pixel-art / dithered look.
    int         renderWidth   = 320;
    int         renderHeight  = 180;
};

class Engine
{
public:
    static Engine& Get();  // Meyers singleton, thread-safe in C++11+

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(Engine&&)      = delete;

    // Initialise all subsystems in dependency order.
    // Returns true on success, false if a fatal error occurred.
    bool Init(const EngineConfig& config = {});

    // Enter the main game loop. Blocks until the window is closed
    // or RequestShutdown() is called.
    void Run();

    // Signal the game loop to exit cleanly on the next frame.
    void RequestShutdown();

    Window&          GetWindow()    const;
    Renderer&        GetRenderer()  const;
    Input&           GetInput()     const;
    Timer&           GetTimer()     const;
    World&           GetWorld()     const;
    Player&          GetPlayer()    const;
    EditorUI&        GetEditorUI()  const;
    InventorySystem& GetInventory() const;

    EngineState         GetState()  const { return m_state;  }
    const EngineConfig& GetConfig() const { return m_config; }

private:
    Engine()  = default;
    ~Engine() = default;

    void ProcessInput();    // Poll events and feed the input system
    void Update(float dt);  // Advance game logic by one fixed step
    void Render();          // Draw world + UI
    void Shutdown();        // Destroy subsystems in reverse order

    EngineConfig m_config;
    EngineState  m_state = EngineState::Uninitialized;

    // Subsystem order matters: destruction is reverse of declaration.
    std::unique_ptr<Timer>           m_timer;
    std::unique_ptr<Input>           m_input;
    std::unique_ptr<Window>          m_window;
    std::unique_ptr<Renderer>        m_renderer;
    std::unique_ptr<World>           m_world;
    std::unique_ptr<Player>          m_player;
    std::unique_ptr<InventorySystem> m_inventory;
    std::unique_ptr<EditorUI>        m_editorUI;
};
