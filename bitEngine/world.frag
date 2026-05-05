// ============================================================
//  world.frag  —  World Geometry Fragment Shader
// ============================================================
//  Implements a simple Blinn-Phong lighting model tuned for
//  the Obra Dinn aesthetic: low-dynamic-range, hard shadows,
//  strongly directional light with almost no ambient fill.
// ============================================================

#version 410 core

// ── Inputs from vertex shader ─────────────────────────────────
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;

out vec4 FragColor;

// ── Material uniforms ─────────────────────────────────────────
uniform sampler2D u_AlbedoTex;  // Albedo/diffuse texture
uniform vec3      u_AlbedoColour;  // Tint (multiplied with texture)
uniform float     u_Specular;      // Specular intensity 0..1
uniform float     u_Roughness;     // Roughness 0 (mirror) .. 1 (matte)
uniform bool      u_HasTexture;    // Use texture or flat colour

// ── Lighting uniforms ─────────────────────────────────────────
uniform vec3 u_CamPos;
uniform vec3 u_SunDir;    // Normalised, pointing toward the light
uniform vec3 u_SunColour;
uniform vec3 u_Ambient;

// Point lights (up to 8)
struct PointLight {
    vec3  position;
    vec3  colour;
    float radius;
    float intensity;
};
uniform PointLight u_PointLights[8];
uniform int        u_PointLightCount;

// ── Helpers ───────────────────────────────────────────────────

/// Blinn-Phong specular term.
float Specular(vec3 N, vec3 L, vec3 V, float roughness)
{
    float shininess = mix(512.0, 2.0, roughness);
    vec3  H         = normalize(L + V);
    return pow(max(dot(N, H), 0.0), shininess);
}

/// Inverse-square attenuation clamped to [radius] metres.
float Attenuation(float dist, float radius)
{
    float norm = clamp(1.0 - (dist / radius), 0.0, 1.0);
    return norm * norm;  // Quadratic falloff
}

void main()
{
    // ── Albedo ────────────────────────────────────────────────
    vec3 albedo = u_HasTexture
        ? texture(u_AlbedoTex, v_TexCoord).rgb * u_AlbedoColour
        : u_AlbedoColour;

    vec3 N = normalize(v_Normal);
    vec3 V = normalize(u_CamPos - v_WorldPos);

    // ── Directional (sun/moon) light ──────────────────────────
    vec3  L_sun   = normalize(-u_SunDir);   // Shader convention: toward light
    float NdotL   = max(dot(N, L_sun), 0.0);
    vec3  diffuse = u_SunColour * NdotL;
    vec3  spec    = u_SunColour * Specular(N, L_sun, V, u_Roughness) * u_Specular;

    // ── Point lights ──────────────────────────────────────────
    for (int i = 0; i < u_PointLightCount; ++i)
    {
        vec3  toLight = u_PointLights[i].position - v_WorldPos;
        float dist    = length(toLight);
        vec3  L       = toLight / dist;
        float atten   = Attenuation(dist, u_PointLights[i].radius);
        float NdL     = max(dot(N, L), 0.0);

        diffuse += u_PointLights[i].colour * NdL * atten * u_PointLights[i].intensity;
        spec    += u_PointLights[i].colour * Specular(N, L, V, u_Roughness)
                    * u_Specular * atten * u_PointLights[i].intensity;
    }

    // ── Combine ───────────────────────────────────────────────
    vec3 colour = albedo * (u_Ambient + diffuse) + spec;

    // Tone-map: simple Reinhard to keep values in [0,1]
    // (The post-process pass does the real look-dev, so just prevent blowout)
    colour = colour / (colour + vec3(1.0));

    FragColor = vec4(colour, 1.0);
}
