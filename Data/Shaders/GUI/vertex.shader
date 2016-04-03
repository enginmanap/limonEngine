#version 400

uniform mat4 worldTransformMatrix;
uniform mat4 orthogonalProjectionMatrix;

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;

out vec2 vs_fs_textureCoord;

void main(void)
{
    vs_fs_textureCoord = textureCoordinate;
    gl_Position = orthogonalProjectionMatrix * (worldTransformMatrix * position);
}
