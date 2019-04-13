#version 330 core

layout (location = 1) out float occlusion;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    mat4 inverseProjection;
	mat4 inverseCamera;
    vec3 position;
	vec3 cameraSpacePosition;
    vec2 noiseScale;
} playerTransforms;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D depthMapSampler;
uniform sampler2D normalMapSampler;
uniform sampler2D ssaoNoiseSampler;


uniform vec3 ssaoKernel[128];
uniform int ssaoSampleCount;

vec3 calcViewSpacePos(vec3 screen) {
    vec4 temp = vec4(screen.x, screen.y, screen.z, 1);
    temp.z = temp.z * 2 - 1;
    temp.y = temp.y * 2 - 1;
    temp.x = temp.x * 2 - 1;
    temp = playerTransforms.inverseProjection * temp;
    vec3 camera_space = temp.xyz / temp.w;
    return camera_space;
}


void main(){

    vec3 normal = texture(normalMapSampler, from_vs.textureCoordinates.xy).xyz;
         normal = normalize(mat3(transpose(inverse(playerTransforms.camera))) * normal);
    float depth = texture(depthMapSampler, from_vs.textureCoordinates.xy).r;

    vec3 randomVec = normalize(texture(ssaoNoiseSampler, from_vs.textureCoordinates * playerTransforms.noiseScale).xyz);

    vec3 tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normal);

    float uRadius = 0.3f;

    vec3 basePosition = calcViewSpacePos( vec3(from_vs.textureCoordinates, depth));
    vec3 playerPosView = (playerTransforms.camera * vec4(playerTransforms.position, 1.0)).xyz;
    //set a bias, that scales with the world space distance with player
    float distanceSQ = float((basePosition.x - playerPosView.x) * (basePosition.x - playerPosView.x) +
                             (basePosition.y - playerPosView.y) * (basePosition.y - playerPosView.y) +
                             (basePosition.z - playerPosView.z) * (basePosition.z - playerPosView.z));

    float bias = 0.005 + 0.01 * distanceSQ / 1000;
    float tempOcculusion = 0;
    for(int i = 0; i < ssaoSampleCount; ++i){
        // get sample position
        vec3 samplePosition = TBN * ssaoKernel[i]; // From tangent to view-space
        samplePosition = samplePosition * uRadius;
        samplePosition  += basePosition;

        vec4 offset = vec4(samplePosition, 1.0);
        offset = playerTransforms.projection * offset;    // from view to clip-space
        offset.xyz /= offset.w;               // perspective divide
        offset.xyz  = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        float sampleDepth = texture(depthMapSampler, offset.xy).r;
        vec3 realElement = calcViewSpacePos(vec3(offset.xy, sampleDepth));
        float rangeCheck= abs(realElement.z - samplePosition.z) < uRadius ? 1.0 : 0.0;
        tempOcculusion += (realElement.z > (samplePosition.z + bias) ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = tempOcculusion / ssaoSampleCount;
}



