#version 330

in vec3 texCoords;
out vec4 color;
uniform samplerCube cubeSampler;

void main(void)
{
	color = texture(cubeSampler, texCoords);
}
