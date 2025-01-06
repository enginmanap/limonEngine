#version 330 core

#define NR_POINT_LIGHTS 4
#define NR_BONE 128
#define NR_MAX_MODELS 4096


layout (location = 2) in vec4 position;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type;
    vec3 attenuation;
    vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;


uniform sampler2D allModelTransformsTexture;
uniform sampler2D allBoneTransformsTexture;

layout (std140) uniform ModelIndexBlock {
    uvec4 models[NR_MAX_MODELS];
} instance;

uniform int renderLightIndex;
uniform int isAnimated;

mat4 getMatrixFromRigTexture(uint rigIndex, uint boneIndex) {
    mat4 matrix;
    matrix[0] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u)    , int(rigIndex)), 0);
    matrix[1] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 1, int(rigIndex)), 0);
    matrix[2] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 2, int(rigIndex)), 0);
    matrix[3] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 3, int(rigIndex)), 0);
    return matrix;
}

void main() {

    mat4 BoneTransform = mat4(1.0);
    if(isAnimated==1) {
        uint skeletonId = instance.models[gl_InstanceID].z;
        BoneTransform  = getMatrixFromRigTexture(skeletonId, boneIDs[0]) * boneWeights[0];
        BoneTransform += getMatrixFromRigTexture(skeletonId, boneIDs[1]) * boneWeights[1];
        BoneTransform += getMatrixFromRigTexture(skeletonId, boneIDs[2]) * boneWeights[2];
        BoneTransform += getMatrixFromRigTexture(skeletonId, boneIDs[3]) * boneWeights[3];
    }

    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(i == renderLightIndex){
            mat4 modelTransform;
            int modelOffset = 4*int(instance.models[gl_InstanceID].x);
            modelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 0), 0);
            modelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 0), 0);
            modelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 0), 0);
            modelTransform[3] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 3, 0), 0);
            gl_Position = modelTransform * (BoneTransform * vec4(vec3(position), 1.0));
        }
    }
}