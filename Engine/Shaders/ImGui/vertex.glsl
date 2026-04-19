
#define_option maximumPointLights
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/ModelRendering.vert>

uniform mat4 ProjMtx;
layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;
layout (location = 5) in vec2 PositionIMGUI;
layout (location = 6) in vec2 UV;
layout (location = 7) in vec4 Color;
layout (location = 8) in uvec4 boneIDs;
layout (location = 9) in vec4 boneWeights;
out vec2 Frag_UV;
out vec4 Frag_Color;

/** Model rendering definitions */

out VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[maximumPointLights];
    flat int depthMapLayer;
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

layout (std140) uniform LightSourceBlock {
    LightSource lights[maximumPointLights];
} LightSources;

/** Model rendering definitions */

uniform int renderModelIMGUI;

vec4 renderModel() {
    to_fs.textureCoord = textureCoordinate;

    calculateWorldPositionAndNormal(position, normal, boneIDs, boneWeights, to_fs.fragPos, to_fs.normal);

    to_fs.materialIndex = int(instance.models[gl_InstanceID].y);
    vec3 temp = (playerTransforms.position - vec3(position));
    if(sqrt(dot(temp, temp)) > 10.0) {
        to_fs.depthMapLayer = 1;
    } else {
        to_fs.depthMapLayer = 0;
    }
    for(int i = 0; i < maximumPointLights; i++){
        if(LightSources.lights[i].type == 1) {
            to_fs.fragPosLightSpace[i] = LightSources.lights[i].shadowMatrices[to_fs.depthMapLayer] * vec4(to_fs.fragPos, 1.0);
        }
    }
    return playerTransforms.cameraProjection * vec4(to_fs.fragPos, 1.0);
}

void main() {
    if(renderModelIMGUI == 0) {
        Frag_UV = UV;
        Frag_Color = Color;
        gl_Position = ProjMtx * vec4(PositionIMGUI.xy, 0, 1);
    } else {
        gl_Position = renderModel();
    }
}
