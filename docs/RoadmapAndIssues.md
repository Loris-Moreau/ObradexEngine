# ObradexEngine — Roadmap, Issues & Fix Log

---

## 1. Known Limitations & Roadmap

| Area                                                  | Current State                                                                               | Planned                                                                                |
|-------------------------------------------------------|---------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------|
| Collision                                             | AABB box collision against all solid `CollisionComponent` entities + floor plane at y = 0   | Ray-cast vs mesh AABB tree or OOBB Collision; stand-up headroom check when uncrouching |
| level editor opens file explorer to select your level | need to write level name exactly                                                            | button that opens file explorer to select a specific file                              |
| Shadows                                               | None                                                                                        | Single directional shadow map                                                          |
| Audio                                                 | Not implemented                                                                             | OpenAL-Soft or miniaudio integration                                                   |
| Level loading                                         | Runtime `.lvl` save/load via ImGui Level Editor tab (plain-text format, `Levels/` folder)   | Additional entity types; level streaming                                               |
| Animation                                             | None                                                                                        | Skeletal animation via Assimp                                                          |
| Vaulting                                              | State stub only                                                                             | Auto-climb ledges ≤ 2 m high                                                           |
| Texture system                                        | Stub uniforms in world shader                                                               | Full material system with diffuse + normal maps                                        |
| Icons for container items                             | None                                                                                        | Pixel-art icons per item type, rendered in the 3×3 container grid                      |
| Inventory                                             | Not Implemented                                                                             | Full diegetic inventory UI *(Deus Ex / Arc Raiders style)*                             |
| Crate                                                 | 3×3 container grid popup with per-slot grab + Grab All; standalone pickups hide on interact | closes after grab all is clicked or when escape is pressed instead of pausing the game |
| Main Menu                                             | Not Implemented                                                                             | Continue, Start, Level Select, Settings, Quit                                          |
| Pause Menu                                            | Not Implemented                                                                             | Continue, Restart Level, Settings, Main Menu, Quit                                     |
| AI / stealth                                          | Not implemented                                                                             | Visibility cones + noise detection + random patrol movement                            |
| Weapons                                               | Not implemented                                                                             | Blunt, slash, ranged *(pistol, shotgun, sniper, semi-auto)*                            |
| Ammo system                                           | Not implemented *(requires weapons)*                                                        | Per-weapon pool: 9 mm, 12 ga, 7.62 mm, 5.56 mm                                         |
| Health system                                         | Not implemented                                                                             | Health bar, death screen, level reload                                                 |
| Networking                                            | Not planned                                                                                 | Revisit if the project ships on Steam                                                  |

---

## 2. Open Issues

| Bug                                                                               | Status | Notes                                                                                                                                                                                                                                                                                |
|-----------------------------------------------------------------------------------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Crouch does not properly disable stand-up when there is a ceiling above           | Open   | The AABB correctly shrinks to `crouchHeight = 0.85 m` when crouching. However there is no headroom check when releasing Ctrl — the player can clip through a low ceiling by standing up inside it. A sweep test upward before allowing the state transition to `Standing` is needed. |
| Inventory system missing                                                          | Open   | Items grabbed from containers and standalone pickups print to the console only. A proper inventory data structure and UI is required before weapons, ammo, and consumables can be designed.                                                                                          |
| Container grid limited to 9 slots                                                 | Open   | The 3×3 grid silently drops items beyond index 8. Containers should enforce a 9-item cap at spawn time.                                                                                                                                                                              |
| Interaction key needs to be pressed multiple times in order to do the interaction | Open   |                                                                                                                                                                                                                                                                                      |

---

## 3. Fixed

All fixes are listed chronologically. Where the same root cause was addressed more than once, entries are merged.

---

### 3.1 Build & Setup

**[FIX] GLAD not found / stub files not detected**
`setup.sh` used `wc -l` to distinguish a real GLAD file from a stub. On Windows Git Bash `wc -l` returns `"15778 "` with trailing whitespace, making the arithmetic comparison `[ "$lines" -gt 100 ]` silently fail as a string test, leaving `ALREADY_REAL=false`. The fallback then tried to regenerate GLAD and failed, leaving the file missing.

