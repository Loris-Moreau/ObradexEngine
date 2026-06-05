# ObradexEngine

> **Pre-Alpha** 
>
> Visual style inspired by *Return of the Obra Dinn*
> 
> First-person immersive-sim engine inspired by *Deus Ex* & *Assassin's Creed Syndicate*

![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/Loris-Moreau/ObradexEngine)

---

## Features

- **Low-res rendering**: internal 320 × 180 buffer upscaled to window size via nearest-neighbour for a crisp pixel-art look
- **Obra Dinn post-processing**: Bayer 8 × 8 ordered dithering, 32-entry palette quantisation, vignette, CRT scanlines
- **FPS movement system**: walk / sprint / crouch / slide / jump / lean *(Thief-style peek)*
- **Interaction system**: doors, containers, pickups, alarm boxes with lambda callbacks
- **Trigger volumes**: AABB enter/exit events
- **Entity/component scene graph**: lightweight, no external ECS framework
- **Point light flickering**: multi-sine candle/fire simulation
- **Dear ImGui editor**: live post-process tuning, entity inspector, player stats, FPS sparkline

---

![Obradex Showcase](https://github.com/Loris-Moreau/loris-moreau.github.io/blob/main/Images/Showcase/BitEngine-4.webp)

## Full Documentation

See [`docs/DOCUMENTATION.md`](docs/DOCUMENTATION.md) for the complete subsystem reference, rendering pipeline breakdown, shader uniform tables, and extension guide.

See [`docs/RoadmapAndIssues.md`](docs/RoadmapAndIssues.md) for the Roadmap and current Issues of the project as well as past issues and how they were fixed.

## Quick Start

```bash
# 1. Fetch all third-party libraries (GLAD, ImGui, stb_image, GLM)
bash setup.sh

# 2. Build
mkdir build 
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)

# 3. Run
./bin/ObradexEngine
```

## Controls

| Input     | Action                     |
|-----------|----------------------------|
| ZQSD/WASD | Move                       |
| Mouse     | Look                       |
| Shift     | Sprint                     |
| Ctrl      | Crouch / Slide             |
| Space     | Jump                       |
| A / E     | Lean left / right          |
| F         | Interact                   |
| I         | Toggle inventory           |
| F1        | Toggle editor overlay      |
| Escape    | Close Inventory UI / Pause |

## Project Structure

```
ObradexEngine/
  bitEngine/     - All source files (flat layout)
    *.h / *.cpp  - Engine, Window, Input, Timer, Renderer, Shader,
                   Mesh, Camera, PostProcess, World, Player,
                   Interaction, InventorySystem, EditorUI, LevelEditor
    Shaders/     - world.vert / world.frag (GLSL 4.10)
    Levels/      - Saved .lvl scene files
  docs/          - DOCUMENTATION.md, RoadmapAndIssues.md, References.md
  glad/          - Pre-generated OpenGL 4.1 Core loader
  third_party/   - GLAD, ImGui, stb, GLM (populated by setup scripts)
  build/         - CMake / MSVC output
```

## Dependencies

| Library           | Role                                    | How obtained                           |
|-------------------|-----------------------------------------|----------------------------------------|
| OpenGL 4.1        | GPU rendering                           | System / driver                        |
| GLFW 3.3+         | Window + input                          | System pkg or CMake FetchContent       |
| GLM 0.9.9.8       | Math *(vectors, matrices, quaternions)* | `setup.sh`                             |
| GLAD              | OpenGL function loader                  | `setup.sh` *(via `glad2` pip package)* |
| Dear ImGui 1.90.1 | Editor UI                               | `setup.sh`                             |
| stb_image         | Texture loading                         | `setup.sh`                             |

## License

MIT: see [License](LICENSE).
