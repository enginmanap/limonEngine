
layout (location = 1) out float occlusion;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_ssaoResult;
uniform sampler2D pre_depthMap;

// Gaussian kernel weights
const float gaussian_kernel[25] = float[](
    1.0,  4.0,  7.0,  4.0,  1.0,
    4.0, 16.0, 26.0, 16.0,  4.0,
    7.0, 26.0, 41.0, 26.0,  7.0,
    4.0, 16.0, 26.0, 16.0,  4.0,
    1.0,  4.0,  7.0,  4.0,  1.0
);


void main() {
    float selfDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;

    vec2 texelSize = 1.0 / vec2(textureSize(pre_ssaoResult, 0));
    float result = 0.0;
    float totalWeight = 0.0;

    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleCoords = from_vs.textureCoordinates + offset;

            float sampleDepth = texture(pre_depthMap, sampleCoords).r;

            if(abs(sampleDepth - selfDepth) < 0.02) {
                float weight = gaussian_kernel[(x + 2) * 5 + (y + 2)];
                result += texture(pre_ssaoResult, sampleCoords).r * weight;
                totalWeight += weight;
            }
        }
    }

    if(totalWeight >= 41.0) {
        occlusion = result / totalWeight;
    } else {
        occlusion = 0.0;
    }
}
