#version 330

#define NR_POINT_LIGHTS 4
#define NR_MAX_MODELS 4096

#define NR_BONE 128

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

out VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    flat int materialIndex;
} to_fs;

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

struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type; //1 Directional, 2 point
	vec3 attenuation;
	vec3 ambient;
};

uniform sampler2D allModelTransformsTexture;

layout (std140) uniform ModelIndexBlock {
    uvec4 models[NR_MAX_MODELS];
} instance;

struct rigBones {
    mat4 transform[NR_BONE];
};

layout (std140) uniform AllAnimationsBlock {
    rigBones rigs[8];
} animation;

layout (std140) uniform LightSourceBlock {
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

uniform bool isAnimated;

void main(void) {
    to_fs.textureCoord = textureCoordinate;
    mat4 modelTransform;
    int modelOffset = 4*int(instance.models[gl_InstanceID].x);
    modelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 0), 0);
    modelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 0), 0);
    modelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 0), 0);
    modelTransform[3] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 3, 0), 0);

    mat3 transposeInverseModelTransform;
    transposeInverseModelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 1), 0).xyz;
    transposeInverseModelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 1), 0).xyz;
    transposeInverseModelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 1), 0).xyz;

    if(isAnimated) {
        uint skeletonId = instance.models[gl_InstanceID].z;
        mat4 BoneTransform  = animation.rigs[skeletonId].transform[boneIDs[0]] * boneWeights[0];
             BoneTransform += animation.rigs[skeletonId].transform[boneIDs[1]] * boneWeights[1];
             BoneTransform += animation.rigs[skeletonId].transform[boneIDs[2]] * boneWeights[2];
             BoneTransform += animation.rigs[skeletonId].transform[boneIDs[3]] * boneWeights[3];

        to_fs.normal = normalize(transposeInverseModelTransform * vec3(BoneTransform * vec4(normal, 0.0)));
        to_fs.fragPos = vec3(modelTransform * (BoneTransform * position));
    }
     else {
        to_fs.normal = normalize(transposeInverseModelTransform * normal);
        to_fs.fragPos = vec3(modelTransform * position);
    }
    to_fs.materialIndex = int(instance.models[gl_InstanceID].y);
    gl_Position = playerTransforms.cameraProjection * vec4(to_fs.fragPos, 1.0);
}
