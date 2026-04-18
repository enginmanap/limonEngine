
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

layout (location = 0) out vec4 color;

in VS_FS {
    vec2 textureCoordinates;
    vec4 colorMultiplier;
} from_vs;

uniform sampler2D sprite;
uniform sampler2D pre_depthMap;


void main(void){
    color = (texture(sprite, from_vs.textureCoordinates));
    color *= from_vs.colorMultiplier;

}
