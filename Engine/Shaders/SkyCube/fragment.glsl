#version 330

in vec3 texCoords;

layout (location = 0) out vec4 finalColor;
layout (location = 1) out vec3 normalOutput;

uniform samplerCube cubeSampler;

void main(void)
{
	finalColor = texture(cubeSampler, texCoords);
	normalOutput = vec3(0,0,0);
}
