// world.frag - Blinn-Phong lighting with up to 8 point lights and exponential fog.
#version 410 core
in vec3  v_WorldPos;
in vec3  v_Normal;
in vec2  v_TexCoord;
in float v_ViewDepth;
out vec4 FragColor;
uniform sampler2D u_AlbedoTex;
uniform vec3  u_AlbedoColour;
uniform float u_Specular,u_Roughness;
uniform int   u_HasTexture;
uniform vec2  u_UVScale;
uniform vec3  u_CamPos,u_SunDir,u_SunColour,u_Ambient;
struct PL{vec3 position,colour;float radius,intensity;};
uniform PL  u_PointLights[8];
uniform int u_PointLightCount;
uniform float u_FogDensity;
uniform vec3  u_FogColour;
float BP(vec3 N,vec3 L,vec3 V,float rough){
    float sh=mix(512.0,2.0,rough);
    return pow(max(dot(N,normalize(L+V)),0.0),sh);
}
float Att(float d,float r){float t=clamp(1.0-d/r,0.0,1.0);return t*t;}
void main(){
    vec3 alb=(u_HasTexture!=0)?texture(u_AlbedoTex,v_TexCoord*u_UVScale).rgb*u_AlbedoColour:u_AlbedoColour;
    vec3 N=normalize(v_Normal),V=normalize(u_CamPos-v_WorldPos);
    vec3 Ls=normalize(-u_SunDir);
    float NdL=max(dot(N,Ls),0.0);
    vec3 diff=u_SunColour*NdL,spec=u_SunColour*BP(N,Ls,V,u_Roughness)*u_Specular;
    for(int i=0;i<u_PointLightCount;++i){
        vec3 tL=u_PointLights[i].position-v_WorldPos;
        float d=length(tL);vec3 L=tL/d;
        float a=Att(d,u_PointLights[i].radius)*u_PointLights[i].intensity;
        diff+=u_PointLights[i].colour*max(dot(N,L),0.0)*a;
        spec+=u_PointLights[i].colour*BP(N,L,V,u_Roughness)*u_Specular*a;
    }
    vec3 col=alb*(u_Ambient+diff)+spec;
    if(u_FogDensity>0.001){
        float ff=clamp(exp(-u_FogDensity*v_ViewDepth),0.0,1.0);
        col=mix(u_FogColour,col,ff);
    }
    col=col/(col+vec3(1.0));
    FragColor=vec4(col,1.0);
}
