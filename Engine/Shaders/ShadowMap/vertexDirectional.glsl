#version 330 core

#define NR_POINT_LIGHTS 4
#define NR_BONE 128
#define NR_MAX_MODELS 1000


layout (location = 2) in vec4 position;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

struct LightSource {
    mat4 shadowMatrices[6];
    mat4 lightSpaceMatrix;
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

layout (std140) uniform MaterialInformationBlock {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    int isMap; 	//using the last 4, ambient=8, diffuse=4, specular=2, opacity = 1
} material;

layout (std140) uniform ModelInformationBlock {
    mat4 worldTransform[NR_MAX_MODELS];
} model;

layout (std140) uniform ModelIndexBlock {
    uvec4 models[NR_MAX_MODELS];
} instance;

uniform mat4 boneTransformArray[NR_BONE];
uniform int renderLightIndex;
uniform int isAnimated;

void main() {

    mat4 BoneTransform = mat4(1.0);
    if(isAnimated==1) {
         BoneTransform = boneTransformArray[boneIDs[0]] * boneWeights[0];
         BoneTransform += boneTransformArray[boneIDs[1]] * boneWeights[1];
         BoneTransform += boneTransformArray[boneIDs[2]] * boneWeights[2];
         BoneTransform += boneTransformArray[boneIDs[3]] * boneWeights[3];
    }
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(i == renderLightIndex){
            gl_Position = LightSources.lights[i].lightSpaceMatrix * (model.worldTransform[instance.models[gl_InstanceID].x] * (BoneTransform * vec4(vec3(position), 1.0)));
        }
    }
}