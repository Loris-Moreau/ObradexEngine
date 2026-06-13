# ObradexEngine - Technical Documentation

![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/Loris-Moreau/ObradexEngine)

> **Engine style:** Inspired by ***Return of the Obra Dinn***
>
> **Gameplay reference:** ***Deus Ex*** & ***Assassin's Creed Syndicate*** *(first-person)*

---

## Table of Contents

1. [Project Overview](#1-project-overview)
2. [Directory Layout](#2-directory-layout)
3. [Quick-Start Build Guide](#3-quick-start-build-guide)
4. [Third-Party Dependencies](#4-third-party-dependencies)
5. [Architecture Overview](#5-architecture-overview)
6. [Subsystem Reference](#6-subsystem-reference)
7. [Rendering Pipeline](#7-rendering-pipeline)
8. [Shader Reference](#8-shader-reference)
9. [Controls](#9-controls)
10. [Level File Format](#10-level-file-format)
11. [Extending the Engine](#11-extending-the-engine)
12. [Roadmap & Issues](RoadmapAndIssues.md)

---

## 1. Project Overview

ObradexEngine is a self-contained C++17 game engine built around the aesthetic of *Return of the Obra Dinn* (limited palette, heavy dithering) crossed with classic 8-bit pixel art. The gameplay layer targets immersive-sim first-person interactions in the spirit of *Deus Ex* and *Assassin's Creed Syndicate*.

**Key aesthetic properties:**
- Renders to a low-resolution internal framebuffer (default 320 × 180 - a 4:1 scale on a 1280 × 720 window).
- A post-process pass applies Bayer ordered dithering, palette quantisation, scanline overlay, and vignette.
- "Obra Dinn mode" forces greyscale + ordered dithering for the classic woodcut look.
- All UI (crosshair, interact prompts, inventory, editor) is drawn at native resolution on top, staying crisp.

---

## 2. Directory Layout

```
ObradexEngine/
├── CMakeLists.txt              - CMake build definition
├── setup.sh                    - One-time dependency bootstrapper (Git Bash / Linux)
├── setup.ps1                   - One-time dependency bootstrapper (PowerShell / Windows)
├── README.md
├── docs/
│   ├── DOCUMENTATION.md        - This file
│   └── RoadmapAndIssues.md     - Known issues, fix log, roadmap
├── bitEngine/                  - All source files (flat layout)
│   ├── main.cpp
│   ├── Engine.h/.cpp           - Singleton core; owns all subsystems; drives the loop
│   ├── Window.h/.cpp           - GLFW window + OpenGL 4.1 context
│   ├── Input.h/.cpp            - Frame-snapshot keyboard / mouse / scroll
│   ├── Timer.h/.cpp            - High-resolution frame timer + smoothed FPS
│   ├── Renderer.h/.cpp         - Top-level render coordinator
│   ├── Shader.h/.cpp           - GLSL compile/link + uniform setters
│   ├── Mesh.h/.cpp             - VAO/VBO/EBO + primitive factories (cube, plane, sphere)
│   ├── Camera.h/.cpp           - FPS camera: yaw/pitch, lean, head-bob
│   ├── PostProcess.h/.cpp      - FBO capture, dither, palette, vignette
│   ├── World.h/.cpp            - Entity/component scene container
│   ├── Player.h/.cpp           - Movement state machine + interaction dispatch
│   ├── Interaction.h/.cpp      - Spawn helpers: lamps, doors, containers, pickups, alarms
│   ├── InventorySystem.h/.cpp  - Item storage + Deus Ex-style overlay UI
│   ├── EditorUI.h/.cpp         - Dear ImGui runtime editor + HUD + container popup
│   ├── LevelEditor.h/.cpp      - Level save/load (.lvl) + entity placement panel
│   ├── Shaders/
│   │   ├── world.vert
│   │   └── world.frag
│   └── Levels/
│       ├── level.lvl
│       └── levelBase.lvl
├── glad/                       - Pre-generated OpenGL 4.1 Core loader
├── third_party/                - Vendored libraries (populated by setup scripts)
│   ├── glad/
│   ├── imgui/                  - Dear ImGui
│   ├── stb/                    - stb_image
│   └── glm/                    - GLM math
└── build/                      - CMake / MSVC build output
```

---

## 3. Quick-Start Build Guide

### Prerequisites

| Tool           | Min version                   | Notes                                                |
|----------------|-------------------------------|------------------------------------------------------|
| C++ compiler   | MSVC 2022 / GCC 11 / Clang 13 | C++17 required                                       |
| CMake          | 3.20                          |                                                      |
| Git            | any                           | For ImGui clone in setup.sh                          |
| OpenGL drivers | 4.1 Core                      | macOS capped at 4.1; Windows/Linux support up to 4.6 |

### Visual Studio 2022 / JetBrains Rider (Windows - recommended)

```powershell
# 1. Fetch all third-party dependencies
./setup.ps1

# 2. Open bitEngine.sln in Visual Studio 2022 or Rider
#    Build target: Debug x64

# 3. Run - the working directory must be bitEngine/
#    (VS sets this automatically via the .vcxproj)
```

### CMake (cross-platform)

```bash
# 1. Fetch dependencies (Git Bash on Windows, bash on Linux/macOS)
bash setup.sh

# 2. Configure
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 3. Build
cmake --build . -j$(nproc)

# 4. Run from the build output directory
./bin/Debug/bitEngine
```

---

## 4. Third-Party Dependencies

| Library        | Version         | License       | How obtained                                        |
|----------------|-----------------|---------------|-----------------------------------------------------|
| **GLAD**       | OpenGL 4.1 Core | MIT           | Pre-bundled in `glad/`; generated via glad.dav1d.de |
| **Dear ImGui** | 1.90.x          | MIT           | Vendored in `third_party/imgui/`                    |
| **stb_image**  | latest          | Public domain | Vendored in `third_party/stb/`                      |
| **GLM**        | 0.9.9.8         | MIT           | Vendored in `third_party/glm/`                      |
| **GLFW**       | ≥ 3.3           | Zlib          | Bundled in `third_party/imgui/examples/libs/glfw/`  |
| **OpenGL**     | 4.1 Core        | -             | System / GPU driver                                 |

All libraries are vendored. No internet access required at build time after setup runs.

---

## 5. Architecture Overview

```
main()
  └── Engine::Init()
        ├── Timer          (no deps)
        ├── Window         (GLFW init, OpenGL context)
        ├── Input          (GLFW callbacks)
        ├── Renderer       (shaders, FBO, post-process)
        ├── World          (scene entities, mesh cache)
        ├── Player         (movement, camera)
        ├── InventorySystem(item storage, overlay UI)
        └── EditorUI       (Dear ImGui context)
  └── Engine::Run()        ← fixed-timestep game loop
        ├── PollEvents()   - glfwPollEvents (must be first)
        ├── ProcessInput() - Input::Update(), hotkeys (F1, Escape, I)
        ├── Update(dt)     - World::Update(), Player::Update()
        └── Render()       - BeginFrame → RenderWorld → PostProcess → Present → ImGui
```

**Design principles:**
- **Singleton engine** (`Engine::Get()`) owns all subsystems as `unique_ptr`; destruction is automatic RAII in reverse declaration order.
- **Fixed-timestep physics** with uncapped rendering. The accumulator pattern avoids spiral-of-death on slow machines.
- **No ECS framework** - a `vector<EntityRecord>` with optional component pointers is fast enough at the target entity count (~512) and easier to debug.
- **Low-res FBO → nearest-neighbour upscale** is the entire "pixel art" trick: render at 320 × 180, blit to 1280 × 720 without interpolation.
- **Interaction key is a single constant** (`constexpr Key INTERACT_KEY = Key::F` in `Input.h`). All prompt strings are built dynamically via `Input::GetKeyName(INTERACT_KEY)`, so rebinding requires changing one line.

---

## 6. Subsystem Reference

### 6.1 Engine (`Engine.h`)

Root singleton. Use `Engine::Get()` everywhere.

```cpp
Engine& engine = Engine::Get();
engine.Init(config);         // Allocate all subsystems
engine.Run();                // Blocking game loop
engine.RequestShutdown();    // Signal clean exit from game code
engine.StartGame();           // Load alpha_demo.lvl and enter Running state
engine.ReturnToMainMenu();    // Clear level and return to MainMenu
engine.RespawnPlayer();       // Reset player at spawn, resume Running
engine.NotifyPlayerDied();    // Called by Player when health reaches zero
engine.NotifyLevelComplete(); // Called by the exit trigger
```

**EngineConfig fields:**

| Field                | Default               | Description                              |
|----------------------|-----------------------|------------------------------------------|
| `windowTitle`        | `"Obradex - Version"` | Window title bar                         |
| `windowWidth/Height` | 1280 × 720            | OS window size in pixels                 |
| `fullscreen`         | false                 | Borderless fullscreen on primary monitor |
| `vsync`              | true                  | Vertical sync                            |
| `targetFPS`          | 60                    | Fixed-step physics tick rate             |
| `renderWidth/Height` | 320 × 180             | Internal low-res buffer size             |

**EngineState transitions:**
```
Uninitialized -> MainMenu -> Running <-> Paused
                                 |
                            GameOver / LevelComplete -> MainMenu
```
Escape priority while Running: open container > open inventory > pause.

---

### 6.2 Window (`Window.h`)

Wraps GLFW. Destructor calls `glfwTerminate()` automatically.

```cpp
Window& win = engine.GetWindow();
win.SetCursorLocked(true);   // Capture mouse (FPS mode)
win.GetWidth();              // Current framebuffer width
win.GetAspect();             // width / height
```

---

### 6.3 Input (`Input.h`)

Frame-snapshot polling - no raw callbacks exposed to game code.

```cpp
input.Update();                        // Call once per frame after PollEvents

input.IsKeyHeld(Key::Z);              // True every frame the key is down
input.IsKeyJustPressed(Key::F);       // True only on the frame it goes down
input.IsKeyJustReleased(Key::LShift); // True only on the frame it goes up

input.GetMouseDelta();   // vec2: pixels moved since last frame
input.GetScrollDelta();  // float: scroll wheel detents
```

`Key` enum values mirror `GLFW_KEY_*` constants directly (zero-cost cast).

**Remapping the interact key:** change `constexpr Key INTERACT_KEY` in `Input.h`. All prompt strings update automatically.

---

### 6.4 Timer (`Timer.h`)

Uses `std::chrono::steady_clock` for monotonic timing.

```cpp
timer.Reset();            // Call before the game loop
float dt = timer.Tick();  // Returns delta-time in seconds
timer.GetFPS();           // 60-frame rolling average
timer.GetTotalTime();     // Elapsed since Reset (seconds)
```

---

### 6.5 Renderer (`Renderer.h`)

Orchestrates the rendering pipeline each frame:

```cpp
renderer.BeginFrame();                  // Bind low-res FBO
renderer.RenderWorld(world, camera);    // Draw entities
renderer.ApplyPostProcess();            // Unbind FBO
renderer.Present(winW, winH);           // Dither + blit to window
```

**Lighting:** one directional light + up to 8 point lights per frame.

```cpp
renderer.SetSunDirection({-0.4f, -0.8f, -0.3f});
renderer.SetSunColour({0.7f, 0.75f, 0.85f});
renderer.AddPointLight({ position, colour, radius, intensity });
renderer.ClearLights();
```

---

### 6.6 Shader (`Shader.h`)

RAII wrapper around an OpenGL program object.

```cpp
Shader sh;
sh.Load("Shaders/world.vert", "Shaders/world.frag");
// or from in-memory strings:
sh.LoadFromSource(vertSrc, fragSrc);

sh.Bind();
sh.SetMat4("u_Model", model);
sh.SetVec3("u_AlbedoColour", {0.8f, 0.6f, 0.3f});
sh.Unbind();
```

Move-only (GPU resource ownership follows the object). No copy.

---

### 6.7 Mesh (`Mesh.h`)

Interleaved vertex buffer: `position (vec3) | normal (vec3) | texCoord (vec2)` = 32 bytes/vertex.

```cpp
// Custom geometry
Mesh m;
m.Upload(vertices, indices);
m.Draw();   // glDrawElements(GL_TRIANGLES, ...)

// Built-in primitives (also available via World getters)
Mesh cube   = Mesh::MakeCube();
Mesh plane  = Mesh::MakePlane(10.f, 4);   // 10 m, 4 subdivisions
Mesh sphere = Mesh::MakeSphere(1.f, 16);  // r=1, 16 latitudinal segments
```

The three primitive meshes (`cube`, `plane`, `sphere`) are owned by `World` and reused by every entity - they are created once in `World::Init()` and never duplicated.

---

### 6.8 Camera (`Camera.h`)

First-person Euler-angle camera.

```cpp
camera.Rotate(dx, dy, sensitivity);  // Mouse look
camera.SetLean(angle);               // ±15° peek (Thief-style)
camera.UpdateHeadBob(speed, dt);     // Lissajous walking bob

camera.GetView();              // mat4
camera.GetProjection(aspect);  // mat4 perspective
camera.GetForward();           // normalised vec3
```

Yaw wraps to `[-180, 180]` to prevent float drift. Pitch clamps to `[-89, 89]`.

---

### 6.9 PostProcess (`PostProcess.h`)

Manages the low-res FBO and the full-screen dither/palette shader.

```cpp
PostProcessSettings& pp = renderer.GetPostProcess().Settings();

// Obra Dinn look
pp.obraDinnMode   = true;
pp.ditherStrength = 1.6f;
pp.paletteSize    = 2;
pp.contrast       = 1.8f;

// 8-bit color look
pp.obraDinnMode   = false;
pp.paletteSize    = 16;
pp.scanlines      = true;
```

All settings are exposed as sliders in the **Renderer** tab of the editor. Presets (Obra Dinn, 8-bit, Bright Colored, Dark Colored) can be applied with one click.

---

### 6.10 World (`World.h`)

Scene container with a simple entity/component system.

```cpp
EntityID e = world.CreateEntity("Crate");

auto* t = world.AddTransform(e);
t->position = {3.f, 0.5f, 5.f};

auto* m = world.AddMesh(e);
m->mesh         = world.GetCubeMesh();
m->albedoColour = {0.45f, 0.35f, 0.22f};

auto* ia = world.AddInteractable(e);
ia->promptText = "[F] Open crate";
ia->onInteract = []() { /* ... */ };

world.DestroyEntity(e);   // Marks inactive; safe during iteration
world.ClearLevel();       // Remove all entities; keep GPU meshes
```

**Component types:**

| Component               | Purpose                                                |
|-------------------------|--------------------------------------------------------|
| `TransformComponent`    | Position, rotation (quat), scale → model matrix        |
| `MeshComponent`         | Mesh pointer + material (albedo, specular, roughness)  |
| `InteractableComponent` | Prompt text, reach range, `onInteract` callback        |
| `LightComponent`        | Point light: colour, radius, intensity, candle flicker |
| `TriggerComponent`      | AABB trigger with `onEnter` / `onExit` callbacks       |
| `CollisionComponent`    | Solid AABB for player push-out. `slippery = false` (default) zeroes horizontal velocity on landing so boxes behave like the floor. Set `slippery = true` for surfaces the player should slide off. |
| `ContainerComponent`    | Item list + `isOpen` flag for the loot popup           |

**Component storage:** each component type lives in a pre-reserved `std::vector` (capacity 1024). `EntityRecord` holds raw pointers into these vectors. Reservation prevents reallocation-induced pointer invalidation.

---

### 6.11 Player (`Player.h`)

First-person controller with a movement state machine.

**States:**

| State       | Trigger          | Speed                        |
|-------------|------------------|------------------------------|
| `Standing`  | Default          | `walkSpeed` (2.25 m/s)       |
| `Sprinting` | Shift + movement | `sprintSpeed` (8 m/s)        |
| `Crouching` | Ctrl             | `crouchSpeed` (1.125 m/s)    |
| `Sliding`   | Sprint + Ctrl    | `slideSpeed` (10 m/s, 0.6 s) |
| `InAir`     | Jump / fall      | Carries horizontal velocity  |
| `Vaulting`  | *(stub)*         | Auto-climb low ledges        |

**Lean:** A leans left, E leans right (when not sprinting). Max ±12°, lerped at 8 × dt.

**Two-phase input:**
- `Player::Update` runs inside the fixed-timestep loop (physics, movement, gravity, collision).
- `Player::ProcessEvents` runs once per real frame (interaction, prevents multi-fire on `IsKeyJustPressed`).

**Tuning at runtime** via the editor's **Player** tab:
```cpp
PlayerStats& s = engine.GetPlayer().GetStats();
s.walkSpeed        = 4.f;
s.mouseSensitivity = 0.12f;
s.interactRange    = 2.5f;
```

---

### 6.12 Interaction System (`Interaction.h`)

Factory functions that spawn pre-wired interactable entities. All return the root `EntityID`.

```cpp
// Lamppost - post + child light, interact toggles light
Interaction::SpawnLamppost(world, {6.f, 0.f, 0.f});

// Door - pivots 90° CCW around its Z-negative hinge
Interaction::SpawnDoor(world, {5.f, 0.f, 3.f},
    /*locked=*/false,
    []() { std::cout << "Opened!\n"; });

// Container - 9-item cap enforced at spawn; popup shown on interact
Interaction::SpawnContainer(world, {2.f, 0.2f, 4.f},
    { {"Lockpick", "A steel pick", 3}, {"Medkit", "First aid", 1} });

// Pickup - hides on grab, routes item to InventorySystem via callback
Interaction::SpawnPickup(world, {1.f, 0.f, 4.f},
    {"Ammo", "9mm rounds", 12},
    [](const Item& it) { Engine::Get().GetInventory().AddItem(it); });

// Alarm - red light + defuse interact
Interaction::SpawnAlarm(world, {-3.f, 1.5f, 0.f});
```

---

### 6.13 InventorySystem (`InventorySystem.h`)

Stores collected items as a flat list of stacks, keyed by item name.

```cpp
InventorySystem& inv = engine.GetInventory();

inv.AddItem({"Lockpick", "A steel pick", 3});  // Stacks with existing entry
inv.HasItem("Lockpick", 2);                    // true
inv.GetQuantity("Lockpick");                   // 3
inv.RemoveItem("Lockpick", 1);                 // Returns true; stack becomes 2
inv.Toggle();                                  // Open / close the UI overlay
```

**UI:** Press **I** to open the Deus Ex: Mankind Divided-style overlay. The cursor is unlocked while the inventory is open. Press **I** again or click the X to close.

Items enter the inventory from:
- Individual slot clicks and **Grab All** in the container popup (`EditorUI::DrawContainerPopup`).
- Standalone pickup `onInteract` callbacks (`Interaction::SpawnPickup`).

---

### 6.14 EditorUI (`EditorUI.h`)

Dear ImGui runtime editor, toggled with **F1**.

**Tabs:**

| Tab              | Contents                                                                        |
|------------------|---------------------------------------------------------------------------------| 
| **Perf**         | FPS, frame time, total time, 90-frame FPS sparkline, render + window resolution |
| **Renderer**     | All `PostProcessSettings` sliders + preset buttons                              |
| **Player**       | Move state, position/speed, all `PlayerStats` sliders, FOV + yaw/pitch          |
| **World**        | Entity list + component inspector (transform, material, light, interactable)    |
| **Level Editor** | Browse + Save + Load + New; entity placement; scene entity list                 |

The **HUD** (crosshair + interact prompt + movement state badge) always renders, even when the editor panel is hidden.

The **container popup** renders as a centred 3×3 amber/dark grid whenever `World::HasOpenContainer()` is true. The cursor unlocks automatically while it is visible.

---

## 7. Rendering Pipeline

```
1. BeginCapture()
   Bind low-res FBO (e.g. 320 × 180)
   glClear(COLOR | DEPTH)

2. RenderWorld()
   World shader (Blinn-Phong + 1 directional + up to 8 point lights)
   For each active entity with Mesh + Transform:
     Upload u_Model, u_AlbedoColour, u_Specular, u_Roughness
     mesh->Draw()  (glDrawElements)

3. EndCapture()
   Unbind low-res FBO

4. Present(winW, winH)
   Bind default FBO; viewport = full window
   Post-process shader on a fullscreen quad:
     a. Sample low-res texture
     b. Contrast / brightness
     c. Optional greyscale (Obra Dinn mode)
     d. Bayer 8×8 ordered dither
     e. Palette quantise (nearest colour, 2–32 entries)
     f. Vignette (radial darkening)
     g. Scanline overlay (every 2 px, optional)

5. EditorUI::Render()
   ImGui at native resolution (sharp, no pixel scaling)
   HUD always drawn
   Editor panel if F1 active
   Inventory overlay if I active
   Container popup if a container is open
```

The nearest-neighbour upscale from 320 × 180 → 1280 × 720 is handled by OpenGL automatically when the FBO colour texture uses `GL_NEAREST` filtering and is drawn on a fullscreen NDC quad.

---

## 8. Shader Reference

### world.vert
**Inputs:** `location 0` = position (vec3), `location 1` = normal (vec3), `location 2` = texCoord (vec2)

**Uniforms:** `u_Model`, `u_View`, `u_Proj` (mat4)

**Outputs:** world-space `v_FragPos`, `v_Normal`, `v_TexCoord`

### world.frag

| Uniform                      | Type  | Description                            |
|------------------------------|-------|----------------------------------------|
| `u_AlbedoColour`             | vec3  | Base colour                            |
| `u_Specular`                 | float | Specular intensity                     |
| `u_Roughness`                | float | Inverts shininess exponent             |
| `u_SunDir`                   | vec3  | World-space direction toward the light |
| `u_SunColour`                | vec3  | Directional light colour               |
| `u_Ambient`                  | vec3  | Constant fill light                    |
| `u_PointLights[i].position`  | vec3  | Point light world position             |
| `u_PointLights[i].colour`    | vec3  | Point light colour                     |
| `u_PointLights[i].radius`    | float | Attenuation distance (m)               |
| `u_PointLights[i].intensity` | float | Brightness scalar                      |
| `u_PointLightCount`          | int   | Active lights (0–8)                    |
| `u_CamPos`                   | vec3  | Camera world position (specular)       |
| `u_FogDensity`               | float | Exponential fog density (0 = off)      |
| `u_FogColour`                | vec3  | Fog blend target colour                |

### Post-process shader (embedded in PostProcess.cpp)

Key uniforms: `u_Scene` (sampler2D), `u_Palette` (sampler1D), `u_PaletteSize`, `u_Dither`, `u_ObraDinn`, `u_Resolution`, `u_Contrast`, `u_Brightness`, `u_VigRadius`, `u_VigFeather`, `u_Scanlines`, `u_ScanAlpha`.

---

## 9. Controls

| Key / Input   | Action                                                  |
|---------------|---------------------------------------------------------|
| Z / Q / S / D | Move forward / left / back / right (physical key position; HUD shows OS label) |
| Mouse X / Y   | Camera yaw / pitch                                      |
| Left Shift    | Sprint                                                  |
| Left Ctrl     | Crouch; if already sprinting, slide                     |
| Space         | Jump (ground only, not while crouching)                 |
| A             | Lean left (not while sprinting or sliding)              |
| E             | Lean right (not while sprinting or sliding)             |
| F             | Interact (open doors, search containers, pick up items) |
| I             | Toggle inventory overlay                                |
| F1            | Toggle editor panel                                     |
| Escape        | Close open container or inventory, or toggle pause      |

---

## 10. Level File Format

Levels are stored as plain-text `.lvl` files in the `Levels/` directory. Use the Level Editor tab in the editor (F1) to save and load; the **Browse** button opens the OS file picker.

```
LEVEL_VERSION 1

ENTITY
  TYPE cube
  NAME Crate
  POS 3.0 0.5 5.0
  SCALE 1.0 1.0 1.0
  ALBEDO 0.69 0.39 0.098
  SPECULAR 0.2
  ROUGHNESS 0.9
  COLLISION 0.5 0.5 0.5
END

ENTITY
  TYPE sphere
  NAME DecorBall
  POS -2.0 1.0 3.0
  SCALE 0.5 0.5 0.5
  ALBEDO 0.8 0.8 0.8
  SPECULAR 0.6
  ROUGHNESS 0.3
END

ENTITY
  TYPE container
  NAME Footlocker
  POS -1.0 0.2 4.0
  ITEM Lockpick 3
  ITEM Medkit 1
  ITEM Ammo 12
END

ENTITY
  TYPE lamppost
  NAME Lamppost
  POS 6.0 0.0 0.0
  LIGHT_COLOR 1.0 0.85 0.5
  LIGHT_RADIUS 10.0
  LIGHT_INTENSITY 1.5
  LIGHT_FLICKER 1
END
```

**Supported TYPE values:**

| Type        | Description                                |
|-------------|--------------------------------------------|
| `cube`      | Static box mesh + optional collision       |
| `plane`     | Flat plane mesh (floor / ceiling / wall)   |
| `sphere`    | UV sphere mesh + optional collision        |
| `lamppost`  | Post + child light; interact toggles light |
| `door`      | Pivoting door panel                        |
| `container` | Searchable loot box (max 9 items)          |
| `pickup`    | Collectible ground item                    |
| `alarm`     | Armed alarm box; deals 25 damage while armed, defuse interact to disarm |
| `light`     | Standalone point light (no mesh)           |
| `spawn`     | Sets the player spawn point (no mesh shown in-game)                     |

Item names with spaces are serialised with underscores (`First_Aid_Kit`) and restored on load.

---

## 11. Extending the Engine

### Adding a new component type

1. Define `struct MyComponent { ... };` in `World.h`.
2. Add `std::vector<MyComponent> m_myComponents;` to `World` and call `.reserve(MAX)` in `ReserveComponentStorage()`.
3. Add `MyComponent* myComponent = nullptr;` to `EntityRecord`.
4. Implement `AddMyComponent(EntityID)` and optionally `GetMyComponent(EntityID)` following the existing pattern.
5. Update `World::Update()` to tick the new component if needed.

### Adding a new interaction factory

1. Add a `SpawnMyThing(World&, ...)` function to `Interaction.h/.cpp`.
2. Wire up `AddTransform`, `AddMesh`, `AddInteractable`, and the `onInteract` lambda.
3. Add a spawn case in `LevelEditor::SpawnCurrent` and a type string in `LevelEditor::SaveLevel` / `LoadLevel`.
4. Add the new type name to `kSpawnTypeNames` in `LevelEditor.h`.

### Adding a new subsystem

1. Create `MySystem.h/.cpp` in `bitEngine/`.
2. Add `std::unique_ptr<MySystem> m_mySystem;` to `Engine.h` (order determines destruction order - last declared, first destroyed).
3. Construct and `Init()` in `Engine::Init()`.
4. Call `Update()` / `Render()` from the appropriate engine phase.
5. Add a `GetMySystem() const` accessor.

### Adding textures

1. Use `stb_image.h` to load PNG/JPG into a `uint8_t*` buffer.
2. Create a GL texture and upload with `glTexImage2D`.
3. Add a `GLuint textureID` field to `MeshComponent`.
4. Bind to texture unit 2 before `mesh->Draw()` in `World::Render()`.
5. Set `u_HasTexture = 1` and sample `u_DiffuseMap` in `world.frag` (uniform stub already declared).

---

### 6.15 AudioSystem (`AudioSystem.h`)

No-op stub. All methods compile and link; no audio is produced until miniaudio is integrated.

```cpp
AudioSystem& audio = engine.GetAudio();
audio.LoadSound("click", "sounds/click.wav");
audio.Play("click");
audio.SetMasterVolume(0.8f);
```

To integrate miniaudio:
1. Download `miniaudio.h` from https://github.com/mackron/miniaudio.
2. Place it in `third_party/miniaudio/`.
3. Create one `.cpp` with `#define MINIAUDIO_IMPLEMENTATION` before the include.
4. Replace the stub bodies in `AudioSystem.cpp` with `ma_engine` calls.

---

### 6.16 ConfigLoader (`ConfigLoader.h`)

Parses INI-style `[section] / key = value` files. Keys and section names are case-insensitive. Lines starting with `#` or `;` are comments. Missing keys return the supplied default — the binary works without a config file.

```cpp
ConfigLoader& cfg = engine.GetConfig();
int   width  = cfg.GetInt  ("window", "width",  1280);
float sens   = cfg.GetFloat("player", "sensitivity", 0.12f);
bool  vsync  = cfg.GetBool ("window", "vsync",  true);
```

`config.ini` is loaded by `Engine::Init` before any subsystem is created. Changes require a restart.
