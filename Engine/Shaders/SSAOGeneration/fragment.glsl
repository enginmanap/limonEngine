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

uniform sampler2D pre_depthMap;
uniform sampler2D pre_normalMap;
uniform sampler2D ssaoNoiseSampler;


uniform vec3 ssaoKernel[128];
uniform int ssaoSampleCount;

vec3 calcViewSpacePos(vec3 screen) {
    vec4 temp = vec4(screen.xyz * 2 - 1, 1.0);
    temp = playerTransforms.inverseProjection * temp;
    vec3 camera_space = temp.xyz / temp.w;
    return camera_space;
}


void main(){

    vec3 normal = texture(pre_normalMap, from_vs.textureCoordinates.xy).xyz;
         normal = normalize(playerTransforms.transposeInverseCamera * normal);
    float depth = texture(pre_depthMap, from_vs.textureCoordinates.xy).r;

    vec3 randomVec = texture(ssaoNoiseSampler, from_vs.textureCoordinates * playerTransforms.noiseScale).xyz;

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float uRadius = 0.3f;

    vec3 basePosition = calcViewSpacePos( vec3(from_vs.textureCoordinates, depth));
    vec3 playerPosView = (playerTransforms.camera * vec4(playerTransforms.position, 1.0)).xyz;
    //set a bias, that scales with the world space distance with player
//    float distanceSQ = float((basePosition.x - playerPosView.x) * (basePosition.x - playerPosView.x) +
//                             (basePosition.y - playerPosView.y) * (basePosition.y - playerPosView.y) +
//                             (basePosition.z - playerPosView.z) * (basePosition.z - playerPosView.z));
//    distanceSQ = sqrt(distanceSQ);
    float distanceSQ = distance(basePosition, playerPosView);
    float bias = 0.005 + 0.1 * distanceSQ / 1000;
    float tempOcculusion = 0;
    for(int i = 0; i < ssaoSampleCount; ++i){
        // get sample position
        vec3 samplePosition = TBN * ssaoKernel[i]; // From tangent to view-space
        samplePosition = samplePosition * uRadius;
        samplePosition  += basePosition;

        vec4 offset = vec4(samplePosition, 1.0);
        offset = playerTransforms.projection * offset;    // from view to clip-space
        offset.xy /= offset.w;               // perspective divide
        offset.xy  = offset.xy * 0.5 + 0.5; // transform to range 0.0 - 1.0
        float sampleDepth = texture(pre_depthMap, offset.xy).r;
        vec3 realElement = calcViewSpacePos(vec3(offset.xy, sampleDepth));
        //float rangeCheck= abs(realElement.z - samplePosition.z) < uRadius ? (1.0/uRadius) *  abs(realElement.z - samplePosition.z): 0.0;
        float rangeCheck = smoothstep(0.0, 1.0, uRadius / abs(samplePosition.z - realElement.z));
        if(realElement.z > (samplePosition.z + bias)) {
            tempOcculusion += rangeCheck;
        }

    }

    occlusion = tempOcculusion / ssaoSampleCount;
}



