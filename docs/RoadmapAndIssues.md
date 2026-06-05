# Obradex Engine - Roadmap, Issues & Fix Log

---

## 1. Known Limitations & Roadmap

| Area                                               | Current State                                                                               | Planned                                                                                                                                                                    |
|----------------------------------------------------|---------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Collision                                          | AABB box collision against all solid `CollisionComponent` entities + floor plane at y = 0   | Switch to OOBB Collision                                                                                                                                                   |
| Physics                                            | Not Implemented                                                                             | Would be nice                                                                                                                                                              |
| Shadows                                            | None                                                                                        | Single directional shadow map                                                                                                                                              |
| Audio                                              | Not implemented                                                                             | OpenAL-Soft or miniaudio integration                                                                                                                                       |
| Level loading                                      | Runtime `.lvl` save/load via ImGui Level Editor tab (plain-text format, `Levels/` folder)   | Additional entity types; level streaming                                                                                                                                   |
| Animation                                          | None                                                                                        | Skeletal animation via Assimp                                                                                                                                              |
| Vaulting                                           | State stub only                                                                             | Auto-climb ledges ≤ 2 m high                                                                                                                                               |
| Texture system                                     | Stub uniforms in world shader                                                               | Full material system with diffuse + normal maps                                                                                                                            |
| Icons for container items                          | None                                                                                        | Pixel-art icons per item type, rendered in the 3×3 container grid                                                                                                          |
| Inventory                                          | Data structure + Deus Ex Mankind Divided style UI implemented (I key); routes grabbed items | Weapons, ammo, and consumable design; equipment slots; weight system                                                                                                       |
| Main Menu                                          | Not Implemented                                                                             | Continue, Start, Level Select, Settings, Quit                                                                                                                              |
| Pause Menu                                         | Not Implemented                                                                             | Continue, Restart Level, Settings, Main Menu, Quit                                                                                                                         |
| AI / stealth                                       | Not implemented                                                                             | Visibility cones + noise detection + random patrol movement                                                                                                                |
| AI Navigation                                      | Not Implemented                                                                             | fastest path then smooth it, patrol & random roam then chase or run away, use weights as well, need to make a nav manager                                                  |
| Weapons                                            | Not implemented                                                                             | Blunt, slash, ranged *(pistol, shotgun, sniper, semi-auto)*                                                                                                                |
| Ammo system                                        | Not implemented *(requires weapons)*                                                        | Per-weapon pool: 9 mm, 12 ga, 7.62 mm, 5.56 mm                                                                                                                             |
| Health system                                      | Not implemented                                                                             | Health bar, death screen, level reload                                                                                                                                     |
| Networking                                         | Not planned                                                                                 | Revisit if the project ships on Steam                                                                                                                                      |
| Meshes                                             | Cube, Plane, Sphere placeable from Level Editor                                             | OBJ/glTF import via Assimp                                                                                                                                                 |
| VFX and Materials                                  | None                                                                                        | VFX editor similar to Unreal / Unity                                                                                                                                       |
| HDA Bridge                                         | Not Implemented                                                                             | Being able to import and use Houdini HDAs                                                                                                                                  |
| Spline Editor                                      | Thinking about it                                                                           |                                                                                                                                                                            |
| Debug Views                                        | Not Implemented                                                                             | UV, Shader Complexity, Lit(unpixelated), Unlit(unpixelated), Collisions, Lights(depends on what is used), Raytracing complexity(if used), Shadow Maps, Reflections(if any) |
| map loading by sections for larger maps            | Not Implemented                                                                             | Maps, Assets, basically if not in camera fullstrum don't render it(maybe keep the collisions to be safe)                                                                   |
| a way to build a game without the editor utilities | Not Implemented                                                                             | A portable .exe                                                                                                                                                            |

---

## 2. Open Issues

| Bug                                                             | Status | Notes                                                                                                                                                                                                                                                                                |
|-----------------------------------------------------------------|--------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| Crouch does not properly disable stand-up when ceiling is above | Open   | The AABB correctly shrinks to `crouchHeight = 0.85 m` when crouching. However there is no headroom check when releasing Ctrl - the player can clip through a low ceiling by standing up inside it. A sweep test upward before allowing the state transition to `Standing` is needed. |
| Inventory UI needs some polish *(text not aligned with titles)* | Open   | Column alignment pass needed.                                                                                                                                                                                                                                                        |

