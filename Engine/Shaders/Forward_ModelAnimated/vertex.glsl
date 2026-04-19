#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/ModelRendering.vert>

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

out VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    flat int materialIndex;
} to_fs;

void main(void) {
    to_fs.textureCoord = textureCoordinate;

    calculateWorldPositionAndNormal(position, normal, boneIDs, boneWeights, to_fs.fragPos, to_fs.normal);

    to_fs.materialIndex = int(instance.models[gl_InstanceID].y);
    gl_Position = playerTransforms.cameraProjection * vec4(to_fs.fragPos, 1.0);
}