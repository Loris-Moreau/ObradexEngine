#pragma once

// ============================================================
//  Engine.h  —  ObradexEngine Core
// ============================================================
//  The Engine class is the root of the entire application.
//  It owns every major subsystem (Window, Renderer, Input,
//  World, Player, UI) and drives the main game loop.
//
//  Lifecycle:
//    Engine::Get().Init()   — allocate & initialise all systems
//    Engine::Get().Run()    — enter the blocking game loop
//    Engine::Get().Shutdown()— release resources (called by Run)
//
//  Design pattern: Singleton accessed via Engine::Get().
//  Systems are stored as unique_ptr to guarantee RAII cleanup
//  and to decouple headers (forward-declarations suffice).
// ============================================================

#include <memory>
#include <string>

// Forward-declare subsystems so headers don't bleed everywhere
class Window;
class Renderer;
class Input;
class Timer;
class World;
class Player;
class EditorUI;
class InventorySystem;

// ── Engine State ─────────────────────────────────────────────
enum class EngineState
{
    Uninitialized,  ///< Before Init() is called
    Running,        ///< Normal game-loop execution
    Paused,         ///< Game logic frozen; editor still ticks
    Shutdown        ///< Cleanup in progress
};

// ── Engine Config ─────────────────────────────────────────────
/// Passed to Engine::Init() to configure startup behaviour.
struct EngineConfig
{
    std::string windowTitle   = "Obradex";
    int         windowWidth   = 1280;
    int         windowHeight  = 720;
    bool        fullscreen    = false;
    bool        vsync         = true;
    int         targetFPS     = 60;

    // Render resolution for the internal low-res buffer.
    // The image is upscaled to the window size, giving the
    // crisp pixel-art / Obra-Dinn dithered look.
    int         renderWidth   = 320;
    int         renderHeight  = 180;
};

// ── Engine ────────────────────────────────────────────────────
class Engine
{
public:
    // ── Singleton access ──────────────────────────────────────
    static Engine& Get();

    // ── Prevent copy/move (singleton) ─────────────────────────
    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(Engine&&)      = delete;

    // ── Lifecycle ─────────────────────────────────────────────

    /// Initialise all subsystems in dependency order.
    /// @param config  Startup configuration struct.
    /// @return true on success, false if a fatal error occurred.
    bool Init(const EngineConfig& config = {});

    /// Enter the main game loop. Blocks until the window is
    /// closed or RequestShutdown() is called.
    void Run();

    /// Signal the game loop to exit cleanly on the next frame.
    void RequestShutdown();

    // ── Subsystem accessors ───────────────────────────────────
    /// @{
    Window&   GetWindow()   const;
    Renderer& GetRenderer() const;
    Input&    GetInput()    const;
    Timer&    GetTimer()    const;
    World&    GetWorld()    const;
    Player&   GetPlayer()   const;
    EditorUI& GetEditorUI() const;
    InventorySystem& GetInventory() const;
    /// @}

    // ── State ─────────────────────────────────────────────────
    EngineState       GetState()  const { return m_state; }
    const EngineConfig& GetConfig() const { return m_config; }

private:
    Engine() = default;  ///< Private — use Engine::Get()
    ~Engine() = default;

    // ── Internal loop phases ──────────────────────────────────
    void ProcessInput();   ///< Poll events, feed Input system
    void Update(float dt); ///< Advance game logic
    void Render();         ///< Draw world + UI
    void Shutdown();       ///< Destroy subsystems in reverse order

    // ── Members ───────────────────────────────────────────────
    EngineConfig m_config;
    EngineState  m_state = EngineState::Uninitialized;

    // Subsystems (ordered — destruction is reverse of declaration)
    std::unique_ptr<Timer>           m_timer;
    std::unique_ptr<Input>           m_input;
    std::unique_ptr<Window>          m_window;
    std::unique_ptr<Renderer>        m_renderer;
    std::unique_ptr<World>           m_world;
    std::unique_ptr<Player>          m_player;
    std::unique_ptr<InventorySystem> m_inventory;
    std::unique_ptr<EditorUI>        m_editorUI;
};
