#version 330

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 cameraTransformMatrix;

out vec4 vs_fs_color;

void main(void)
{
    gl_Position = cameraTransformMatrix * vec4(position,1.0);
    vs_fs_color = vec4(color,1.0);
}
