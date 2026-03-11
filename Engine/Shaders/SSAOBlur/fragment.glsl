#version 330 core

layout (location = 1) out float occlusion;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_ssaoResult;
uniform sampler2D pre_depthMap;

void main() {
    float selfDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;

    vec2 texelSize = 1.0 / vec2(textureSize(pre_ssaoResult, 0));
    float result = 0.0;
    int sampleCount = 0;

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleCoords = from_vs.textureCoordinates + offset;

            float sampleDepth = texture(pre_depthMap, sampleCoords).r;

            // Only include samples that are at a similar depth
            if(abs(sampleDepth - selfDepth) < 0.02) {
                result += texture(pre_ssaoResult, sampleCoords).r;
                sampleCount++;
            }
        }
    }

    // division by zero guard
    if (sampleCount > 0) {
        occlusion = result / float(sampleCount);
    } else {
        // zero sample fall back
        occlusion = texture(pre_ssaoResult, from_vs.textureCoordinates).r;
    }
}
