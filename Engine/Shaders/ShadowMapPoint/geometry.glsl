#version 330 core

#define NR_POINT_LIGHTS 4

layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

struct LightSource {
    mat4 shadowMatrices[6];
    mat4 lightSpaceMatrix;
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type;
    vec3 attenuation;
    vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

uniform int renderLightIndex;

out vec4 FragPos; // FragPos from GS (output per emitvertex)

void main()
{
    for(int face = 0; face < 6; ++face)
    {
        gl_Layer = renderLightIndex*6+face; // built-in variable that specifies to which face we render.
        for(int i = 0; i < 3; ++i) // for each triangle's vertices
        {
            FragPos = gl_in[i].gl_Position;
            gl_Position = LightSources.lights[renderLightIndex].shadowMatrices[face] * FragPos;
            EmitVertex();
        }
        EndPrimitive();
    }
}