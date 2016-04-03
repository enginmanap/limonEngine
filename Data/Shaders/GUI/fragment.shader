#version 400

in vec2 vs_fs_textureCoord;
out vec4 color;

uniform sampler2D boxSampler;

void main(void)
{
		color = vec4(1.0, 1.0, 1.0, texture(boxSampler, vs_fs_textureCoord).r);
}
