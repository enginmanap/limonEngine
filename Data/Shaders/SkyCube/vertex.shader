#version 330

uniform mat4 cameraTransformMatrix;

layout (location = 2) in vec4 position;
out vec3 texCoords;

void main(void)
{
    gl_Position = cameraTransformMatrix * position;
    gl_Position.z = gl_Position.w;
	texCoords = position.xyz;
}