*Fix:* Detection changed to `grep -q "GLAPI" "$GLAD_H"` — the string `GLAPI` appears 3 281 times in a real generated file and never in a stub. Real GLAD files (15 778 lines for OpenGL 4.1 Core) are now generated in the sandbox and pre-bundled in the zip so no Python or pip is required on the developer machine. `setup.sh` only checks; it does not attempt generation.

---

**[FIX] `setup.sh` closes immediately on Windows without doing anything (other than creating `third_party/`)**
`set -euo pipefail` at the top of `setup.sh`, combined with CRLF line endings after Windows extraction, caused bash to see `#!/usr/bin/env bash\r` and exit immediately. The only side-effect visible to the user was the `mkdir -p "$TP"` built-in running before the shebang was interpreted.

*Fix:* Removed `set -euo pipefail`. Changed to `set -e` only, with per-command `|| warn "..."` guards. Removed ANSI escape sequences containing `!` from the `warn()` function (`!\033[0m` triggers bash history expansion in Git Bash interactive mode, causing the function definition to fail and all subsequent `warn` calls to error with `bash: warn: command not found`).

---

**[FIX] `setup.sh` skips GLAD generation on Windows (no Python)**
The bash fallback path attempted to fetch `glad.dav1d.de` and run `tar`, which silently failed. Combined with the skip-detection bug above, GLAD was always missing on developer machines without Python.

*Fix:* GLAD is now pre-bundled in the repository zip (generated at packaging time). `setup.sh` only verifies presence via `grep -q "GLAPI"` and prints manual-download instructions if the file is missing.

---

**[FIX] CMake GLM version incompatibility (`cmake_minimum_required(VERSION 2.6)`)**
GLM 0.9.9.8's own `CMakeLists.txt` declares `cmake_minimum_required(VERSION 2.6)`. CMake 3.28+ removed compatibility with versions below 3.5, causing a fatal error during `FetchContent_MakeAvailable(glm)`.

*Fix:* Replaced `FetchContent_MakeAvailable(glm)` with `FetchContent_Populate(glm)` (downloads sources without executing CMakeLists) and manually created an `INTERFACE` target pointing at the headers. GLM's `CMakeLists.txt` is never run.

---

**[FIX] `unique_ptr` with incomplete type — MSVC C2027 / C2338**

Three separate instances of this error across the project history:
- `World.h` used `std::unique_ptr<Mesh>` with only a forward declaration of `Mesh`. MSVC requires the complete type to generate `~unique_ptr<Mesh>()`. Fixed by adding `#include "Mesh.h"` and `#include <memory>` to `World.h`.
- `Renderer.h` used `std::unique_ptr<Shader>` and `std::unique_ptr<PostProcess>` with forward declarations. Fixed by replacing the forward declarations with full includes.
- `EditorUI.h` used `std::unique_ptr<LevelEditor>` with a forward-declared `LevelEditor`. `EditorUI() = default` in the header forced the compiler to generate the destructor inline, where `LevelEditor` was incomplete. Fixed by declaring `EditorUI()` and `~EditorUI()` in the header and defining both as `= default` in `EditorUI.cpp`, where `LevelEditor.h` is included.

---

**[FIX] MSVC rejects anonymous struct as template argument (`Interaction.cpp`)**
`SpawnDoor` used `std::make_shared<struct { bool open = false; bool locked; }>()`. MSVC does not allow unnamed/anonymous structs as template arguments in C++17 conformance mode.

*Fix:* Replaced with a named `struct DoorState { bool open = false; bool locked = false; };`.

---

### 3.2 Rendering & Shaders

**[FIX] GLSL multidimensional array error — dither shader crash at startup**
The post-process shader declared the Bayer dither matrix as `float[8][8]`, which GLSL forbids (no multidimensional arrays). NVIDIA's driver rejected the fragment shader and the engine exited immediately after window creation.

*Fix:* Flattened to `float[64]` with element access changed from `B[row][col]` to `B[row * 8 + col]`. The dither values and distribution are unchanged.

