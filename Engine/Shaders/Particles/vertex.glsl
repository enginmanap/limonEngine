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
    float alpha;
} to_fs;

uniform sampler2D positions;
uniform float size;


void main(){
    to_fs.textureCoordinates = textureCoordinates;
    vec2 texelSize = 1.0 / vec2(textureSize(positions, 0));
    float elementIndex = gl_InstanceID * texelSize.x;
    vec4 worldPosition = texture(positions, vec2(elementIndex, 0.0));
    to_fs.alpha = worldPosition.w;
    worldPosition.w = 1.0;
    vec4 cameraCenterPosition = playerTransforms.cameraProjection * worldPosition;
    cameraCenterPosition.xyz = cameraCenterPosition.xyz + (position.xyz * size);

    gl_Position = cameraCenterPosition;
}