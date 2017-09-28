#version 330 core

#define NR_POINT_LIGHTS 4
#define NR_BONE 128

layout (location = 2) in vec4 position;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

struct LightSource
{
		mat4 lightSpaceMatrix;
		vec3 position;
		vec3 color;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

uniform mat4 boneTransformArray[NR_BONE];
uniform mat4 worldTransformMatrix;
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
            gl_Position = LightSources.lights[i].lightSpaceMatrix * (worldTransformMatrix * (BoneTransform * vec4(vec3(position), 1.0)));
        }
    }
}