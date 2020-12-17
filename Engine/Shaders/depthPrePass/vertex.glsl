#version 330 core

#define NR_POINT_LIGHTS 4
#define NR_BONE 128
#define NR_MAX_MODELS 1000


layout (location = 2) in vec4 position;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

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

uniform sampler2D allModelTransformsTexture;

layout (std140) uniform ModelIndexBlock {
    uvec4 models[NR_MAX_MODELS];
} instance;

uniform mat4 boneTransformArray[NR_BONE];
uniform int isAnimated;

void main() {
    mat4 modelTransform;
    int modelOffset = 4*int(instance.models[gl_InstanceID].x);
    modelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 0), 0);
    modelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 0), 0);
    modelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 0), 0);
    modelTransform[3] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 3, 0), 0);

    if(isAnimated==1) {
        mat4 BoneTransform = mat4(1.0);
        BoneTransform = boneTransformArray[boneIDs[0]] * boneWeights[0];
        BoneTransform += boneTransformArray[boneIDs[1]] * boneWeights[1];
        BoneTransform += boneTransformArray[boneIDs[2]] * boneWeights[2];
        BoneTransform += boneTransformArray[boneIDs[3]] * boneWeights[3];

        gl_Position = playerTransforms.cameraProjection * (modelTransform * (BoneTransform * vec4(vec3(position), 1.0)));
    } else {
        gl_Position = playerTransforms.cameraProjection * (modelTransform * position);
    }
}