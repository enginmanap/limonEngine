#version 330
uniform mat4 ProjMtx;
layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;
layout (location = 5) in vec2 PositionIMGUI;
layout (location = 6) in vec2 UV;
layout (location = 7) in vec4 Color;
out vec2 Frag_UV;
out vec4 Frag_Color;

/** Model rendering definitions */

#define NR_POINT_LIGHTS 4
#define NR_MAX_MODELS 4096
#define NR_MAX_MATERIALS 200

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

layout (std140) uniform LightSourceBlock {
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

/** Model rendering definitions */

uniform int renderModelIMGUI;

vec4 renderModel() {

    // c/p from Model/vertex.glsl
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

    to_fs.materialIndex = int(instance.models[gl_InstanceID].y);
    to_fs.normal = normalize(transposeInverseModelTransform * normal);
    to_fs.fragPos = vec3(modelTransform * position);
    vec3 temp = (playerTransforms.position - vec3(position));
    if(sqrt(dot(temp, temp)) > 10) {
        to_fs.depthMapLayer = 1;
    } else {
        to_fs.depthMapLayer = 0;
    }
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
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