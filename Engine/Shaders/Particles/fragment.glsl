
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/ColorSpace.glsl>

layout (location = 0) out vec4 color;

in VS_FS {
    vec2 textureCoordinates;
    vec4 colorMultiplier;
} from_vs;

uniform sampler2D sprite;
uniform sampler2D pre_depthMap;


void main(void){
    vec4 sampled = texture(sprite, from_vs.textureCoordinates);
    color = vec4(colorTextureToLinear(sampled.rgb), sampled.a);
    color *= from_vs.colorMultiplier;
    color.rgb = linearToOutputColor(color.rgb);

}
