#version 400
#extension GL_ARB_explicit_uniform_location : enable

in vec4 vs_fs_color;
out vec4 color;

void main(void)
{
		color = vs_fs_color;
}
