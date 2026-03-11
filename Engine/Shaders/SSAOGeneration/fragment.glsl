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

vec3 calcViewSpacePosFromDepth(vec2 texCoords, float depth) {
    vec4 clipSpacePos = vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePos = playerTransforms.inverseProjection * clipSpacePos;
    return viewSpacePos.xyz / viewSpacePos.w;
}

void main(){
    vec3 worldSpaceNormal = texture(gNormalMap, from_vs.textureCoordinates).xyz;
    float depth = texture(pre_depthMap, from_vs.textureCoordinates.xy).r;

    vec3 basePosition = calcViewSpacePosFromDepth(from_vs.textureCoordinates, depth);
    vec3 normal = normalize(playerTransforms.transposeInverseCamera * worldSpaceNormal);

    vec3 randomVec = texture(ssaoNoiseSampler, from_vs.textureCoordinates * playerTransforms.noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float tempOcculusion = 0.0;
    for(int i = 0; i < ssaoSampleCount; ++i){
        vec3 samplePosition = TBN * ssaoKernel[i];
        samplePosition = basePosition + samplePosition * uRadius;

        vec4 offset = playerTransforms.projection * vec4(samplePosition, 1.0); // from view to clip-space
        offset.xy /= offset.w;
        offset.xy  = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(pre_depthMap, offset.xy).r;
        vec3 realElement = calcViewSpacePosFromDepth(offset.xy, sampleDepth);

        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(samplePosition.z - realElement.z));
        tempOcculusion += (realElement.z >= samplePosition.z + uBias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = tempOcculusion / ssaoSampleCount;
}
