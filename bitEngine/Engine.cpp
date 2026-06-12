// Engine.cpp - Core engine implementation.
#include "Engine.h"
#include "Window.h"
#include "Input.h"
#include "Timer.h"
#include "Renderer.h"
#include "World.h"
#include "Player.h"
#include "EditorUI.h"
#include "InventorySystem.h"
#include "AudioSystem.h"
#include "ConfigLoader.h"
#include <iostream>
#include "LevelEditor.h"
#include <fstream>
#include <stdexcept>

// Tee-buffer: mirrors writes to both console and log file.
struct TeeBuf : std::streambuf {
    std::streambuf *orig, *file;
    TeeBuf(std::streambuf* o, std::streambuf* f): orig(o), file(f){}
    int overflow(int c) override {
        if(c==EOF)return EOF;
        if(orig)orig->sputc(char(c));
        if(file)file->sputc(char(c));
        return c;
    }
    std::streamsize xsputn(const char* s,std::streamsize n) override {
        if(orig)orig->sputn(s,n);
        if(file)file->sputn(s,n);
        return n;
    }
};
static TeeBuf *s_coutTee=nullptr, *s_cerrTee=nullptr;

Engine& Engine::Get(){ static Engine e; return e; }

void Engine::InitLogFile(){
    m_logFile.open("../log.txt", std::ios::out|std::ios::trunc);
    if(m_logFile.is_open()){
        s_coutTee = new TeeBuf(std::cout.rdbuf(), m_logFile.rdbuf());
        s_cerrTee = new TeeBuf(std::cerr.rdbuf(), m_logFile.rdbuf());
        std::cout.rdbuf(s_coutTee);
        std::cerr.rdbuf(s_cerrTee);
        std::cout << "[Engine] Logging to log.txt\n";
    }
}

void Engine::LoadConfig(){
    m_configLoader = std::make_unique<ConfigLoader>();
    m_configLoader->Load("config.ini");
    m_config.windowWidth  = m_configLoader->GetInt  ("window","width",         m_config.windowWidth);
    m_config.windowHeight = m_configLoader->GetInt  ("window","height",        m_config.windowHeight);
    m_config.fullscreen   = m_configLoader->GetBool ("window","fullscreen",    m_config.fullscreen);
    m_config.vsync        = m_configLoader->GetBool ("window","vsync",         m_config.vsync);
    m_config.renderWidth  = m_configLoader->GetInt  ("render","width",         m_config.renderWidth);
    m_config.renderHeight = m_configLoader->GetInt  ("render","height",        m_config.renderHeight);
    m_config.targetFPS    = m_configLoader->GetInt  ("render","fps",           m_config.targetFPS);
    m_config.sensitivity  = m_configLoader->GetFloat("player","sensitivity",   m_config.sensitivity);
    m_config.masterVolume = m_configLoader->GetFloat("audio", "master_volume", m_config.masterVolume);
}

bool Engine::Init(const EngineConfig& config){
    if(m_state!=EngineState::Uninitialized){
        std::cerr<<"[Engine] Init() called more than once.\n"; return false;
    }
    m_config = config;
    InitLogFile();
    LoadConfig();
    std::cout<<"[Engine] Initialising ObradexEngine\n";
    try{
        m_timer = std::make_unique<Timer>();
        std::cout<<"[Engine]  Timer        OK\n";

        m_window = std::make_unique<Window>();
        if(!m_window->Init(m_config.windowTitle,
                           m_config.windowWidth, m_config.windowHeight,
                           m_config.fullscreen,  m_config.vsync))
            throw std::runtime_error("Window init failed.");
        std::cout<<"[Engine]  Window       OK\n";

        m_input = std::make_unique<Input>();
        m_input->Init(m_window->GetGLFWWindow());
        std::cout<<"[Engine]  Input        OK\n";

        m_renderer = std::make_unique<Renderer>();
        if(!m_renderer->Init(m_config.renderWidth, m_config.renderHeight))
            throw std::runtime_error("Renderer init failed.");
        std::cout<<"[Engine]  Renderer     OK\n";

        m_world = std::make_unique<World>();
        m_world->Init();
        std::cout<<"[Engine]  World        OK\n";

        m_player = std::make_unique<Player>();
        m_player->Init();
        m_player->GetStats().mouseSensitivity = m_config.sensitivity;
        std::cout<<"[Engine]  Player       OK\n";

        m_inventory = std::make_unique<InventorySystem>();
        std::cout<<"[Engine]  Inventory    OK\n";

        m_audio = std::make_unique<AudioSystem>();
        m_audio->Init();
        m_audio->SetMasterVolume(m_config.masterVolume);
        std::cout<<"[Engine]  Audio        OK\n";

        m_editorUI = std::make_unique<EditorUI>();
        m_editorUI->Init(m_window->GetGLFWWindow());
        std::cout<<"[Engine]  EditorUI     OK\n";
    }
    catch(const std::exception& e){
        std::cerr<<"[Engine] FATAL: "<<e.what()<<"\n"; return false;
    }
    m_state = EngineState::MainMenu;
    m_window->SetCursorLocked(false);
    std::cout<<"[Engine] Ready. Showing main menu.\n";
    return true;
}

void Engine::StartGame(){
    m_world->ClearLevel();

    // Try to load the packaged demo level; fall back to built-in test scene.
    {
        LevelEditor tmpLoader;
        bool loaded = tmpLoader.LoadLevel(*m_world, "Levels/alpha_demo.lvl");
        if (!loaded)
        {
            std::cout << "[Engine] alpha_demo.lvl not found, using built-in level.\n";
            m_world->LoadDefaultLevel();
        }
    }
    m_player->Init();
    m_player->SetSpawnPos(m_world->GetSpawnPos());
    m_player->RespawnAtSpawn();
    m_inventory->Clear();
    m_world->ResetLevelComplete();
    m_state = EngineState::Running;
    m_window->SetCursorLocked(true);
}

