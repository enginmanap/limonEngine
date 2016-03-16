#version 330

uniform mat4 worldTransformMatrix;
uniform mat4 cameraTransformMatrix;

layout (location = 2) in vec4 position;
layout (location = 3) in vec4 color;

out vec4 vs_fs_color;

void main(void)
{
    vs_fs_color = color;
    gl_Position = cameraTransformMatrix * (worldTransformMatrix * position);
}
