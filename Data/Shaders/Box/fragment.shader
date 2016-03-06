#version 400
#extension GL_ARB_explicit_uniform_location : enable

in vec2 vs_fs_textureCoord;
out vec4 color;

uniform sampler2D boxSampler;

void main(void)
{
		color = texture(boxSampler, vs_fs_textureCoord);
}
