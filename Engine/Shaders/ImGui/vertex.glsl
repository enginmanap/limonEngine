
#import <./Engine/Shaders/Shared/Lights.glsl>
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/ModelRendering.vert>

uniform mat4 ProjMtx;
layout (location = 2) in vec4 position;         // hard coded slot
layout (location = 3) in vec2 textureCoordinate;// hard coded slot
layout (location = 4) in vec3 normal;           // hard coded slot
layout (location = 5) in uvec4 boneIDs;         // hard coded slot
layout (location = 6) in vec4 boneWeights;      // hard coded slot
layout (location = 8) in vec2 PositionIMGUI;    // Dynamic slot
layout (location = 9) in vec2 UV;               // Dynamic slot
layout (location = 10) in vec4 Color;           // Dynamic slot
out vec2 Frag_UV;
out vec4 Frag_Color;

/** Model rendering definitions */

out VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[performance_maximumLights];
    flat int depthMapLayer;
    flat int materialIndex;
} to_fs;

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
    for(int i = 0; i < performance_maximumLights; i++){
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
        // Force editor to nearest depth so it always wins against GUI layer depths [0, 0.019]
        gl_Position.z = -gl_Position.w;
    } else {
        gl_Position = renderModel();
    }
}