---

## 3. Fixed

All fixes are listed chronologically. Where the same root cause was addressed more than once, entries are merged.

---

### 3.1 Build & Setup

**[FIX] GLAD not found / stub files not detected**
`setup.sh` used `wc -l` to distinguish a real GLAD file from a stub. On Windows Git Bash `wc -l` returns `"15778 "` with trailing whitespace, making the arithmetic comparison `[ "$lines" -gt 100 ]` silently fail as a string test, leaving `ALREADY_REAL=false`. The fallback then tried to regenerate GLAD and failed, leaving the file missing.

*Fix:* Detection changed to `grep -q "GLAPI" "$GLAD_H"` - the string `GLAPI` appears 3 281 times in a real generated file and never in a stub. Real GLAD files (15 778 lines for OpenGL 4.1 Core) are now generated in the sandbox and pre-bundled in the zip so no Python or pip is required on the developer machine. `setup.sh` only checks; it does not attempt generation.

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

**[FIX] `unique_ptr` with incomplete type - MSVC C2027 / C2338**

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

**[FIX] GLSL multidimensional array error - dither shader crash at startup**
The post-process shader declared the Bayer dither matrix as `float[8][8]`, which GLSL forbids (no multidimensional arrays). NVIDIA's driver rejected the fragment shader and the engine exited immediately after window creation.

*Fix:* Flattened to `float[64]` with element access changed from `B[row][col]` to `B[row * 8 + col]`. The dither values and distribution are unchanged.

---

### 3.3 Input System

**[FIX] Keyboard input completely unresponsive - only mouse worked**
`glfwPollEvents()` was called at the **end** of the game loop, after `Input::Update()` had already read `glfwGetKey()`. Keyboard state (`WM_KEYDOWN` / `WM_KEYUP`) only enters GLFW's internal state after `glfwPollEvents()`; reading before polling always returned last frame's stale values. Mouse worked because `glfwGetCursorPos()` with `GLFW_CURSOR_DISABLED` (raw input mode on Windows) bypasses the message queue and reads accumulated delta directly from the driver.

*Fix:* `Window::PollEvents()` moved to the **top** of the game loop, before `ProcessInput()` and `Input::Update()`.

---

**[FIX] F1 / Escape / Interact key continuously toggled - `IsKeyJustPressed` toggled every frame a key was held**
The input double-buffer used a copy-then-overwrite pattern: `m_keys[prev] = m_keys[cur]`, then `m_keys[cur] = glfwGetKey(...)`, then `++m_frame`. After `++m_frame`, the query functions read `m_keys[m_frame & 1]`, which had just flipped to the opposite buffer - the one that had just been overwritten with the stale two-frames-ago state, not the freshly polled state. This made `IsKeyJustPressed` see PRESS vs RELEASE every single frame a key was held.

*Fix:* Switched to **flip-then-overwrite**: `m_cur = 1 - m_cur` first, then only `m_keys[m_cur][i] = glfwGetKey(...)`. The opposite slot is never touched during that frame, so it naturally retains the previous frame's snapshot. No copy step is needed.

---

**[FIX] WASD keys used on AZERTY keyboard - movement and sprint did not work**
`UpdateMoveState`, `HandleMovement`, and the `moving` check in `UpdateMoveState` referenced `Key::W` (87) and `Key::A` (65), which correspond to QWERTY positions. On an AZERTY keyboard these keys are physically at different positions and are never pressed during normal play.

*Fix:* Movement remapped to AZERTY layout: `Key::Z` (90) = forward, `Key::Q` (81) = left, `Key::S` (83) = back, `Key::D` (68) = right, `Key::A` (65) = lean left, `Key::E` (69) = lean right. A single `constexpr Key INTERACT_KEY = Key::F` in `Input.h` is the sole source of truth; all world prompts use `Input::GetKeyName(INTERACT_KEY)` to build their text dynamically.

