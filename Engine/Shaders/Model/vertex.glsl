#version 330

#define NR_POINT_LIGHTS 4
#define NR_MAX_MODELS 1000

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;

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
    mat4 inverseProjection;
	mat4 inverseCamera;
    vec3 position;
	vec3 cameraSpacePosition;
    vec2 noiseScale;
} playerTransforms;

struct LightSource {
    mat4 shadowMatrices[6];
    mat4 lightSpaceMatrix;
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type; //1 Directional, 2 point
	vec3 attenuation;
	vec3 ambient;
};

layout (std140) uniform ModelInformationBlock {
    mat4 worldTransform[NR_MAX_MODELS];
} model;

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
    mat4 currentWorldTransform = model.worldTransform[instance.models[gl_InstanceID].x];
    to_fs.normal = normalize(mat3(transpose(inverse(currentWorldTransform))) * normal);
    to_fs.fragPos = vec3(currentWorldTransform * position);
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(LightSources.lights[i].type == 1) {
            to_fs.fragPosLightSpace[i] = LightSources.lights[i].lightSpaceMatrix * vec4(to_fs.fragPos, 1.0);
        }
    }
    gl_Position = playerTransforms.cameraProjection * (currentWorldTransform * position);
}
