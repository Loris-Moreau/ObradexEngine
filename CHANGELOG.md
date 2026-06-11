# ObradexEngine Changelog

## [Alpha 0.1.0]

### Added
- Main menu (Start Game / Quit) shown on launch before gameplay
- Pause menu (Continue / Main Menu / Quit) on Escape during gameplay
- Game Over screen when player health reaches zero; Retry respawns at spawn point
- Level Complete screen when player reaches the exit trigger
- Player health system: 100 HP, HUD bar, TakeDamage() API
- Alarm boxes deal 25 damage on proximity while armed; defuse to disarm
- Spawn point entity type (TYPE spawn in .lvl format); player respawns here on death
- Kill-plane at y = -20 m; falling off the level triggers death and respawn
- Fog: exponential distance fog in the world shader with density and colour tunable in the editor
- Point lights now collected from World LightComponent entities each frame (lampposts illuminate)
- AudioSystem stub; wired into engine lifecycle, documented for miniaudio integration
- ConfigLoader parses config.ini at startup (resolution, FPS, sensitivity, volume)
- Log file (log.txt) written alongside the executable via tee-buffer on cout/cerr
- Window icon: programmatic 16x16 amber ring generated at startup via glfwSetWindowIcon
- Slide direction locked at start of slide; releasing movement keys no longer stops it early
- Physical key labels in HUD prompts via glfwGetKeyName (layout-agnostic)
- Fog density and colour sliders in the Renderer editor tab
- Audio master volume slider in new Audio editor tab
- Designed demo level: Levels/alpha_demo.lvl (warehouse with platforms, crates, doors, loot)
- CHANGELOG.md and config.ini

### Fixed
- Point lights had no visual effect (LightComponent data never sent to renderer)
- Shaders loaded from relative path (fragile); world shader source embedded in binary
- Player fell forever off level edge
- Pause menu showed no UI
- No main menu; game launched directly into the test level
- Slide stopped dead when movement keys released
- MoveState::Vaulting appeared in HUD badge despite being unreachable
- strncpy_s used on Linux branch (MSVC extension, compile failure on GCC/Clang)
- Null dereference in HandleInteraction when GetRecord returned nullptr
- Circular include: World.cpp included Engine.h which includes World.h
- Deprecated ImGui KeysDown[] API replaced with IsKeyDown(ImGuiKey_Escape)
- Missing <algorithm> include in PostProcess.cpp

### Known Issues
- Crouch ceiling check not implemented: player can stand up through low ceilings
- Inventory UI column alignment needs a pass
- Vaulting state is reserved but cannot be entered
- Audio is a no-op stub until miniaudio is integrated
- No persistence of player health or inventory across sessions
