#version 330 core

layout (location = 1) in vec3 position;
layout (location = 2) in vec2 textureCoordinates;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    mat4 inverseProjection;
	mat4 inverseCamera;
    vec3 position;
	vec3 cameraSpacePosition;
    vec2 noiseScale;
} playerTransforms;

out VS_FS {
    vec2 textureCoordinates;
} to_fs;

void main()
{
    to_fs.textureCoordinates = textureCoordinates;
    gl_Position = vec4(position, 1.0);
}