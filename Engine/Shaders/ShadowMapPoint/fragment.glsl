#import <./Engine/Shaders/Shared/Lights.glsl>

in vec4 FragPos;

uniform int renderLightIndex;


void main()
{
    float lightDistance = length(FragPos.xyz - LightSources.lights[renderLightIndex].position);
    lightDistance = lightDistance / LightSources.lights[renderLightIndex].radius;
    gl_FragDepth = lightDistance;
}  