---

**[FIX] Container closes game instead of closing popup when Escape is pressed**
`Engine::ProcessInput` handled Escape before checking whether a container was open, so pressing Escape during looting paused the game rather than dismissing the popup.

*Fix:* `Engine::ProcessInput` now calls `World::CloseOpenContainer()` and returns early when `HasOpenContainer()` is true. `World::CloseOpenContainer()` sets `isOpen = false` on the first open container. The cursor is re-locked by `EditorUI::Render` on the next frame when it detects the container closed.

---

**[FIX] Grab All closes container without dismissing popup (container stays visible as empty)**
After `Grab All` cleared the item list the popup remained open, showing "Container is empty." The user had to click Close separately.

*Fix:* The Grab All button now sets `windowOpen = false` immediately after clearing items. The popup dismissal path at the bottom of `DrawContainerPopup` (which sets `container->isOpen = false`) fires on the same frame.

---

### 3.4 Physics & Collision

**[FIX] Crash in `Mesh::Draw()` - vector pointer invalidation**
`EntityRecord` stored raw pointers into `std::vector` component storage (e.g. `rec.mesh = &m_meshes.back()`). Each `push_back` that caused a reallocation invalidated all previously stored pointers. By the first render frame, most entity mesh and transform pointers pointed at freed memory; calling `glBindVertexArray` through a dangling `Mesh*` crashed the driver.

*Fix:* All six component vectors are pre-reserved to 1 024 entries in `World::Init()` and `World::ClearLevel()` before any entities are created. Reallocation cannot occur as long as the scene stays within that budget.

---

**[FIX] No AABB collision - player walked through all objects**
Collision was limited to a floor plane check (`m_position.y < 0`). All solid geometry was passable.

*Fix:* Added `CollisionComponent { glm::vec3 halfExtents; bool solid; }` to the entity/component system. `Player::ResolveCollision` runs a two-pass resolution each frame: (1) floor plane at y = 0, (2) all active `CollisionComponent` entities using minimum-penetration-axis push-out. The player AABB (`kPlayerHalfW = 0.30 m`, `kPlayerHeight = 1.80 m` standing, `crouchHeight = 0.85 m` crouching) is recomputed after each box push-out so stacked collisions are handled correctly.

---

**[FIX] `Player.h` - `UpdateMoveState` signature mismatch**
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

**[FIX] Container 3×3 grid silently drops items beyond index 8**
`SpawnContainer` accepted an unbounded `std::vector<Item>`, but `DrawContainerPopup` only renders a 3×3 grid (9 slots). Items at index ≥ 9 were stored in the component but never displayed or grabbable.

*Fix:* `SpawnContainer` now calls `items.resize(9)` (no-op if ≤ 9 items) before moving into the `ContainerComponent`. The cap is enforced at spawn time so the data is always consistent with what the UI renders.

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

**[FIX] Container interaction opens nothing visible - interact prompt was the only feedback**
Interacting with a container printed items to the console but showed no UI.

*Fix:* Added `ContainerComponent { vector<Item> items; bool isOpen; }`. `SpawnContainer`'s `onInteract` sets `isOpen = true` instead of printing. `EditorUI::DrawContainerPopup` renders a centred 3×3 ImGui grid popup (amber/dark palette) when any container has `isOpen == true`. Each occupied slot shows the item name and quantity badge; clicking a slot removes that item and routes it into the inventory; **Grab All** takes everything at once and closes the popup. While the popup is open, the cursor is unlocked and player look/movement are suppressed so clicks register correctly.

---

**[FIX] Level editor required exact filename to be typed manually**
The filename input field had no file picker; the user had to type `level.lvl` exactly or the load would fail with a misleading error.

*Fix:* A **Browse** button opens the OS native file dialog (`GetOpenFileNameA` on Windows, `zenity` on Linux) filtered to `*.lvl` files, defaulting to the `Levels/` directory. Selecting a file populates the filename field with the basename only, which `LevelPath()` then resolves to `Levels/<basename>` as usual. Typing the name manually continues to work.

---

