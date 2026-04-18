
#import <./Engine/Shaders/Shared/ModelRendering.vertex>

#define NR_POINT_LIGHTS 4

layout (location = 2) in vec4 position;
layout (location = 4) in vec3 normal;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

struct LightSource {
    mat4 shadowMatrices[6];
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
uniform int renderLightLayer;

void main() {
    vec3 worldPosition;
    vec3 worldNormal;
    calculateWorldPositionAndNormal(position, normal, boneIDs, boneWeights, worldPosition, worldNormal);
    gl_Position = LightSources.lights[renderLightIndex].shadowMatrices[renderLightLayer] * vec4(worldPosition, 1.0);
}
