#pragma once

// Billboard.h - World-space quad that always faces the camera.
// Used for decals, signs, icons, and effect sprites.
// Rendered in a separate pass after opaque geometry (depth-test on,
// depth-write off, alpha blend enabled) so they composite correctly.

#include <string>
#include <glm/glm.hpp>

struct BillboardComponent
{
    unsigned int textureID  = 0;       // GPU texture handle (0 = white)
    glm::vec2    size       = {1.f, 1.f};  // World-space width x height (m)
    glm::vec4    tint       = {1.f, 1.f, 1.f, 1.f};  // RGBA multiplied with texture
    bool         fixedSize  = false;   // If true, size is in screen pixels (UI billboard)
    bool         axisLocked = false;   // If true, only rotates around world-Y (tree-style)
    std::string  texturePath;          // Stored so the level editor can save/reload
};
