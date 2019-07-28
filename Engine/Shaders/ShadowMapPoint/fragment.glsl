#version 330 core
#define NR_POINT_LIGHTS 4


in vec4 FragPos;

struct LightSource {
    mat4 shadowMatrices[6];
    mat4 lightSpaceMatrix;
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type;
    vec3 attenuation;
    vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

uniform int renderLightIndex;


void main()
{
    float lightDistance = length(FragPos.xyz - LightSources.lights[renderLightIndex].position);
    lightDistance = lightDistance / LightSources.lights[renderLightIndex].farPlanePoint;
    gl_FragDepth = lightDistance;
}  