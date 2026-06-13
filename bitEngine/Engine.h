#pragma once

// Engine.h - Root singleton. Owns all subsystems and drives the game loop.
//
// State machine:
//   Uninitialized -> MainMenu -> Running <-> Paused
//                                  |
//                             GameOver / LevelComplete -> MainMenu

#include <memory>
#include <fstream>
#include <string>

class Window;
class Renderer;
class Input;
class Timer;
class World;
class Player;
class EditorUI;
class InventorySystem;
class AudioSystem;
class ConfigLoader;

enum class EngineState
{
    Uninitialized,
    MainMenu,       // Title screen before gameplay begins
    Running,        // Normal gameplay
    Paused,         // Physics frozen; editor still ticks
    GameOver,       // Player health reached zero
    LevelComplete,  // Player reached the exit trigger
    Shutdown
};

struct EngineConfig
{
    std::string windowTitle  = "Obradex";
    int  windowWidth         = 1280;
    int  windowHeight        = 720;
    bool fullscreen          = false;
    bool vsync               = true;
    int  targetFPS           = 60;
    int  renderWidth         = 320;
    int  renderHeight        = 180;
    float sensitivity        = 0.12f;
    float masterVolume       = 1.0f;
};

class Engine
{
public:
    static Engine& Get();

    Engine(const Engine&)            = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&)                 = delete;
    Engine& operator=(Engine&&)      = delete;

    bool Init(const EngineConfig& config = {});
    void Run();

    void RequestShutdown()     { m_state = EngineState::Shutdown; }
    void SetState(EngineState s){ m_state = s; }

    void NotifyPlayerDied();
    void NotifyLevelComplete();
    void StartGame();
    void ReturnToMainMenu();
    void RespawnPlayer();

    Window&          GetWindow()    const;
    Renderer&        GetRenderer()  const;
    Input&           GetInput()     const;
    Timer&           GetTimer()     const;
    World&           GetWorld()     const;
    Player&          GetPlayer()    const;
    EditorUI&        GetEditorUI()  const;
    InventorySystem& GetInventory() const;
    AudioSystem&     GetAudio()     const;
    ConfigLoader&    GetConfig()    const;

    EngineState         GetState()  const { return m_state;  }
    const EngineConfig& GetEngineConfig()const { return m_config; }

private:
    Engine()  = default;
    ~Engine() = default;

    void ProcessInput();
    void Update(float dt);
    void Render();
    void Shutdown();
    void InitLogFile();
    void LoadConfig();

    EngineConfig  m_config;
    EngineState   m_state = EngineState::Uninitialized;
    std::ofstream m_logFile;

    std::unique_ptr<Timer>           m_timer;
    std::unique_ptr<Input>           m_input;
    std::unique_ptr<Window>          m_window;
    std::unique_ptr<Renderer>        m_renderer;
    std::unique_ptr<World>           m_world;
    std::unique_ptr<Player>          m_player;
    std::unique_ptr<InventorySystem> m_inventory;
    std::unique_ptr<EditorUI>        m_editorUI;
    std::unique_ptr<AudioSystem>     m_audio;
    std::unique_ptr<ConfigLoader>    m_configLoader;
};
