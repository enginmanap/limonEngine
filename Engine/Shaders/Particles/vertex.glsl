
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

layout (location = 2) in vec3 position;
layout (location = 3) in vec2 textureCoordinates;

out VS_FS {
    vec2 textureCoordinates;
    vec4 colorMultiplier;
} to_fs;

uniform sampler2D positions;
uniform vec3 size;

vec4 unpackFloat(float value) {
    uint rgba = floatBitsToUint(value);
    float a = float(rgba >> 24) / 255.0;
    float b = float((rgba & 0x00ff0000u) >> 16) / 255.0;
    float g = float((rgba & 0x0000ff00u) >> 8) / 255.0;
    float r = float(rgba & 0x000000ffu) / 255.0;
    return vec4(r, g, b, a);
}

void main(){
    to_fs.textureCoordinates = textureCoordinates;
    vec4 worldPosition = texelFetch(positions, ivec2(gl_InstanceID, 0), 0);
    to_fs.colorMultiplier = unpackFloat(worldPosition.w);
    worldPosition.w = 1.0;
    vec4 cameraCenterPosition = playerTransforms.cameraProjection * worldPosition;
    //position.z is always 0 for this quad, so only x/y need the (previously scalar, now per-axis) size scale.
    cameraCenterPosition.xyz = cameraCenterPosition.xyz + vec3(position.xy * size.xy, 0.0);

    gl_Position = cameraCenterPosition;
}