---

### 3.3 Input System

**[FIX] Keyboard input completely unresponsive — only mouse worked**
`glfwPollEvents()` was called at the **end** of the game loop, after `Input::Update()` had already read `glfwGetKey()`. Keyboard state (`WM_KEYDOWN` / `WM_KEYUP`) only enters GLFW's internal state after `glfwPollEvents()`; reading before polling always returned last frame's stale values. Mouse worked because `glfwGetCursorPos()` with `GLFW_CURSOR_DISABLED` (raw input mode on Windows) bypasses the message queue and reads accumulated delta directly from the driver.

*Fix:* `Window::PollEvents()` moved to the **top** of the game loop, before `ProcessInput()` and `Input::Update()`.

---

**[FIX] F1 / Escape / Interact key continuously toggled — `IsKeyJustPressed` toggled every frame a key was held**
The input double-buffer used a copy-then-overwrite pattern: `m_keys[prev] = m_keys[cur]`, then `m_keys[cur] = glfwGetKey(...)`, then `++m_frame`. After `++m_frame`, the query functions read `m_keys[m_frame & 1]`, which had just flipped to the opposite buffer — the one that had just been overwritten with the stale two-frames-ago state, not the freshly polled state. This made `IsKeyJustPressed` see PRESS vs RELEASE every single frame a key was held.

*Fix:* Switched to **flip-then-overwrite**: `m_cur = 1 - m_cur` first, then only `m_keys[m_cur][i] = glfwGetKey(...)`. The opposite slot is never touched during that frame, so it naturally retains the previous frame's snapshot. No copy step is needed.

---

**[FIX] WASD keys used on AZERTY keyboard — movement and sprint did not work**
`UpdateMoveState`, `HandleMovement`, and the `moving` check in `UpdateMoveState` referenced `Key::W` (87) and `Key::A` (65), which correspond to QWERTY positions. On an AZERTY keyboard these keys are physically at different positions and are never pressed during normal play.

*Fix:* Movement remapped to AZERTY layout: `Key::Z` (90) = forward, `Key::Q` (81) = left, `Key::S` (83) = back, `Key::D` (68) = right, `Key::A` (65) = lean left, `Key::E` (69) = lean right. A single `constexpr Key INTERACT_KEY = Key::F` in `Input.h` is the sole source of truth; all world prompts use `Input::GetKeyName(INTERACT_KEY)` to build their text dynamically.

---

### 3.4 Physics & Collision

**[FIX] Crash in `Mesh::Draw()` — vector pointer invalidation**
`EntityRecord` stored raw pointers into `std::vector` component storage (e.g. `rec.mesh = &m_meshes.back()`). Each `push_back` that caused a reallocation invalidated all previously stored pointers. By the first render frame, most entity mesh and transform pointers pointed at freed memory; calling `glBindVertexArray` through a dangling `Mesh*` crashed the driver.

*Fix:* All six component vectors are pre-reserved to 1 024 entries in `World::Init()` and `World::ClearLevel()` before any entities are created. Reallocation cannot occur as long as the scene stays within that budget.

---

**[FIX] No AABB collision — player walked through all objects**
Collision was limited to a floor plane check (`m_position.y < 0`). All solid geometry was passable.

*Fix:* Added `CollisionComponent { glm::vec3 halfExtents; bool solid; }` to the entity/component system. `Player::ResolveCollision` runs a two-pass resolution each frame: (1) floor plane at y = 0, (2) all active `CollisionComponent` entities using minimum-penetration-axis push-out. The player AABB (`kPlayerHalfW = 0.30 m`, `kPlayerHeight = 1.80 m` standing, `crouchHeight = 0.85 m` crouching) is recomputed after each box push-out so stacked collisions are handled correctly.

---

**[FIX] `Player.h` — `UpdateMoveState` signature mismatch**
`UpdateMoveState` was declared as `void UpdateMoveState(const Input& input)` but implemented and called as `void UpdateMoveState(const Input& input, float dt)`. MSVC rejected the compilation unit.

*Fix:* Header declaration updated to match the implementation.

---

