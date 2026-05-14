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

    finalColor   = vec4(color - ambient * ssao, 1.0);
    gl_FragDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;
}
