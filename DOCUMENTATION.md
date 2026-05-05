# ObradexEngine — Developer Documentation

> **Version 0.1.0 — Pre-Alpha**
> A first-person immersive-sim engine with an 8-bit / *Return of the Obra Dinn* aesthetic, built in C++17 with OpenGL 4.1 and Dear ImGui.

---

## Table of Contents

1. [Overview & Design Goals](#1-overview--design-goals)
2. [Architecture Diagram](#2-architecture-diagram)
3. [Directory Structure](#3-directory-structure)
4. [Dependencies](#4-dependencies)
5. [Building the Engine](#5-building-the-engine)
6. [System Reference](#6-system-reference)
   - 6.1 [Engine (Core Loop)](#61-engine-core-loop)
   - 6.2 [Window](#62-window)
   - 6.3 [Input](#63-input)
   - 6.4 [Timer](#64-timer)
   - 6.5 [Renderer](#65-renderer)
   - 6.6 [Shader](#66-shader)
   - 6.7 [Mesh](#67-mesh)
   - 6.8 [Camera](#68-camera)
   - 6.9 [PostProcess](#69-postprocess)
   - 6.10 [World & ECS-lite](#610-world--ecs-lite)
   - 6.11 [Player Controller](#611-player-controller)
   - 6.12 [Interaction System](#612-interaction-system)
   - 6.13 [EditorUI (ImGui)](#613-editorui-imgui)
7. [GLSL Shader Reference](#7-glsl-shader-reference)
8. [Gameplay Systems](#8-gameplay-systems)
9. [Aesthetic Pipeline](#9-aesthetic-pipeline)
10. [Extending the Engine](#10-extending-the-engine)
11. [Known Limitations & Roadmap](#11-known-limitations--roadmap)
12. [Coding Conventions](#12-coding-conventions)

---

## 1. Overview & Design Goals

ObradexEngine is a deliberately minimal, single-file-per-system C++ game engine targeting a specific aesthetic and gameplay feel:

| Goal | Implementation |
|---|---|
| **"Return of the Obra Dinn" look** | Low-res FBO (320×180), Bayer ordered dithering, palette quantisation, vignette |
| **"8-bit game" feel** | Nearest-neighbour upscale, 2–32 colour palette, scanline overlay |
| **Deus Ex movement** | Lean, crouch, crouch-slide, interaction prompts, interactable objects |
| **Assassin's Creed Syndicate movement** | Sprint, jump, trigger zones, kinematic controller |
| **Runtime editor** | Dear ImGui panel (F1): post-process tweaking, entity inspector, player stats |
| **Simple codebase** | ~2 500 lines, no external frameworks beyond OpenGL/GLFW/ImGui/GLM |

The engine is intentionally small enough to read in a day and modify without fighting an abstraction wall.

---

## 2. Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│            EngineConfig → Engine::Init() → Run()        │
└───────────────────────┬─────────────────────────────────┘
                        │ owns
        ┌───────────────▼────────────────────────────────┐
        │                  Engine (Singleton)             │
        │  ProcessInput() → Update(dt) → Render()        │
        │                                                 │
        │  ┌──────┐ ┌───────┐ ┌──────┐ ┌──────────────┐ │
        │  │Timer │ │ Input │ │Window│ │   Renderer   │ │
        │  └──────┘ └───────┘ └──────┘ │  ┌─────────┐ │ │
        │                              │  │ Shader  │ │ │
        │  ┌───────────────────┐       │  │ PostFX  │ │ │
        │  │       World       │       │  └─────────┘ │ │
        │  │  EntityRecord[]   │       └──────────────┘ │
        │  │  Component pools  │                        │
        │  └───────────────────┘                        │
        │                                               │
        │  ┌──────────────────┐  ┌───────────────────┐  │
        │  │  Player          │  │     EditorUI      │  │
        │  │  Camera          │  │  (ImGui panels)   │  │
        │  │  MoveState FSM   │  └───────────────────┘  │
        │  └──────────────────┘                         │
        └────────────────────────────────────────────────┘

Render pipeline:
  World geometry ──► low-res FBO (320×180)
                        │
                        ▼
               Dither + Palette pass
                        │
                        ▼
             Nearest-neighbour blit to window (1280×720)
                        │
                        ▼
               ImGui overlay (native resolution)
```

---

## 3. Directory Structure

```
engine/
├── CMakeLists.txt          Build system
├── setup.sh                Dependency bootstrapper
├── src/
│   ├── main.cpp            Entry point + EngineConfig
│   ├── engine/
│   │   ├── Engine.h/.cpp   Core loop, singleton, subsystem ownership
│   │   ├── Window.h/.cpp   GLFW window + OpenGL context
│   │   ├── Input.h/.cpp    Keyboard/mouse polling (frame-delta based)
│   │   └── Timer.h/.cpp    High-resolution frame timing + FPS smoothing
│   ├── renderer/
│   │   ├── Renderer.h/.cpp Main render coordinator
│   │   ├── Shader.h/.cpp   GLSL compile/link + uniform setters
│   │   ├── Mesh.h/.cpp     VAO/VBO/EBO + primitive factories
│   │   ├── Camera.h/.cpp   First-person camera (yaw/pitch, lean, head-bob)
│   │   └── PostProcess.h/.cpp Dither + palette FBO pass
│   ├── world/
│   │   └── World.h/.cpp    Entity registry + component pools
│   ├── gameplay/
│   │   ├── Player.h/.cpp   FPS controller (movement FSM, interaction)
│   │   └── Interaction.h/.cpp World-interaction factories (door, pickup…)
│   └── ui/
│       └── EditorUI.h/.cpp ImGui editor overlay
├── assets/
│   └── shaders/
│       ├── world.vert      Geometry vertex shader
│       └── world.frag      Blinn-Phong lighting fragment shader
└── third_party/
    ├── glad/               OpenGL function loader (generate from glad.dav1d.de)
    ├── imgui/              Dear ImGui v1.90.1 (cloned by setup.sh)
    └── stb/                stb_image.h (fetched by setup.sh)
```

---

## 4. Dependencies

| Library | Version | Purpose | How obtained |
|---|---|---|---|
| **OpenGL** | 4.1 Core | GPU rendering | System |
| **GLFW** | ≥ 3.3 | Window, context, input events | System package or CMake FetchContent |
| **GLAD** | gl 4.1 core | OpenGL function pointer loader | Generate at glad.dav1d.de, copy to `third_party/glad/` |
| **GLM** | 0.9.9.8 | Math (vec3, mat4, quat) | System package or CMake FetchContent |
| **Dear ImGui** | 1.90.1 | In-game editor & HUD | Cloned by `setup.sh` |
| **stb_image** | latest | PNG/JPEG texture loading | Fetched by `setup.sh` |

### Installing system packages

**Ubuntu / Debian:**
```bash
sudo apt update
sudo apt install libglfw3-dev libglm-dev cmake build-essential
```

**macOS (Homebrew):**
```bash
brew install glfw glm cmake
```

**Windows (vcpkg):**
```powershell
vcpkg install glfw3 glm
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### GLAD (mandatory manual step)

1. Go to <https://glad.dav1d.de/>
2. Set **Language** = C/C++, **API / gl** = 4.1, **Profile** = Core, tick **Generate a loader**
3. Click **Generate** → download the zip
4. Copy `glad/include/glad/glad.h` → `third_party/glad/include/glad/glad.h`
5. Copy `glad/src/glad.c`          → `third_party/glad/src/glad.c`

---

## 5. Building the Engine

```bash
# 1. Clone / place the engine source
cd engine

# 2. Bootstrap third-party dependencies
chmod +x setup.sh && ./setup.sh

# 3. Generate GLAD (see Section 4 above) and copy files

# 4. Configure with CMake
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug   # or Release

# 5. Build
cmake --build . -j$(nproc)          # Linux / macOS
# cmake --build . --config Debug    # Windows

# 6. Run
./bin/ObradexEngine                  # Linux / macOS
# bin\Debug\ObradexEngine.exe       # Windows
```

### Build types

| Type | Defines | Optimisation |
|---|---|---|
| `Debug` | `OBRADEX_DEBUG` | Off — full debug info |
| `Release` | `OBRADEX_RELEASE` | `-O3` / `/O2` |

Assets are automatically copied to `build/bin/assets/` post-build.

---

## 6. System Reference

### 6.1 Engine (Core Loop)

**File:** `src/engine/Engine.h/.cpp`

The `Engine` singleton owns every subsystem and runs the game loop.

#### Lifecycle

```cpp
Engine::Get().Init(config);   // Allocates all systems in dependency order
Engine::Get().Run();          // Blocking game loop (returns when window closes)
// Shutdown() is called automatically by Run()
```

#### Fixed-timestep loop

The game loop uses a **fixed-timestep physics accumulator** pattern:

```
real frame time → accumulator
while accumulator >= fixedDt:
    Update(fixedDt)          ← game logic at stable 60 Hz
    accumulator -= fixedDt
Render()                     ← as fast as possible (vsync-limited)
```

This keeps physics deterministic regardless of frame rate spikes.

#### EngineConfig fields

| Field | Default | Description |
|---|---|---|
| `windowTitle` | `"Obradex"` | Window title bar |
| `windowWidth/Height` | 1280 × 720 | OS window pixel dimensions |
| `fullscreen` | `false` | Borderless fullscreen |
| `vsync` | `true` | Swap interval |
| `targetFPS` | 60 | Fixed-update rate |
| `renderWidth/Height` | 320 × 180 | Internal low-res FBO |

#### Engine states

```
Uninitialized → Running → Paused → Running
                       ↓
                    Shutdown
```

Press **Escape** in-game to toggle pause.  
Call `Engine::Get().RequestShutdown()` from anywhere to exit cleanly.

---

### 6.2 Window

**File:** `src/engine/Window.h/.cpp`

Wraps GLFW. Creates an **OpenGL 4.1 Core** context.

```cpp
engine.GetWindow().SetCursorLocked(true);   // FPS mouse capture
engine.GetWindow().GetWidth();              // Current pixel width
engine.GetWindow().GetAspect();             // width / height
engine.GetWindow().GetGLFWWindow();         // Raw GLFWwindow* for ImGui
```

A `FramebufferSizeCallback` automatically resizes the viewport when the OS window is resized.

---

### 6.3 Input

**File:** `src/engine/Input.h/.cpp`

Frame-based polling with **double-buffered state** — no raw GLFW callbacks needed in gameplay code.

```cpp
const Input& input = engine.GetInput();

// Keyboard
input.IsKeyHeld(Key::W);             // true every frame key is held
input.IsKeyJustPressed(Key::Space);  // true only the frame it went down
input.IsKeyJustReleased(Key::LShift);

// Mouse
input.GetMouseDelta();   // glm::vec2 — pixels moved since last frame
input.GetScrollDelta();  // float — scroll wheel detents this frame
input.IsButtonJustPressed(MouseButton::Left);
```

#### Key enum

Values match `GLFW_KEY_*` constants directly (cast to `int`), so there is zero overhead from a lookup table.  See `Input.h` for the full list.

#### Adding new keys

Add an entry to the `Key` enum in `Input.h` using the matching `GLFW_KEY_*` integer value.

---

### 6.4 Timer

**File:** `src/engine/Timer.h/.cpp`

Uses `std::chrono::steady_clock` — monotonic, immune to wall-clock adjustments.

```cpp
const Timer& t = engine.GetTimer();
t.GetDeltaTime();   // float seconds — last frame
t.GetTotalTime();   // float seconds — since Reset()
t.GetFPS();         // float — rolling average over 60 frames
t.GetFrameCount();  // int
```

---

### 6.5 Renderer

**File:** `src/renderer/Renderer.h/.cpp`

Coordinates the render pipeline:

```cpp
Renderer& r = engine.GetRenderer();

// Add lights before RenderWorld
r.ClearLights();
r.AddPointLight({ .position={3,2,0}, .colour={1,.8,.5}, .radius=8, .intensity=1.5 });
r.SetSunDirection(glm::normalize({-1,-2,-1}));
r.SetSunColour({0.7f, 0.75f, 0.85f});   // Cool moonlight

// Normally called by Engine automatically — shown for clarity
r.BeginFrame();
r.RenderWorld(world, camera);
r.ApplyPostProcess();
r.Present(windowW, windowH);
```

**Lighting model:** Blinn-Phong with one directional light + up to **8 point lights** per frame.  The count limit is enforced in both C++ (`kMaxPointLights`) and GLSL (`u_PointLights[8]`).

---

### 6.6 Shader

**File:** `src/renderer/Shader.h/.cpp`

Compiles GLSL vertex + fragment stages, links them, and exposes type-safe uniform setters.

```cpp
Shader sh;
sh.Load("assets/shaders/world.vert", "assets/shaders/world.frag");
// — or from embedded strings —
sh.LoadFromSource(vertSrc, fragSrc);

sh.Bind();
sh.SetMat4("u_MVP", mvp);
sh.SetVec3("u_SunColour", {0.7f, 0.75f, 0.85f});
sh.SetFloat("u_Roughness", 0.8f);
sh.SetInt("u_HasTexture", 0);
sh.Unbind();
```

Errors are logged to `stderr` with the GLSL compile log.  The `Shader` destructor calls `glDeleteProgram` automatically.

---

### 6.7 Mesh

**File:** `src/renderer/Mesh.h/.cpp`

Uploads interleaved `Vertex` data (position + normal + UV) to a VAO/VBO/EBO on the GPU.

```cpp
// Use a built-in primitive
Mesh cube   = Mesh::MakeCube();
Mesh plane  = Mesh::MakePlane(10.f, 4);   // 10m × 10m, 4 subdivisions
Mesh sphere = Mesh::MakeSphere(0.5f, 16); // r=0.5m, 16 segments

// Or upload custom geometry
std::vector<Vertex>   verts = { ... };
std::vector<unsigned> idx   = { ... };
Mesh custom;
custom.Upload(verts, idx);

// Draw (shader must be bound by caller)
shader.Bind();
shader.SetMat4("u_Model", model);
cube.Draw();
```

**Vertex layout** (32 bytes, interleaved):
```
offset  0 : vec3 position
offset 12 : vec3 normal
offset 24 : vec2 texCoord
```
Attribute locations: 0 = position, 1 = normal, 2 = texCoord.

---

### 6.8 Camera

**File:** `src/renderer/Camera.h/.cpp`

First-person camera with Euler-angle orientation, lean, and head-bob.

```cpp
Camera cam({0, 1.7, 0});  // eye position

// Per-frame
cam.Rotate(mouseDx, mouseDy, sensitivity);
cam.SetLean(-12.f);            // Lean left 12°
cam.UpdateHeadBob(speed, dt);

// Matrices
glm::mat4 view = cam.GetView();
glm::mat4 proj = cam.GetProjection(aspectRatio);

// Direction vectors (recalculated after every Rotate call)
cam.GetForward();   // Normalised look direction
cam.GetRight();
cam.GetUp();
```

**Head-bob** uses a Lissajous figure-8 pattern (vertical + horizontal sine at different frequencies) for a natural walking feel.  It automatically damps out when the player's speed drops below 0.5 m/s.

**Lean** smoothly interpolates with `kLeanSpeed = 8 rad/s`.  The lean is applied as a roll rotation around the view Z-axis in `GetView()`.

---

### 6.9 PostProcess

**File:** `src/renderer/PostProcess.h/.cpp`

The most visually significant system — entirely implemented in a single GLSL fragment shader.

#### Pipeline (per frame)

```
3D scene → low-res FBO texture (GL_NEAREST)
           │
           ▼
  1. Contrast / Brightness
  2. Obra Dinn greyscale (optional)
  3. Bayer 8×8 ordered dithering
  4. Nearest palette colour lookup (brute-force, 32 entries)
  5. Vignette (radial darkening)
  6. Scanline overlay (every 2 pixels)
           │
           ▼
  Blit to window (nearest-neighbour → crisp pixels)
```

#### PostProcessSettings (live-editable via EditorUI)

```cpp
PostProcessSettings& pp = engine.GetRenderer().GetPostProcess().Settings();

pp.obraDinnMode   = true;   // Force monochrome + aggressive dither
pp.ditherStrength = 1.6f;   // 0 = off, 1 = standard, 2 = heavy
pp.paletteSize    = 16;     // Active palette entries (2–32)
pp.contrast       = 1.2f;
pp.brightness     = 0.0f;   // [-1 .. 1]
pp.vignetteRadius = 0.7f;
pp.scanlines      = true;
pp.scanlineAlpha  = 0.15f;
```

#### Built-in palette

The default 32-colour palette is defined in `PostProcess.cpp` as `kDefaultPalette`.  The first 16 entries are the standard "Obra Dinn inspired" set:

- Near-black to white (7 greyscale ramps)
- Parchment / aged-paper / amber / mahogany (warm ink tones)
- Deep teal / teal / cyan-teal / navy (cool atmospheric tones)

Entries 17–32 extend with reds, golds, mossy greens, purples, and earth tones for colour levels.

#### Adding a custom palette

Edit the `kDefaultPalette` array in `PostProcess.cpp` — it is a plain `glm::vec3[32]` in linear colour space (0..1).  The palette is uploaded to a 1D GPU texture via `BuildPalette()`.

#### Obra Dinn preset (code)

```cpp
pp.obraDinnMode   = true;
pp.ditherStrength = 1.6f;
pp.paletteSize    = 2;      // Black and white only
pp.contrast       = 1.8f;
pp.scanlines      = false;
pp.vignetteRadius = 0.6f;
```

---

### 6.10 World & ECS-lite

**File:** `src/world/World.h/.cpp`

The `World` is a minimalist entity-component system.  An **Entity** is just a `uint32_t` handle.  Components are plain structs stored in `std::vector` pools.

#### Creating entities & attaching components

```cpp
World& world = engine.GetWorld();

EntityID e = world.CreateEntity("MyObject");

TransformComponent* t = world.AddTransform(e);
t->position = {3.f, 0.5f, 5.f};
t->scale    = {1.f, 1.f, 1.f};

MeshComponent* m = world.AddMesh(e);
m->mesh         = &myMesh;          // non-owning pointer
m->albedoColour = {0.4f, 0.3f, 0.2f};
m->roughness    = 0.9f;

InteractableComponent* ia = world.AddInteractable(e);
ia->promptText  = "[E] Open hatch";
ia->range       = 2.f;
ia->onInteract  = []() { std::cout << "Hatch opened!\n"; };
```

#### Component types

| Component | Fields | Purpose |
|---|---|---|
| `TransformComponent` | `position`, `rotation` (quat), `scale` | World-space placement |
| `MeshComponent` | `mesh*`, `albedoColour`, `specular`, `roughness` | Visual representation |
| `InteractableComponent` | `promptText`, `range`, `enabled`, `onInteract` | E-key interactions |
| `LightComponent` | `colour`, `radius`, `intensity`, `flicker` | Point light source |
| `TriggerComponent` | `halfExtents`, `onEnter`, `onExit` | AABB trigger zone |

#### Querying the scene

```cpp
// Find the nearest interactable within 3m of a point
EntityID id = world.FindNearestInteractable(playerPos, 3.f);

// Get a specific component
TransformComponent* t = world.GetTransform(id);
InteractableComponent* ia = world.GetInteractable(id);

// Iterate all records (for editor, AI, etc.)
for (const auto& rec : world.GetAllRecords())
{
    if (!rec.active) continue;
    // rec.transform, rec.mesh, rec.light, etc.
}
```

---

### 6.11 Player Controller

**File:** `src/gameplay/Player.h/.cpp`

Implements the character controller as a state machine.

#### Movement state machine

```
┌─────────────────────────────────────────────────┐
│                                                 │
│  Standing ──LShift+move──► Sprinting            │
│     │                          │                │
│  LCtrl                    LCtrl+move            │
│     │                          │                │
│     ▼                          ▼                │
│  Crouching         Sliding ◄───┘                │
│                    (0.6s)                       │
│                       │                         │
│     Space (jump)       └──► Standing/Crouching  │
│        ▼                                        │
│     InAir ──land──► Standing                   │
│                                                 │
└─────────────────────────────────────────────────┘
```

#### Controls

| Key | Action |
|---|---|
| `W/A/S/D` | Move |
| `LShift` | Sprint |
| `LCtrl` | Crouch (hold) |
| `LCtrl` while sprinting + moving | Slide |
| `Space` | Jump |
| `Q` | Lean left |
| `E` (stationary) | Lean right |
| `E` (near object) | Interact |
| `F1` | Toggle editor |
| `Escape` | Pause / unpause |

#### Tweaking movement feel

All movement parameters are in `PlayerStats` and live-editable via the **Player** tab in the editor (F1):

```cpp
PlayerStats& stats = engine.GetPlayer().GetStats();
stats.walkSpeed    = 4.0f;   // m/s
stats.sprintSpeed  = 8.0f;   // m/s
stats.crouchSpeed  = 1.8f;   // m/s
stats.slideSpeed   = 10.0f;  // m/s (initial burst)
stats.jumpHeight   = 1.2f;   // metres
stats.gravity      = -15.f;  // m/s²
```

#### Physics note

The current collision system is a **simple Y=0 ground plane**.  For real levels, replace `ResolveCollision()` with ray-casts against scene geometry (e.g., using a BVH of AABB/triangle tests).

---

### 6.12 Interaction System

**File:** `src/gameplay/Interaction.h/.cpp`

Factory functions that create pre-wired entities:

```cpp
#include "gameplay/Interaction.h"

// Door (locked)
Interaction::SpawnDoor(world, {5.f, 0.f, 0.f}, /*locked=*/true,
    []{ std::cout << "Door opened!\n"; });

// Chest with loot
Interaction::SpawnContainer(world, {2.f, 0.5f, 3.f},
    { {"Key",   "A rusty iron key", 1},
      {"Coins", "A handful of coins", 12} },
    [](const std::vector<Item>& items){
        // Add to player inventory here
    });

// Pickup item
Interaction::SpawnPickup(world, {1.f, 0.f, 2.f},
    {"Lockpick", "A slender pick", 1},
    [](const Item& i){ /* add to inventory */ });

// Alarm
Interaction::SpawnAlarm(world, {-3.f, 1.f, 0.f},
    []{ /* alarm triggered */ },
    []{ /* alarm defused  */ });
```

---

### 6.13 EditorUI (ImGui)

**File:** `src/ui/EditorUI.h/.cpp`

Press **F1** to toggle.  The panel has four tabs:

| Tab | Contents |
|---|---|
| **Perf** | FPS, frame-time, frame count, FPS sparkline |
| **Renderer** | All `PostProcessSettings` sliders, preset buttons |
| **World** | Entity list with component inspector (position, albedo, lights) |
| **Player** | Movement state, position, all `PlayerStats` sliders, FOV |

The **HUD** (crosshair + interact prompt + movement-state badge) always renders, regardless of the F1 toggle.

#### Changing the ImGui theme

Edit the `ImGuiStyle` block inside `EditorUI::Init()`.  The defaults use near-black backgrounds with amber accent colours to match the Obra Dinn palette.

---

## 7. GLSL Shader Reference

### `world.vert`

| Attribute | Location | Type | Description |
|---|---|---|---|
| `a_Position` | 0 | `vec3` | Object-space vertex position |
| `a_Normal` | 1 | `vec3` | Object-space vertex normal |
| `a_TexCoord` | 2 | `vec2` | UV coordinates |

| Uniform | Type | Description |
|---|---|---|
| `u_Model` | `mat4` | Object → world transform |
| `u_View` | `mat4` | World → camera transform |
| `u_Proj` | `mat4` | Perspective projection |

### `world.frag`

| Uniform | Type | Description |
|---|---|---|
| `u_AlbedoColour` | `vec3` | Base colour tint |
| `u_HasTexture` | `int` | 1 = sample `u_AlbedoTex`, 0 = flat colour |
| `u_AlbedoTex` | `sampler2D` | Diffuse texture (unit 0) |
| `u_Specular` | `float` | Specular intensity 0–1 |
| `u_Roughness` | `float` | Roughness 0 (mirror) – 1 (matte) |
| `u_CamPos` | `vec3` | World-space camera position |
| `u_SunDir` | `vec3` | Directional light direction (normalised) |
| `u_SunColour` | `vec3` | Directional light colour |
| `u_Ambient` | `vec3` | Ambient fill colour |
| `u_PointLightCount` | `int` | Active point lights (0–8) |
| `u_PointLights[i].position` | `vec3` | Point light world position |
| `u_PointLights[i].colour` | `vec3` | Point light colour |
| `u_PointLights[i].radius` | `float` | Attenuation radius (m) |
| `u_PointLights[i].intensity` | `float` | Multiplier |

### Post-process pass (embedded in `PostProcess.cpp`)

Uses a fullscreen quad with a dedicated fragment shader.  See source comments for the `BayerMatrix()` and `NearestPalette()` functions.

---

## 8. Gameplay Systems

### Inventory (stub — integrate your own)

The `Interaction` system fires `onPickup(item)` / `onOpen(items)` callbacks.  Wire these into your own inventory container:

```cpp
struct Inventory {
    std::vector<Item> items;
    void Add(const Item& i) { items.push_back(i); }
};

Inventory playerInventory;
Interaction::SpawnPickup(world, pos, {"Key","Iron key",1},
    [&](const Item& i){ playerInventory.Add(i); });
```

### AI / Stealth (stub)

`Player::IsHidden()` returns `true` when the player is crouching — use this to control NPC awareness.  A full stealth system would also factor in:
- Ambient light level at the player position (ray-cast to the sun + each point light)
- NPC line-of-sight ray-cast
- Sound radius (footstep sound scales with movement state)

### Hacking mini-game (stub)

`Interaction::SpawnAlarm` is designed to be the entry point.  Replace the `onInteract` lambda body in `SpawnAlarm` (or add a new `SpawnTerminal` factory) with a call that pushes a hacking UI state — e.g., a pipeline-puzzle screen rendered via ImGui in a fullscreen window.

---

## 9. Aesthetic Pipeline

### Choosing your look

| Setting combination | Result |
|---|---|
| `obraDinnMode=true, paletteSize=2, dither=1.6` | Pure monochrome Obra Dinn |
| `obraDinnMode=false, paletteSize=4, dither=1.0` | 4-tone Gameboy-style |
| `obraDinnMode=false, paletteSize=16, dither=0.8, scanlines=true` | CRT 8-bit colour |
| `obraDinnMode=false, paletteSize=32, dither=0.3` | Colour but painterly |

### Changing render resolution

Edit `EngineConfig::renderWidth/renderHeight` in `main.cpp`:

| Resolution | Scale @1280×720 | Character |
|---|---|---|
| 160 × 90 | 8× | Extreme chunky pixels |
| 320 × 180 | 4× | Sweet spot (default) |
| 426 × 240 | ~3× | Softer retro |
| 640 × 360 | 2× | Subtle pixelation |

### Adding a custom palette

In `PostProcess.cpp`, replace entries in `kDefaultPalette[]`:

```cpp
// Each entry is a linear-space RGB glm::vec3 (0..1)
static const glm::vec3 kDefaultPalette[32] = {
    {0.f,   0.f,   0.f  },   // [0] Black
    {1.f,   1.f,   1.f  },   // [1] White
    {0.5f,  0.2f,  0.1f },   // [2] Warm mid-tone
    // ...
};
```

Set `pp.paletteSize` to however many entries you defined.

---

## 10. Extending the Engine

### Adding a new component type

1. Declare the struct in `World.h`:
   ```cpp
   struct AudioComponent {
       std::string clipPath;
       float volume = 1.f;
       bool  looping = false;
   };
   ```
2. Add a pool in `World` private members:  
   `std::vector<AudioComponent> m_audioComponents;`
3. Add `AddAudio()` / `GetAudio()` methods (pattern mirrors existing ones).
4. Update `EntityRecord` with `AudioComponent* audio = nullptr;`
5. Update `World::Render()` or add a new `World::UpdateAudio()` if needed.

### Adding a new gameplay system

Create a new class in `src/gameplay/`, initialise it in `Engine::Init()`, store it as a `unique_ptr` member, and call its `Update()` from `Engine::Update()`.

### Adding a shader

1. Place `.vert` / `.frag` files in `assets/shaders/`.
2. Load with `Shader::Load()`.
3. Set uniforms matching the layout expected by your GLSL source.

### Adding texture loading (stb_image)

`stb_image.h` is already downloaded by `setup.sh`. To load a texture:

```cpp
#define STB_IMAGE_IMPLEMENTATION  // once, in stb_image_impl.cpp
#include "stb_image.h"

int w, h, ch;
stbi_set_flip_vertically_on_load(true);
unsigned char* data = stbi_load("assets/textures/stone.png", &w, &h, &ch, 4);

unsigned texID;
glGenTextures(1, &texID);
glBindTexture(GL_TEXTURE_2D, texID);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
glGenerateMipmap(GL_TEXTURE_2D);
stbi_image_free(data);
```

Bind to texture unit 0, set `u_HasTexture = 1` on the world shader.

---

## 11. Known Limitations & Roadmap

### Current limitations

| Limitation | Impact | Planned fix |
|---|---|---|
| No mesh ray-cast collision | Player falls through geometry | BVH + triangle test in `ResolveCollision()` |
| No texture loading in World | All geometry is flat-colour | Texture cache + stb_image integration |
| No shadow maps | Ambient shadows only | PCF shadow map on the directional light |
| Single FBO (no HDR) | Post-process on LDR data | HDR FBO → tonemap → dither |
| No audio | Silent world | OpenAL-Soft or miniaudio integration |
| No serialisation | Levels are hardcoded in C++ | JSON (nlohmann) or binary level format |
| No animation | Static geometry only | Skeletal animation with GLM quaternion blending |
| Point light count capped at 8 | Limited dynamic lighting | Clustered forward shading or light grid |
| AABB trigger only | No convex/mesh triggers | GJK / convex hull overlap |

### Suggested next steps (in priority order)

1. **GLAD integration** — replace the stubs with real generated files (10 minutes, critical)
2. **Mesh collision** — ray-cast the scene to fix the player falling through geometry
3. **Texture loading** — stb_image is already fetched; wire it into `MeshComponent`
4. **Audio** — integrate [miniaudio](https://miniaud.io) (single-header, no deps)
5. **Level serialisation** — JSON with [nlohmann/json](https://github.com/nlohmann/json)
6. **Shadow maps** — depth-only pass from sun direction, PCF in `world.frag`
7. **Inventory UI** — ImGui window showing collected items
8. **AI awareness system** — NPC sight/sound cones feeding a detection meter

---

## 12. Coding Conventions

| Convention | Rule |
|---|---|
| **Naming** | `PascalCase` for classes/types, `camelCase` for locals, `m_camelCase` for members, `k_UPPER` for constants |
| **File headers** | Every `.h` and `.cpp` opens with the banner comment block |
| **Comments** | Use `///` for Doxygen-style, `//` for inline notes, `// ──` for section dividers |
| **Smart pointers** | `unique_ptr` for owned resources, raw pointers for non-owning references |
| **RAII** | Every GPU resource (VBO, shader, FBO) is owned by a class with a destructor |
| **Error handling** | Return `bool` from Init functions; log to `stderr`; throw only in Init chains |
| **No global state** | All state lives inside subsystem classes owned by `Engine` |
| **C++17** | Structured bindings, `if constexpr`, inline variables allowed |
| **No exceptions** | Except in `Engine::Init()` (wrapped in try/catch) |

---

*Documentation generated for ObradexEngine v0.1.0 — May 2026*
