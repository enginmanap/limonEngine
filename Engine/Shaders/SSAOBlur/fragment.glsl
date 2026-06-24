
#define_option ssao_blurRadius

layout (location = 1) out float occlusion;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_ssaoResult;
uniform sampler2D pre_depthMap;

void main() {
    vec2  texelSize = 1.0 / vec2(textureSize(pre_ssaoResult, 0));
    float selfDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;

    float result      = 0.0;
    float totalWeight = 0.0;

    // sigma = ssao_blurRadius so the Gaussian widens naturally with the kernel size.
    // ssao_blurRadius is a compile-time constant, so the compiler unrolls the loops
    // and constant-folds all exp() evaluations.
    const float sigma2 = 2.0 * float(ssao_blurRadius * ssao_blurRadius);

    for (int x = -ssao_blurRadius; x <= ssao_blurRadius; ++x) {
        for (int y = -ssao_blurRadius; y <= ssao_blurRadius; ++y) {
            vec2 coords = from_vs.textureCoordinates + vec2(float(x), float(y)) * texelSize;

            // Both reads issued before any arithmetic so the GPU can pipeline them together
            float sampleDepth = texture(pre_depthMap,  coords).r;
            float sampleSSAO  = texture(pre_ssaoResult, coords).r;

            // Bilateral depth weight: zero if sample is from a different surface
            float depthWeight    = 1.0 - step(0.02, abs(sampleDepth - selfDepth));
            float gaussianWeight = exp(-float(x * x + y * y) / sigma2);
            float weight         = gaussianWeight * depthWeight;
            result      += sampleSSAO * weight;
            totalWeight += weight;
        }
    }

    // Center always passes depth test (distance to itself = 0), so totalWeight > 0 always.
    // The guard handles degenerate cases (e.g. sky pixels with depth = 1).
    occlusion = (totalWeight > 0.0) ? result / totalWeight : 0.0;
}
