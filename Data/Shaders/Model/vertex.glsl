#version 330

#define NR_POINT_LIGHTS 4

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;

out VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[NR_POINT_LIGHTS];
} to_fs;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    vec3 position;
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

uniform mat4 worldTransformMatrix;

void main(void)
{
    to_fs.textureCoord = textureCoordinate;
    to_fs.normal = normalize(mat3(transpose(inverse(worldTransformMatrix))) * normal);
    to_fs.fragPos = vec3(worldTransformMatrix * position);
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(LightSources.lights[i].type == 0) {
            to_fs.fragPosLightSpace[i] = LightSources.lights[i].lightSpaceMatrix * vec4(to_fs.fragPos, 1.0);
        }
    }
    gl_Position = playerTransforms.cameraProjection * (worldTransformMatrix * position);
}
