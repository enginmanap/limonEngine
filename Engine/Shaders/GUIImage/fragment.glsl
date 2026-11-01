
#import <./Engine/Shaders/Shared/ColorSpace.glsl>

uniform sampler2D GUISampler;

in vec2 vs_fs_textureCoord;
in float externalAlpha;

out vec4 color;

void main(void)
{
		color = texture(GUISampler, vs_fs_textureCoord);
#ifndef LIMON_TEXTURES_UNORM
		//the sampler decoded it and this stage writes raw, so the curve goes back on. A Data image shows too bright here
		color.rgb = linearToSRGB(color.rgb);
#endif
		color.w = color.w * externalAlpha;
}
