#version 330 core

layout (location = 1) in vec3 position;
layout (location = 2) in vec2 textureCoordinates;

out VS_FS {
    vec2 textureCoordinates;
} to_fs;

void main()
{
    to_fs.textureCoordinates = textureCoordinates;
    gl_Position = vec4(position, 1.0);
}