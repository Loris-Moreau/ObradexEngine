// ============================================================
//  world.vert  -  World Geometry Vertex Shader
// ============================================================
#version 410 core

// ── Per-vertex attributes ────────────────────────────────────
layout(location = 0) in vec3 a_Position;  // Object-space position
layout(location = 1) in vec3 a_Normal;    // Object-space normal
layout(location = 2) in vec2 a_TexCoord;  // UV coordinates

// ── Uniforms ─────────────────────────────────────────────────
uniform mat4 u_Model;   // Object-to-world transform
uniform mat4 u_View;    // World-to-camera transform
uniform mat4 u_Proj;    // Camera-to-clip transform

// ── Outputs to fragment shader ────────────────────────────────
out vec3 v_WorldPos;    // World-space fragment position (for lighting)
out vec3 v_Normal;      // World-space normal
out vec2 v_TexCoord;

void main()
{
    vec4 worldPos   = u_Model * vec4(a_Position, 1.0);
    v_WorldPos      = worldPos.xyz;

    // Normal matrix: transpose of the inverse of the upper-left 3×3
    // Handles non-uniform scaling correctly
    mat3 normalMat  = transpose(inverse(mat3(u_Model)));
    v_Normal        = normalize(normalMat * a_Normal);

    v_TexCoord      = a_TexCoord;
    gl_Position     = u_Proj * u_View * worldPos;
}
