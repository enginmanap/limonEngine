#version 400
#extension GL_ARB_explicit_uniform_location : enable

layout (location = 0) uniform mat4 worldTransformMatrix;
layout (location = 1) uniform mat4 cameraTransformMatrix;

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;

out vec2 vs_fs_textureCoord;

void main(void)
{
    vs_fs_textureCoord = textureCoordinate;
    gl_Position = cameraTransformMatrix * (worldTransformMatrix * position);
}
