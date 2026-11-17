#define_option ssao_power
#ifndef ssao_power
#define ssao_power 1.0
#endif
#define_option ssao_radius
#ifndef ssao_radius
#define ssao_radius 0.3
#endif
#define_option ssao_fadeEnd
#ifndef ssao_fadeEnd
#define ssao_fadeEnd 75.0
#endif
#define_option ssao_angleBias
#ifndef ssao_angleBias
#define ssao_angleBias 0.05
#endif


#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

//occlusion and linear view depth as packHalf2x16 halves of one R32F texel: the blur gets both from one gather, and
//linear depth gives it a tolerance that scales with distance
layout (location = 1) out float occlusionAndDepth;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_gNormalMap;   // World-space normal, pocked
uniform sampler2D pre_depthMap;
uniform sampler2D ssaoNoiseSampler;

uniform vec3 ssaoKernel[128];
#define_option ssao_sampleCount
const int ssaoSampleCount = int(ssao_sampleCount);//compile time, so the loop unrolls
const float uRadius = float(ssao_radius);

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
    // if user is using different resolution for ssao, than display resolution, the ssao sample gets filtered. We are trying
    // to get depth and normal, if they are filtered, results are wrong. So we use texelFetch
    ivec2 fullResPixel = ivec2(from_vs.textureCoordinates * vec2(textureSize(pre_depthMap, 0)));
    vec2 pixelCoordinates = (vec2(fullResPixel) + 0.5) / vec2(textureSize(pre_depthMap, 0));
    vec3 worldSpaceNormal = unpackNormal(texelFetch(pre_gNormalMap, fullResPixel, 0).xy);
    float depth = texelFetch(pre_depthMap, fullResPixel, 0).r;

    // Precompute inverse projection scalars used for depth linearisation.
    // For a perspective camera invProj[2][2]=0, so z_view = ipZC / (ipZA*z_ndc + ipZB).
    // Reading them once avoids repeated UBO lookups inside the sample loop.
    float ipZA = playerTransforms.inverseProjection[2][3]; // z_ndc coefficient in w
    float ipZB = playerTransforms.inverseProjection[3][3]; // constant in w
    float ipZC = playerTransforms.inverseProjection[3][2]; // z numerator (= -1 std GL)

    // Reconstruct base view-space position from depth
    float z_ndc = depth * 2.0 - 1.0;
    float inv_w  = 1.0 / (ipZA * z_ndc + ipZB);
    vec2 ndc_xy  = pixelCoordinates * 2.0 - 1.0;
    vec3 basePosition = vec3(
        playerTransforms.inverseProjection[0][0] * ndc_xy.x * inv_w,
        playerTransforms.inverseProjection[1][1] * ndc_xy.y * inv_w,
        ipZC * inv_w
    );

    // Camera distance based attenuation
    const float FADE_END = float(ssao_fadeEnd);
    const float FADE_START = 0.0;
    float viewDepth = abs(basePosition.z);
    float distanceFade = 1.0 - smoothstep(FADE_START, FADE_END, viewDepth);

    if (distanceFade <= 0.0) {
        occlusionAndDepth = uintBitsToFloat(packHalf2x16(vec2(0.0, viewDepth))); // No occlusion needed beyond fade distance
        return;
    }

    // Scale radius with distance to prevent large halos on distant objects
    // Decreasing radius as distance increases.
    float scaledRadius  = uRadius / (1.0 + viewDepth * 0.01);
    float scaledRadius2 = scaledRadius * scaledRadius; // precomputed for loop
    float invRadius     = 1.0 / scaledRadius;          // precomputed for loop

    vec3 normal    = normalize(playerTransforms.transposeInverseCamera * worldSpaceNormal);
    //indexed by this target's own pixel, not by screen coordinates: below full resolution those put several noise texels
    //in one AO texel, so the blur window would not line up with the tile and couldn't cancel it.
    //tile size depends on AO resolution, see SSAOKernelRenderMethod::generateSSAONoiseTexture
    vec3 randomVec = texelFetch(ssaoNoiseSampler, ivec2(gl_FragCoord.xy) % textureSize(ssaoNoiseSampler, 0), 0).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float tempOcclusion = 0.0;

    // Explicit angle bias to prevent shadow acne on flat surfaces.
    const float ANGLE_BIAS = float(ssao_angleBias);

    for(int i = 0; i < ssaoSampleCount; ++i){
        vec3 sampleDir = TBN * ssaoKernel[i];
        vec3 samplePos = basePosition + sampleDir * scaledRadius;

        // Project sample to UV space for depth buffer lookup
        //the player camera is a symmetric perspective, clip xy is the diagonal times view xy and w is -z
        vec2 offset = vec2(playerTransforms.projection[0][0] * samplePos.x, playerTransforms.projection[1][1] * samplePos.y) / -samplePos.z * 0.5 + 0.5;

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

    float occlusion = pow(tempOcclusion / float(ssaoSampleCount), float(ssao_power)) * distanceFade;
    occlusionAndDepth = uintBitsToFloat(packHalf2x16(vec2(occlusion, viewDepth)));
}
