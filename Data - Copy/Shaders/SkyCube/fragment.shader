#version 400
#extension GL_ARB_explicit_uniform_location : enable


in vec3 texCoords;
out vec4 color;
uniform samplerCube cubeSampler;

void main(void)
{
	//color = vec4(1,0.1,0.1,1);
	color = texture(cubeSampler, texCoords);
}
