#define_option ssao_strength
#ifndef ssao_strength
#define ssao_strength 3.0
#endif

layout (location = 0) out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_colorBuffer;
uniform sampler2D pre_ambientBuffer;
uniform sampler2D pre_ssao;
uniform sampler2D pre_depthMap;

void main() {
    vec3 color   = texture(pre_colorBuffer,   from_vs.textureCoordinates).rgb;
    vec3 ambient = texture(pre_ambientBuffer, from_vs.textureCoordinates).rgb;
    float ssao   = texture(pre_ssao,          from_vs.textureCoordinates).r;

    float occlusion = clamp(ssao * float(ssao_strength), 0.0, 1.0);
    finalColor   = vec4(color + ambient * (1.0 - occlusion), 1.0);
    gl_FragDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;
}
