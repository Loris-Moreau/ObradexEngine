# ObradexEngine

> A first-person immersive-sim engine with an **8-bit / *Return of the Obra Dinn*** aesthetic.
> Built in **C++17** with **OpenGL 4.1**, **Dear ImGui**, **GLFW**, and **GLM**.

```
┌─────────────────────────────────────────────────┐
│  ▓▓▒▒░░  OBRADEX ENGINE  ░░▒▒▓▓                │
│                                                 │
│  Deus Ex movement · Assassin's Creed parkour   │
│  Obra Dinn dithering · 8-bit pixel palette     │
│  First-person · Immersive sim · OpenGL 4.1     │
└─────────────────────────────────────────────────┘
```

## Quickstart

```bash
# 1. Bootstrap dependencies (ImGui, stb_image, GLFW check)[DOCUMENTATION.md](DOCUMENTATION.md)
chmod +x setup.sh && ./setup.sh

# 2. Generate GLAD for OpenGL 4.1 Core at https://glad.dav1d.de/
#    Copy glad.h → third_party/glad/include/glad/glad.h
#    Copy glad.c → third_party/glad/src/glad.c

# 3. Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# 4. Run
./bin/ObradexEngine
```

## Controls

| Key | Action |
|---|---|
| `W/A/S/D` | Move |
| `Mouse` | Look |
| `LShift` | Sprint |
| `LCtrl` | Crouch |
| `LCtrl` (while sprinting) | Slide |
| `Space` | Jump |
| `Q` | Lean left |
| `E` | Lean right / Interact |
| `F1` | Toggle editor overlay |
| `Escape` | Pause |

## Features

- **Aesthetic pipeline** — low-res FBO (320×180), Bayer 8×8 ordered dithering, 32-colour palette quantisation, vignette, CRT scanlines
- **Obra Dinn mode** — one-click monochrome + 1-bit dither preset
- **Movement FSM** — standing, crouching, sprinting, sliding, in-air states
- **Lean system** — left/right peek, smoothly interpolated
- **Head-bob** — Lissajous figure-8 pattern, speed-scaled
- **Interaction system** — doors, containers, pickups, alarms
- **AABB trigger zones** — enter/exit callbacks
- **Point lights with flicker** — candle/fire simulation
- **Runtime editor** — F1 ImGui panel: post-process tweaking, entity inspector, player stats, FPS graph

## Documentation

See [`docs/DOCUMENTATION.md`](docs/DOCUMENTATION.md) for the full developer reference.

## Dependencies

| Library | Purpose |
|---|---|
| OpenGL 4.1 | Rendering |
| GLFW 3.3+ | Window + input events |
| GLAD (gl 4.1 core) | OpenGL function loader |
| GLM 0.9.9.8 | Math |
| Dear ImGui 1.90.1 | Editor UI + HUD |
| stb_image | PNG/JPEG loading |
