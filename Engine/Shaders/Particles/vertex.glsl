#version 330 core

layout (location = 2) in vec3 position;
layout (location = 3) in vec2 textureCoordinates;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    mat4 inverseProjection;
    mat4 inverseCamera;
    mat3 transposeInverseCamera;
    vec3 position;
    vec3 cameraSpacePosition;
    vec2 noiseScale;
} playerTransforms;

out VS_FS {
    vec2 textureCoordinates;
    vec4 colorMultiplier;
} to_fs;

uniform sampler2D positions;
uniform float size;

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
    cameraCenterPosition.xyz = cameraCenterPosition.xyz + (position.xyz * size);

    gl_Position = cameraCenterPosition;
}