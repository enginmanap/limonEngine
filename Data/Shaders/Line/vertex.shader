#version 330

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 color;

uniform mat4 cameraTransformMatrix;


out vec4 vs_fs_color;

void main(void)
{
    gl_Position = cameraTransformMatrix * position;
    vs_fs_color = color;
}
