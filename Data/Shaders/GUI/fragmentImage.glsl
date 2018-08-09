#version 330

uniform sampler2D GUISampler;

in vec2 vs_fs_textureCoord;
out vec4 color;

void main(void)
{
		color = texture(GUISampler, vs_fs_textureCoord);
}