### 3.5 World & Level System

**[FIX] Lamppost floats upward on each save/load cycle**
`SaveLevel` wrote the lamppost transform's `position.y` (the pole centre) as the `POS` field. `SpawnLamppost` expects the **base** (foot) position and adds `kPostHeight / 2 = 2 m` internally. Each save/load round-trip therefore pushed the lamp up by 2 m.

*Fix:* For `type == "lamppost"` entities, `SaveLevel` writes `saveY = position.y - scale.y * 0.5f` to recover the base position before the factory offset was applied.

---

**[FIX] Light entity duplicated on level load**
`SaveLevel` evaluated the `if (!rec.mesh && rec.light) → type = "light"` branch **before** the child-entity skip condition. The lamppost's child entity "LampostLight" (no mesh, has light) was therefore saved as a standalone `light` entry. On load, `SpawnLamppost` was called (recreating "LampostLight") and then the standalone `light` entry was also restored, producing two overlapping lights at the same position.

*Fix:* The child-entity skip check (`rec.name == "LampostLight" || rec.name == "AlarmLight"`) now runs **before** the type-detection block. Child entities are never written to the `.lvl` file; they are reconstructed by their parent factory on load.

---

**[FIX] Pickup item name corrupted after level load**
The load flush for `type == "pickup"` passed `cur.name` (the entity name, e.g. `"Pickup_Lockpick"`) directly as `it.name`, so the loaded item was named `"Pickup_Lockpick"` instead of `"Lockpick"`.

*Fix:* The load flush strips the `"Pickup_"` prefix from `cur.name` and restores underscore-to-space conversion (the same convention used when saving item names that contain spaces).

---


**[FIX] Door shrinks instead of pivoting**
`SpawnDoor`'s `onInteract` set `tr->scale.z = 0.05f` when open, making the door panel nearly invisible instead of rotating like a real door.

*Fix:* `DoorState` now stores the closed centre position and the pre-computed open centre position (hinge + rotated offset). On interact: if opening, `tr->rotation` is set to `glm::angleAxis(90°, {0,1,0})` and `tr->position` moves to the open centre; if closing, both are restored to their original values. The hinge is placed at the Z-negative end of the panel; a 90° CCW rotation around Y maps the panel's local +Z axis to global +X, so the door swings outward correctly.

---

### 3.6 Editor & UI

**[FIX] Level files saved to working directory**
`SaveLevel` and `LoadLevel` wrote files relative to the executable's working directory, with no clear organisation.

*Fix:* All level files are stored in a `Levels/` subfolder. `LevelPath()` strips any directory component the user types (preventing path-traversal) and always resolves to `Levels/<basename>`. The folder is created by `fs::create_directories("Levels")` on first save, and also by a CMake post-build step and a vcxproj `PostBuildEvent` so it exists before the engine runs.

---

**[FIX] Containers spawned from the level editor were always empty**
`SpawnCurrent` for the Container type called `Interaction::SpawnContainer(world, pos, {})` regardless of what was configured in the UI.

*Fix:* Added a **Container Contents** item editor to the Level Editor panel (visible when `Type == Container`). The editor shows a live list of pending items with per-row remove buttons, an add-item form (name + quantity), a Clear All button, and a "Clear list after placing" toggle. `SpawnCurrent` now passes `m_containerItems` to `SpawnContainer`. Items are round-tripped through the `.lvl` format as `ITEM <name> <quantity>` lines.

---

**[FIX] Container interaction opens nothing visible — interact prompt was the only feedback**
Interacting with a container printed items to the console but showed no UI.

*Fix:* Added `ContainerComponent { vector<Item> items; bool isOpen; }`. `SpawnContainer`'s `onInteract` sets `isOpen = true` instead of printing. `EditorUI::DrawContainerPopup` renders a centred 3×3 ImGui grid popup (amber/dark palette) when any container has `isOpen == true`. Each occupied slot shows the item name and quantity badge; clicking a slot removes that item and prints a grab message; **Grab All** takes everything at once. While the popup is open, the cursor is unlocked and player look/movement are suppressed so clicks register correctly.
