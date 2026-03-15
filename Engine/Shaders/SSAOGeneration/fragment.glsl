#version 330 core

layout (location = 1) out float occlusion;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    mat4 inverseProjection;
    mat4 inverseCamera;
    mat3 transposeInverseCamera;
    vec3 position;
    vec3 cameraSpacePosition;
    vec2 noiseScale;
    int time;
} playerTransforms;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D gNormalMap;   // World-space normal
uniform sampler2D pre_depthMap;
uniform sampler2D ssaoNoiseSampler;

uniform vec3 ssaoKernel[128];
uniform int ssaoSampleCount;
uniform float uRadius = 0.3f;
uniform float uBias = 0.005f;

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

vec3 calcViewSpacePosFromDepth(vec2 texCoords, float depth) {
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = playerTransforms.inverseProjection * clipSpacePos;
    return viewSpacePos.xyz / viewSpacePos.w;
}

void main(){
    vec3 worldSpaceNormal = unpackNormal(texture(gNormalMap, from_vs.textureCoordinates).xy);
    float depth = texture(pre_depthMap, from_vs.textureCoordinates.xy).r;

    vec3 basePosition = calcViewSpacePosFromDepth(from_vs.textureCoordinates, depth);

    // Camera distance based Attenuation
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
    float scaledRadius = uRadius / (1.0 + viewDepth * 0.01);

    vec3 normal = normalize(playerTransforms.transposeInverseCamera * worldSpaceNormal);
    vec3 randomVec = texture(ssaoNoiseSampler, from_vs.textureCoordinates * playerTransforms.noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float tempOcculusion = 0.0;

    // Explicit angle bias to prevent shadow acne on flat surfaces.
    const float ANGLE_BIAS = 0.05;

    for(int i = 0; i < ssaoSampleCount; ++i){
        vec3 sampleDir = TBN * ssaoKernel[i];
        vec3 samplePos = basePosition + sampleDir * scaledRadius;

        vec4 offset = playerTransforms.projection * vec4(samplePos, 1.0);
        offset.xy /= offset.w;
        offset.xy  = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(pre_depthMap, offset.xy).r;
        vec3 realNeighborPos = calcViewSpacePosFromDepth(offset.xy, sampleDepth);

        vec3 horizonVec = realNeighborPos - basePosition;
        float dist = length(horizonVec);

        if (dist < scaledRadius && dist > 0.01) {
            float dotVal = dot(normal, normalize(horizonVec));

            if (dotVal > ANGLE_BIAS) {
                // Attenuate relative to scaledRadius
                float attenuation = smoothstep(0.0, 1.0, 1.0 - (dist / scaledRadius));
                tempOcculusion += dotVal * attenuation;
            }
        }
    }

    occlusion = (tempOcculusion / float(ssaoSampleCount)) * distanceFade;
}
