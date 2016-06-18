#version 330

uniform mat4 worldTransformMatrix;
uniform mat4 cameraTransformMatrix;

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;

out vec2 vs_fs_textureCoord;
out vec3 vs_fs_normal;
out vec3 vs_fs_fragPos;

void main(void)
{
    vs_fs_textureCoord = textureCoordinate;
    vs_fs_normal = normalize(mat3(transpose(inverse(worldTransformMatrix))) * normal);
    vs_fs_fragPos = vec3(worldTransformMatrix * position);
    gl_Position = cameraTransformMatrix * (worldTransformMatrix * position);
}
