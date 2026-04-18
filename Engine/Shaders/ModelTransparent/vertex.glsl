
#define_option maximumPointLights
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/ModelRendering.vertex>

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

out VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[maximumPointLights];
    flat int materialIndex;
} to_fs;

struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type; //1 Directional, 2 point
	vec3 attenuation;
	vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[maximumPointLights];
} LightSources;

void main(void)
{
    to_fs.textureCoord = textureCoordinate;

    calculateWorldPositionAndNormal(position, normal, boneIDs, boneWeights, to_fs.fragPos, to_fs.normal);

    for(int i = 0; i < maximumPointLights; i++){
        if(LightSources.lights[i].type == 1) {
            to_fs.fragPosLightSpace[i] = LightSources.lights[i].shadowMatrices[0] * vec4(to_fs.fragPos, 1.0);
        }
    }
    to_fs.materialIndex = int(instance.models[gl_InstanceID].y);
    gl_Position = playerTransforms.cameraProjection * vec4(to_fs.fragPos, 1.0);
}
