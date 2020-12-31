#version 330 core

layout (location = 0) out vec4 color;

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
} playerTransforms;

in VS_FS {
    vec2 textureCoordinates;
    vec4 colorMultiplier;
} from_vs;

uniform sampler2D sprite;
uniform sampler2D pre_depthMap;


void main(void){
    color = (texture(sprite, from_vs.textureCoordinates));
    color *= from_vs.colorMultiplier;

}
