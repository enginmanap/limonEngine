#version 330

#define NR_POINT_LIGHTS 4
#define NR_MAX_MODELS 4096

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;

out VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[NR_POINT_LIGHTS];
    flat int depthMapLayer;
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

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

void main(void)
{
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


    to_fs.normal = normalize(transposeInverseModelTransform * normal);
    to_fs.fragPos = vec3(modelTransform * position);
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(LightSources.lights[i].type == 1) {
            to_fs.fragPosLightSpace[i] = LightSources.lights[i].shadowMatrices[0] * vec4(to_fs.fragPos, 1.0);
        }
    }
    to_fs.materialIndex = int(instance.models[gl_InstanceID].y);
    gl_Position = playerTransforms.cameraProjection * (modelTransform * position);
}