**[FIX] Inventory items printed to console only - no UI or data structure**
Grabbed items (container slots, Grab All, standalone pickups) called `std::cout` and were discarded with no persistent storage.

*Fix:* Added `InventorySystem` (`InventorySystem.h/.cpp`). It stores `InventoryEntry` values (item + stack count) and stacks by name on `AddItem`. The Deus Ex: Mankind Divided-style overlay (`DrawUI`) is drawn at native resolution over the game view, toggled by the **I** key. Container grabs and standalone pickup `onInteract` callbacks now call `engine.GetInventory().AddItem(it)` instead of printing. The cursor is unlocked while the inventory is open.

---

### 3.7 Camera, Inventory & Door (batch fix)

**[FIX] Camera continues to move when inventory is open**
`Player::Update` checked `HasOpenContainer()` to block mouse look and movement, but inventory open state was not checked. Inventory now sets `EngineState::Paused` when opened (same as pressing Escape), which stops the fixed-step `Update` loop entirely. Camera movement is blocked as a side-effect of the engine not ticking rather than requiring a per-system guard.

**[FIX] Escape key does not close inventory**
`Engine::ProcessInput` Escape handling only checked for open containers, then fell through to the pause toggle. Escape now checks containers first (priority 1), then inventory (priority 2, closes it and resumes `EngineState::Running`), then the normal pause toggle (priority 3).

**[FIX] Opening inventory does not pause the game**
`Toggle()` + `SetCursorLocked(false)` was the only effect of pressing I. Physics and movement continued running (head bob, gravity, sliding). The I handler now sets `m_state = EngineState::Paused` on open and restores `EngineState::Running` on close, consistent with how Escape-pause works.

**[FIX] Head bob continues when walking then opening a container**
`UpdateCameraHeight` called `camera.UpdateHeadBob(m_speed, dt)` unconditionally. `m_speed` retained its last value from the previous frame when `HandleMovement` was blocked by the container popup, so the bob continued advancing at the last walking speed. Fixed by passing `suppressBob` to `UpdateCameraHeight` - when a container is open, speed `0` is passed so the bob damps out via the existing decay path in `UpdateHeadBob`.

**[FIX] Lean tilt angle changes while looking around**
`Camera::GetView()` built the view matrix via `glm::lookAt(eye, eye + m_forward, m_up)` where `m_up` is the camera's pitched-up vector (from `cross(right, forward)`). At non-zero pitch `m_up` tilts, introducing an implicit roll into the base view matrix that compounded with the explicit lean roll. Fixed by passing `glm::vec3(0,1,0)` (world-up) to `lookAt` so the base view matrix is always roll-free, and the lean rotation post-multiplied with axis `(0,0,1)` is purely a screen-space roll at all pitch angles.

**[FIX] Door collision is one-way (player blocked after door opens)**
Two independent sub-bugs:

1. `ResolveCollision` computed the door's AABB as `halfExtents * scale` without applying `transform->rotation`. When the door opened (90° Y rotation) the physical AABB remained in the closed orientation, blocking the doorway even though the mesh had swung away. Fixed by expanding the OBB to an enclosing AABB using the standard formula: `halfAABB[i] = Σ |R[col][i]| * localHalf[col]` where R is the rotation matrix.

2. Even with a correctly rotated AABB the open door still partially blocked the opening because the panel now occupies the wall beside the door rather than the doorway. Fixed by setting `collision->solid = false` in the `onInteract` open branch and `solid = true` in the close branch. The AABB rotation fix is retained for any other rotating solid entities.

---

### 3.8 Camera Lean & Window Maximize

**[FIX] Lean tilt angle changes while looking around**

Root cause: `glm::rotate(mat4 M, angle, axis)` applies the rotation in the coordinate space that `M` maps *from* - world space. The axis `vec3(0, 0, 1)` is world +Z. As the player yaws, world +Z maps to a different direction on screen, so the roll axis precesses and the horizon tilt visibly shifts with yaw.

