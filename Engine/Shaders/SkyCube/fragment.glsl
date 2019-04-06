#version 330

in vec3 texCoords;

layout (location = 0) out vec4 finalColor;

uniform samplerCube cubeSampler;

void main(void)
{
	finalColor = texture(cubeSampler, texCoords);
}
