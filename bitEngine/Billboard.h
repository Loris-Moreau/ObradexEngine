#pragma once

// Billboard.h - World-space quad that always faces the camera.
// Rendered after opaque geometry with depth-test on, depth-write off, alpha blend.

#include <string>
#include <glm/glm.hpp>

struct BillboardComponent
{
    unsigned int textureID   = 0;
    glm::vec2    size        = {1.f, 1.f};   // World-space width x height (m)
    glm::vec4    tint        = {1.f, 1.f, 1.f, 1.f};
    bool         axisLocked  = false;         // If true, rotates around world-Y only
    std::string  texturePath;
};
