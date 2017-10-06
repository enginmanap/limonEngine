#version 330

#define NR_POINT_LIGHTS 4
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
    vec4 fragPosLightSpace[NR_POINT_LIGHTS];
} to_fs;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
} playerTransforms;

struct LightSource
{
		mat4 lightSpaceMatrix;
		vec3 position;
		vec3 color;
		int type; //0 Directional, 1 point
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

uniform mat4 boneTransformArray[NR_BONE];
uniform mat4 worldTransformMatrix;


void main(void)
{

    mat4 BoneTransform = boneTransformArray[boneIDs[0]] * boneWeights[0];
    BoneTransform += boneTransformArray[boneIDs[1]] * boneWeights[1];
    BoneTransform += boneTransformArray[boneIDs[2]] * boneWeights[2];
    BoneTransform += boneTransformArray[boneIDs[3]] * boneWeights[3];

    to_fs.textureCoord = textureCoordinate;
    to_fs.normal = vec3(normalize(transpose(inverse(worldTransformMatrix)) * (BoneTransform * vec4(normal, 0.0))));
    to_fs.fragPos = vec3(worldTransformMatrix * (BoneTransform * position));
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(LightSources.lights[i].type == 0) {
            to_fs.fragPosLightSpace[i] = LightSources.lights[i].lightSpaceMatrix * vec4(to_fs.fragPos, 1.0);
        }
    }
    //The transform is calculated twice, it can be reused from to_fs.fragPos
    gl_Position = playerTransforms.cameraProjection * (worldTransformMatrix * (BoneTransform * position));
}
