#version 330 core

#define NR_POINT_LIGHTS 4

layout (location = 2) in vec4 position;

struct LightSource
{
		mat4 lightSpaceMatrix;
		vec3 position;
		vec3 color;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

uniform mat4 worldTransformMatrix;

void main()
{
    //FIXME this forces single light
    gl_Position = LightSources.lights[0].lightSpaceMatrix * (worldTransformMatrix * position);
}