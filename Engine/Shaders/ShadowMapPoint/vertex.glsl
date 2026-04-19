
#import <./Engine/Shaders/Shared/ModelRendering.vert>

layout (location = 2) in vec4 position;
layout (location = 4) in vec3 normal;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

void main() {
    vec3 worldPosition;
    vec3 worldNormal;
    calculateWorldPositionAndNormal(position, normal, boneIDs, boneWeights, worldPosition, worldNormal);
    gl_Position = vec4(worldPosition, 1.0);
}