*Fix (`Camera.cpp`):* Build the roll rotation on `glm::mat4(1.f)` (identity) and right-multiply it onto the view matrix: `view = view * roll`. Right-multiplying a rotation onto a view matrix applies it in *view space* (the space the view maps TO). View-space Z is the permanent into-screen axis at all yaw and pitch angles, so the tilt is a stable screen-space roll that never precesses. The `glm::lookAt` call continues to use world-up `(0,1,0)` so the base matrix is always roll-free.

---

**[FIX] Window does not properly maximise - image stretches to fill window**

`PostProcess::Apply` called `glViewport(0, 0, windowW, windowH)` unconditionally, stretching the 320×180 render to fill any window size. Maximising to a size with a different aspect ratio (or any size that isn't an exact integer multiple of 320×180) produced visible distortion.

Additionally `Window::FramebufferSizeCallback` called `glViewport(0, 0, w, h)` on every OS resize event, which could fire mid-frame and override the low-res capture viewport set by `BeginCapture`.

*Fix (`PostProcess.cpp`, `Window.cpp`):*
- `Apply` now computes `scale = min(windowW / renderW, windowH / renderH)`, produces a centred viewport of `(renderW × scale) × (renderH × scale)`, clears the full window to black first (painting the letterbox bars), then blits into only the centred region. The image is never stretched at any window size.
- `FramebufferSizeCallback` no longer calls `glViewport` - it only updates `m_width` / `m_height`. All viewport management is owned by `PostProcess::Apply` (blit) and `PostProcess::BeginCapture` (low-res scene render).

---

### 3.9 Lean & Maximize (corrected)

Previous §3.8 entries described the intended fix; the actual bugs in the shipped code required the following additional corrections.

**[FIX] Lean inverted, precesses with yaw, lost while moving, causes diagonal drift**

Four sub-bugs:

**a) Precesses with yaw** - `glm::rotate(M, angle, axis)` rotates in world space. `vec3(0,0,1)` is world +Z; as the player yaws it maps to different screen directions, so the roll axis drifted. Fix: `view = view * glm::rotate(I, angle, Z)` - right-multiplying applies the rotation in view space where Z is permanently the into-screen axis.

**b) Inverted direction** - The sign was `-m_lean` in `GetView`, which made A (target −12) produce a +12° roll = right-side-up = lean right. Changed to `+m_lean`: A → negative roll → left-side-up → lean left ✓.

**c) E lean lost while moving** - E was gated on `glm::length(moveDir) < 0.001f`. Pressing any movement key zeroed the lean immediately. Restriction removed - lean works freely during walk/crouch/air.

**d) Bob offsets using pitched camera vectors** - `GetView` computed eye position as `m_position + m_up*bobY + m_right*bobX`. `m_up` has a forward component when pitched, making the eye drift forward/backward during bob. Changed to `worldUp = {0,1,0}` for bobY and `flatRight = normalize(cross(m_forward, worldUp))` for bobX.

---

**[FIX] Maximize black bars not painted - GL_SCISSOR_TEST left enabled by ImGui**

ImGui leaves `GL_SCISSOR_TEST` enabled with its own scissor rect after `ImGui_ImplOpenGL3_RenderDrawData`. On the next frame, the `glClear` at the start of `PostProcess::Apply` was clipped to ImGui's last scissor region, so the letterbox bar areas were never cleared and retained stale pixel content. Fix: `glDisable(GL_SCISSOR_TEST)` before the full-window clear in `Apply`.

---

### 3.10 Lean distortion & maximize bottom-left (root cause fixes)

**[FIX] Lean distorts geometry when far from the world origin / wrong direction per facing**

Root cause: `view * roll` right-multiplies the roll rotation onto the **full** view matrix `V = R * T(-eye)`. The rotation axis passes through the world origin, not through the eye. When the eye is far from `(0,0,0)` the eye position is swung through a visible arc on every lean frame - causing the distortion reported far from origin. The direction also varied by facing because the axis `(0,0,1)` in the space `view` maps from is world +Z, which points in a different screen direction at different yaw angles.

