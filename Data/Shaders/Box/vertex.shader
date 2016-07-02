#version 330

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;

out VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
} to_fs;

uniform mat4 worldTransformMatrix;
uniform mat4 cameraTransformMatrix;

void main(void)
{
    to_fs.textureCoord = textureCoordinate;
    to_fs.normal = normalize(mat3(transpose(inverse(worldTransformMatrix))) * normal);
    to_fs.fragPos = vec3(worldTransformMatrix * position);
    gl_Position = cameraTransformMatrix * (worldTransformMatrix * position);
}
