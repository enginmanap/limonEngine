#version 330

uniform sampler2DArray GUISampler;
uniform vec3 inColor;
uniform int characterIndex;

in vec2 vs_fs_textureCoord;
out vec4 color;

void main(void)
{
		color = vec4(inColor, texture(GUISampler, vec3(vs_fs_textureCoord, characterIndex)).r);
}