*Fix (`Camera.cpp`):* Decompose the view matrix construction explicitly as `V = R * T(-eye)` and apply the lean roll **to R before T is appended**. When `T(-eye)` has not been applied yet, the rotation origin is the eye. `glm::rotate(R, lean, Z)` rotates the orientation matrix around its local Z axis (which is the camera's into-screen axis regardless of yaw/pitch), and the eye translation is applied afterward - so the axis always passes through the eye at any world position. The orientation matrix is built from the same `flatRight`, `up_view`, `m_forward` basis vectors that `glm::lookAt` would produce, giving identical results without a lean.

---

**[FIX] Maximize keeps render at bottom-left of window**

Root cause: `GetWidth()` / `GetHeight()` returned the cached `m_width` / `m_height` members that are only updated by `FramebufferSizeCallback`. On Windows, GLFW fires the framebuffer-size callback correctly, but there are edge cases (minimize then maximize, certain DPI-scaling configurations, window state transitions during the same `PollEvents` call where a frame is already mid-render) where the cached value is stale. The letterbox viewport `vpX = (windowW - vpW) / 2` then receives the original window dimensions rather than the maximised ones, producing `vpX = 0, vpY = 0` - image at bottom-left with black bars only on the right and top.

*Fix (`Window.cpp`, `Window.h`):* `GetWidth()`, `GetHeight()`, and `GetAspect()` now call `glfwGetFramebufferSize()` directly on every query rather than reading cached members. `glfwGetFramebufferSize` always returns the current framebuffer dimensions synchronously with no caching, so the letterbox calculation is never based on a stale size.

---

### 3.11 Lean axis & jump velocity

**[FIX] Lean tilt still changes with look direction**

Root cause (final): `glm::rotate(R, angle, vec3(0,0,1))` - even with R being the pure rotation matrix built from the yaw/pitch basis vectors - applies the rotation around `{0,0,1}` **in the space R maps from**, which is world space. World +Z is only the camera's into-screen axis when the player faces -Z (yaw 0°). At any other yaw the world-Z axis is angled relative to the screen, so the roll axis precesses.

*Fix (`Camera.cpp`):* Pass `-m_forward` as the rotation axis instead of `{0,0,1}`. `-m_forward` is always the camera's actual forward / into-screen axis in world space at any yaw and pitch. `glm::rotate(R, lean, -m_forward)` therefore always rolls around the true screen-depth axis - the tilt is a pure screen-space roll identical at every look direction. The translation is still appended last (`R * T`) so the roll origin is the eye, not the world origin.

---

**[FIX] Jump does not conserve horizontal velocity**

Root cause: `HandleMovement` set `m_velocity.x = moveDir * speed` and `m_velocity.z = moveDir * speed` unconditionally whenever `m_state != Sliding`. This included `InAir`, so the frame after a jump the horizontal velocity was overwritten with the current WASD input times walk speed - sprint-jump momentum was lost instantly.

*Fix (`Player.cpp`):* Split the velocity assignment on `m_onGround`:
- **On ground**: snap directly to `moveDir * targetSpeed` (unchanged - responsive ground movement).
- **In air**: apply input as an acceleration nudge (`kAirControl = 4 m/s²`) so the player can steer during a jump but the horizontal speed set at jump-time is preserved. A cap at `targetSpeed` prevents unlimited air-strafe acceleration. A sprint-jump now carries the sprint velocity for the full arc; a walk-jump carries walk velocity.

---

### 3.12 Jump velocity cap fix

**[FIX] Jump does not conserve sprint or walk velocity (cap was wrong)**

The §3.11 fix preserved momentum in the air but the cap `if (hspd > targetSpeed)` read `targetSpeed` from the current state switch, which falls through to `walkSpeed = 2.25 m/s` for `InAir`. On the very first air frame the sprint velocity (8 m/s) was clamped to 2.25 m/s - effectively the same bug.

*Fix (`Player.cpp`, `Player.h`):* Added `m_airSpeedCap` (float member). At the exact frame `Space` is pressed, `m_airSpeedCap` is set to the current horizontal speed (`length(velocity.xz)`) - this captures sprint speed after a sprint-jump and walk speed after a walk-jump. A floor of `walkSpeed` ensures standing-still jumps still allow basic air steering. The in-air cap now uses `m_airSpeedCap` instead of `targetSpeed`, so the horizontal velocity is fully preserved for the entire arc.

---

### 3.13 Jump reliability, velocity retention & editor exposure

**[FIX] Spacebar sometimes requires multiple presses to jump**

Root cause: `Player::Update` runs inside a fixed-timestep accumulator and can execute 2–3 sub-steps per real frame. `IsKeyJustPressed` returns `true` throughout the entire real frame (the snapshot doesn't change between sub-steps). On the first sub-step, the jump fires and sets `m_onGround = false`. Then `ApplyGravity` moves the player slightly upward, but `ResolveCollision` (which runs after gravity) can snap `m_position.y` back to 0 and reset `m_onGround = true` on the same sub-step if the upward displacement is small. On subsequent sub-steps of the same frame, `IsKeyJustPressed` is still true and `m_onGround` is true again - the jump fires a second time, doubling the vertical velocity. The resulting over-launch ends abnormally, making the jump feel unreliable.

*Fix (`Player.cpp`, `Player.h`):* Added `bool m_jumpConsumed`. Set to `true` the frame Space is first processed; cleared when `m_onGround` becomes true (floor landing, box landing) and when `IsKeyHeld(Space)` is false (key released). The jump condition now requires `!m_jumpConsumed`, so it fires exactly once per key press regardless of how many physics sub-steps run.

---

**[FIX / FEATURE] Jump conserves only 85% of horizontal velocity**

*Fix (`Player.cpp`):* At jump time, `m_velocity.x` and `m_velocity.z` are multiplied by `m_stats.jumpVelocityRetain` (default `0.85f`) before setting the vertical impulse and computing `m_airSpeedCap`. Sprint-jumps carry 85% of sprint speed; walk-jumps carry 85% of walk speed.

---

**[FEATURE] Air control and jump velocity retention exposed in editor**

Added `airControl` (m/s² horizontal nudge in air) and `jumpVelocityRetain` (0–1 fraction) to `PlayerStats`. Both appear as sliders in the **Player → Air Movement** section of the F1 editor with tooltips. Presets (Base, SpeedRun) updated to include the new fields.

---

### 3.14 Pre-alpha code review fixes

**[FIX] `strncpy_s` on Linux/macOS**
`LevelEditor.cpp` used `strncpy_s` (MSVC extension) on both branches of the `#ifdef _WIN32` Browse dialog. The non-Windows branch would fail to compile under GCC or Clang. Fixed by using `std::strncpy` with an explicit null-terminator on the non-Windows branch.

**[FIX] Null dereference in `HandleInteraction`**
`Player::HandleInteraction` called `world.GetRecord(near)->interactable` without checking whether `GetRecord` returned `nullptr`. Although `FindNearestInteractable` only returns IDs that exist at the time of the call, entity destruction could theoretically invalidate the ID before `GetRecord` runs. Added an explicit null check before dereferencing.

**[FIX] Circular/unused includes in `World.cpp`**
`World.cpp` included `Engine.h` and `InventorySystem.h`. Neither is used in the file. `Engine.h` includes `World.h`, creating a circular include chain. Both removed.

**[FIX] Deprecated ImGui `KeysDown` API in `EditorUI.cpp`**
`escIO.KeysDown[256]` used the `KeysDown[]` array removed in ImGui 1.87. Replaced with `ImGui::IsKeyDown(ImGuiKey_Escape)`.

**[FIX] Missing `<algorithm>` include in `PostProcess.cpp`**
`std::min` was used in `Apply` without an explicit `<algorithm>` include. The call compiled only because a transitive include pulled it in. Added the explicit include.

**[NOTE] `SpawnAlarm` `onTrigger` parameter is reserved but never called**
The alarm has no event system to call `onTrigger`. The parameter is kept for API forward-compatibility but is annotated as reserved in the source.

**[FIX] Section 3.7 door fix log contradicts actual implementation**
Section 3.7 described the door fix as setting `collision->solid = false` on open. The actual shipped fix relies on OBB-to-AABB expansion in `ResolveCollision` and does *not* toggle `solid`. The section 3.7 entry described an earlier iteration that was subsequently replaced. This log entry is retained for historical context; the definitive implementation is the OBB expansion described in section 3.10.
