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
uniform int renderLightIndex;

void main()
{
    for(int i = 0; i < NR_POINT_LIGHTS; i++){
        if(i == renderLightIndex){
            gl_Position = LightSources.lights[i].lightSpaceMatrix * (worldTransformMatrix * position);
        }
    }
}