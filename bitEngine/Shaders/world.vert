// world.vert - World geometry vertex shader.
// Passes world-space position/normal for lighting and view depth for fog.
#version 410 core
layout(location=0)in vec3 a_Position;
layout(location=1)in vec3 a_Normal;
layout(location=2)in vec2 a_TexCoord;
uniform mat4 u_Model,u_View,u_Proj;
out vec3  v_WorldPos;
out vec3  v_Normal;
out vec2  v_TexCoord;
out float v_ViewDepth;
void main(){
    vec4 wp   = u_Model*vec4(a_Position,1.0);
    v_WorldPos= wp.xyz;
    v_Normal  = normalize(transpose(inverse(mat3(u_Model)))*a_Normal);
    v_TexCoord= a_TexCoord;
    vec4 vp   = u_View*wp;
    v_ViewDepth = -vp.z;
    gl_Position = u_Proj*vp;
}
