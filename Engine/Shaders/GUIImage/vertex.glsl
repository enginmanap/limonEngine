#version 330

uniform mat4 worldTransformMatrix;
uniform mat4 orthogonalProjectionMatrix;

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;

out vec2 vs_fs_textureCoord;
out float externalAlpha;

void main(void)
{
    vs_fs_textureCoord = textureCoordinate;
    gl_Position = orthogonalProjectionMatrix * (worldTransformMatrix * position);
    externalAlpha = 1.0 - worldTransformMatrix[3][2];
}
