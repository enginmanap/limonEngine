
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

layout (location = 1) out float occlusion;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_gNormalMap;   // World-space normal, pocked
uniform sampler2D pre_depthMap;
uniform sampler2D ssaoNoiseSampler;

uniform vec3 ssaoKernel[128];
uniform int ssaoSampleCount;
float uRadius = 0.3f;

vec3 unpackNormal(vec2 pa) {
    vec2 p = pa * 2.0 - 1.0;

    // Reconstruct Y-Up vector, other up vectors have different formula
    vec3 n = vec3(p.x, 1.0 - abs(p.x) - abs(p.y), p.y);

    if (n.y < 0.0) {
        float oldX = n.x;
        n.x = (1.0 - abs(n.z)) * (oldX >= 0.0 ? 1.0 : -1.0);
        n.z = (1.0 - abs(oldX)) * (n.z >= 0.0 ? 1.0 : -1.0);
    }

    return normalize(n);
}

void main(){
    vec3 worldSpaceNormal = unpackNormal(texture(pre_gNormalMap, from_vs.textureCoordinates).xy);
    float depth = texture(pre_depthMap, from_vs.textureCoordinates).r;

    // Precompute inverse projection scalars used for depth linearisation.
    // For a perspective camera invProj[2][2]=0, so z_view = ipZC / (ipZA*z_ndc + ipZB).
    // Reading them once avoids repeated UBO lookups inside the sample loop.
    float ipZA = playerTransforms.inverseProjection[2][3]; // z_ndc coefficient in w
    float ipZB = playerTransforms.inverseProjection[3][3]; // constant in w
    float ipZC = playerTransforms.inverseProjection[3][2]; // z numerator (= -1 std GL)

    // Reconstruct base view-space position from depth
    float z_ndc = depth * 2.0 - 1.0;
    float inv_w  = 1.0 / (ipZA * z_ndc + ipZB);
    vec2 ndc_xy  = from_vs.textureCoordinates * 2.0 - 1.0;
    vec3 basePosition = vec3(
        playerTransforms.inverseProjection[0][0] * ndc_xy.x * inv_w,
        playerTransforms.inverseProjection[1][1] * ndc_xy.y * inv_w,
        ipZC * inv_w
    );

    // Camera distance based attenuation
    const float FADE_END = 75.0;
    const float FADE_START = 0.0;
    float viewDepth = abs(basePosition.z);
    float distanceFade = 1.0 - smoothstep(FADE_START, FADE_END, viewDepth);

    if (distanceFade <= 0.0) {
        occlusion = 0.0; // No occlusion needed beyond fade distance
        return;
    }

    // Scale radius with distance to prevent large halos on distant objects
    // Decreasing radius as distance increases.
    float scaledRadius  = uRadius / (1.0 + viewDepth * 0.01);
    float scaledRadius2 = scaledRadius * scaledRadius; // precomputed for loop
    float invRadius     = 1.0 / scaledRadius;          // precomputed for loop

    vec3 normal    = normalize(playerTransforms.transposeInverseCamera * worldSpaceNormal);
    vec3 randomVec = texture(ssaoNoiseSampler, from_vs.textureCoordinates * playerTransforms.noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float tempOcclusion = 0.0;

    // Explicit angle bias to prevent shadow acne on flat surfaces.
    const float ANGLE_BIAS = 0.05;

    for(int i = 0; i < ssaoSampleCount; ++i){
        vec3 sampleDir = TBN * ssaoKernel[i];
        vec3 samplePos = basePosition + sampleDir * scaledRadius;

        // Project sample to UV space for depth buffer lookup
        vec4 offset = playerTransforms.projection * vec4(samplePos, 1.0);
        offset.xy /= offset.w;
        offset.xy  = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(pre_depthMap, offset.xy).r;

        // Reconstruct neighbour view-space position without a full mat4 multiply.
        // Only z is needed from the inverse projection; XY follow from the perspective
        // depth ratio: x_actual = samplePos.x * (z_actual / samplePos.z).
        float s_z_ndc  = sampleDepth * 2.0 - 1.0;
        float s_z_view = ipZC / (ipZA * s_z_ndc + ipZB);
        float depthRatio = s_z_view / samplePos.z;
        vec3 realNeighborPos = vec3(samplePos.xy * depthRatio, s_z_view);

        vec3  horizonVec = realNeighborPos - basePosition;
        float dist2      = dot(horizonVec, horizonVec);

        if (dist2 < scaledRadius2 && dist2 > 0.0001) {
            // Single inversesqrt covers both the normalize and the distance,
            // avoiding a second sqrt: dist = dot(h,h) * inversesqrt(dot(h,h)) = sqrt(dot(h,h))
            float distInv = inversesqrt(dist2);
            float dotVal  = dot(normal, horizonVec) * distInv;

            if (dotVal > ANGLE_BIAS) {
                float dist        = dist2 * distInv;
                float attenuation = smoothstep(0.0, 1.0, 1.0 - dist * invRadius);
                tempOcclusion += dotVal * attenuation;
            }
        }
    }

    occlusion = (tempOcclusion / float(ssaoSampleCount)) * distanceFade;
}
