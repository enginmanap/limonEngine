#version 330 core

layout (location = 1) out float occlusion;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D ssaoResultSampler;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoResultSampler, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoResultSampler, from_vs.textureCoordinates + offset).r;
        }
    }
    occlusion = result / (5.0 * 5.0);
}