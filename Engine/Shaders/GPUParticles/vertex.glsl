#version 330 core

layout (location = 2) in vec3 position;
layout (location = 3) in vec2 textureCoordinates;

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

out VS_FS {
    vec2 textureCoordinates;
    vec4 colorMultiplier;
} to_fs;

uniform sampler2D positions;
uniform float size;
uniform vec3 gravity;
uniform bool continuousEmit;
uniform float lifeTime;

vec4 unpackFloat(float value) {
    uint rgba = floatBitsToUint(value);
    float a = float(rgba >> 24) / 255.0;
    float b = float((rgba & 0x00ff0000u) >> 16) / 255.0;
    float g = float((rgba & 0x0000ff00u) >> 8) / 255.0;
    float r = float(rgba & 0x000000ffu) / 255.0;
    return vec4(r, g, b, a);
}

void main(){
    to_fs.textureCoordinates = textureCoordinates;
    vec4 positionFromTexel = texelFetch(positions, ivec2(gl_InstanceID, 0), 0);
    float creationTime = positionFromTexel.w;
    vec4 worldPosition = positionFromTexel;
    worldPosition.w = 1.0;
    vec4 speed = texelFetch(positions, ivec2(gl_InstanceID, 1), 0);
    speed = 2.0 * speed;

    float destroyTime = speed.w;
    if(playerTransforms.time < creationTime) {
        gl_Position = vec4(9,9,9,1);//outside of ndc
        return;
    }

    float msSinceStart = mod((playerTransforms.time - creationTime), lifeTime);

    //speed changes with gravity, calculate total
    float framecount = msSinceStart / (1000.0/ 60.0);
    vec4 lastSpeed = framecount * vec4(gravity / ((1000.0/ 60.0)), 0.0 + speed);
//  vec4 totalChange = ((lastSpeed + speed) * (lastSpeed - speed + vec4(gravity, 0.0))) / (2.0 * vec4(gravity, 1.0));
    //vec4 totalChange = ((lastSpeed + speed) * (lastSpeed - speed)) / (2.0 * vec4(gravity, 1.0));// gravity has 0 in it, thats why it doesn't work
    vec4 totalChange = ((lastSpeed + speed) * framecount) / 2.0;
    totalChange.w = 0.0;

    //to_fs.colorMultiplier = totalChange;//temporary hack
    to_fs.colorMultiplier = speed;
    to_fs.colorMultiplier.w = 1.0;

    //totalChange = (((2.0* speed) + vec4(gravity, 0.0)) * msSinceStart / 60.0) * framecount;
    //totalChange = (0.005* speed) * framecount;

    worldPosition = worldPosition + totalChange;
    vec4 cameraCenterPosition = playerTransforms.cameraProjection * worldPosition;
    cameraCenterPosition.xyz = cameraCenterPosition.xyz + (position.xyz * size);//position is centered.

    //to_fs.colorMultiplier = vec4(1.0, 1.0, 1.0, 1.0);//temporary hack
    gl_Position = cameraCenterPosition;
}