void Engine::ReturnToMainMenu(){
    m_world->ClearLevel();
    m_world->Init();  // Rebuild primitive meshes
    m_player->Init();
    m_inventory->Clear();
    m_state = EngineState::MainMenu;
    m_window->SetCursorLocked(false);
}

void Engine::RespawnPlayer(){
    m_player->RespawnAtSpawn();
    m_state = EngineState::Running;
    m_window->SetCursorLocked(true);
}

void Engine::NotifyPlayerDied(){
    if(m_state==EngineState::Running){
        m_state = EngineState::GameOver;
        m_window->SetCursorLocked(false);
    }
}

void Engine::NotifyLevelComplete(){
    if(m_state==EngineState::Running){
        m_state = EngineState::LevelComplete;
        m_window->SetCursorLocked(false);
    }
}

void Engine::Run(){
    if(m_state==EngineState::Uninitialized){
        std::cerr<<"[Engine] Run() before Init().\n"; return;
    }
    m_timer->Reset();
    const float fixedDt = 1.f / float(m_config.targetFPS);
    float acc = 0.f;
    while(!m_window->ShouldClose() && m_state!=EngineState::Shutdown){
        float dt = m_timer->Tick();
        if(dt>0.25f)dt=0.25f;
        acc+=dt;
        m_window->PollEvents();
        ProcessInput();
        while(acc>=fixedDt){
            if(m_state==EngineState::Running) Update(fixedDt);
            acc-=fixedDt;
        }
        Render();
        m_window->SwapBuffers();
    }
    Shutdown();
}

void Engine::ProcessInput(){
    m_input->Update();
    if(m_input->IsKeyJustPressed(Key::F1))
        m_editorUI->ToggleEditorPanel();

    if(m_input->IsKeyJustPressed(Key::I) &&
       (m_state==EngineState::Running||m_state==EngineState::Paused) &&
       !m_world->HasOpenContainer()){
        if(m_inventory->IsOpen()){
            m_inventory->Toggle();
            m_window->SetCursorLocked(true);
            if(m_state==EngineState::Paused) m_state=EngineState::Running;
        } else if(m_state==EngineState::Running){
            m_inventory->Toggle();
            m_window->SetCursorLocked(false);
            m_state=EngineState::Paused;
        }
    }

    if(m_input->IsKeyJustPressed(Key::Escape)){
        if(m_world->HasOpenContainer())          m_world->CloseOpenContainer();
        else if(m_inventory->IsOpen()){
            m_inventory->Toggle();
            m_window->SetCursorLocked(true);
            m_state=EngineState::Running;
        }
        else if(m_state==EngineState::Running){
            m_state=EngineState::Paused;
            m_window->SetCursorLocked(false);
        }
        else if(m_state==EngineState::Paused){
            m_state=EngineState::Running;
            m_window->SetCursorLocked(true);
        }
    }

    if(m_state==EngineState::Running && m_player && m_world)
        m_player->ProcessEvents(*m_input, *m_world);
}

void Engine::Update(float dt){
    m_world->Update(dt);
    m_player->Update(dt, *m_input, *m_world);
    if(m_player->IsDead())           NotifyPlayerDied();
    if(m_world->IsLevelComplete())   NotifyLevelComplete();
}

void Engine::Render(){
    // Collect point lights from World LightComponent entities each frame.
    m_renderer->ClearLights();
    for(const auto& rec : m_world->GetAllRecords()){
        if(!rec.active||!rec.light||!rec.transform) continue;
        if(rec.light->intensity<=0.f) continue;
        PointLight pl;
        pl.position  = rec.transform->position;
        pl.colour    = rec.light->colour;
        pl.radius    = rec.light->radius;
        pl.intensity = rec.light->intensity;
        m_renderer->AddPointLight(pl);
    }
    m_renderer->BeginFrame();
    m_renderer->RenderWorld(*m_world, m_player->GetCamera());
    m_renderer->ApplyPostProcess();
    m_renderer->Present(m_window->GetWidth(), m_window->GetHeight());
    m_editorUI->Render(*this);
}

void Engine::Shutdown(){
    std::cout<<"[Engine] Shutting down...\n";
    m_state=EngineState::Shutdown;
    if(m_audio) m_audio->Shutdown();
    m_editorUI.reset(); m_audio.reset(); m_inventory.reset();
    m_player.reset(); m_world.reset(); m_renderer.reset();
    m_input.reset(); m_window.reset(); m_timer.reset();
    std::cout<<"[Engine] Goodbye.\n";
    if(s_coutTee){ std::cout.rdbuf(s_coutTee->orig); delete s_coutTee; s_coutTee=nullptr; }
    if(s_cerrTee){ std::cerr.rdbuf(s_cerrTee->orig); delete s_cerrTee; s_cerrTee=nullptr; }
}

Window&          Engine::GetWindow()   const{ return *m_window;      }
Renderer&        Engine::GetRenderer() const{ return *m_renderer;    }
Input&           Engine::GetInput()    const{ return *m_input;       }
Timer&           Engine::GetTimer()    const{ return *m_timer;       }
World&           Engine::GetWorld()    const{ return *m_world;       }
Player&          Engine::GetPlayer()   const{ return *m_player;      }
EditorUI&        Engine::GetEditorUI() const{ return *m_editorUI;    }
InventorySystem& Engine::GetInventory()const{ return *m_inventory;   }
AudioSystem&     Engine::GetAudio()    const{ return *m_audio;       }
ConfigLoader&    Engine::GetConfig()   const{ return *m_configLoader;}
