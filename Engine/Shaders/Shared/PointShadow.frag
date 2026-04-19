#ifndef LIGHT_DEFINITIONS
#define LIGHT_DEFINITIONS
#define_option maximumPointLights
struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type;
	vec3 attenuation;
	vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[maximumPointLights];
} LightSources;
#endif

uniform samplerCubeArray pre_shadowPoint;

vec3 _pointSampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float ShadowCalculationPoint(vec3 world_space_frag_pos, float bias, float viewDistance, int lightIndex)
{
    vec3 fragToLight = world_space_frag_pos - LightSources.lights[lightIndex].position;
    float fragDistance = length(fragToLight);
    if(LightSources.lights[lightIndex].farPlanePoint < fragDistance) {
        return 1.0; // Occluded if beyond the light's far plane
    }

    float closestDepth = texture(pre_shadowPoint, vec4(fragToLight, lightIndex)).r;
    closestDepth *= LightSources.lights[lightIndex].farPlanePoint;

    float currentDepth = fragDistance;
    float shadow = 0.0;
    int samples  = 20;
    float diskRadius = (1.0 + (viewDistance / LightSources.lights[lightIndex].farPlanePoint)) / 50.0;

    for(int i = 0; i < samples; ++i) {
        float pcfDepth = texture(pre_shadowPoint, vec4(fragToLight + _pointSampleOffsetDirections[i] * diskRadius, lightIndex)).r;
        pcfDepth *= LightSources.lights[lightIndex].farPlanePoint;
        if(currentDepth - bias > pcfDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    // Attenuation
    float attenuation = 1.0 / (LightSources.lights[lightIndex].attenuation.x +
                              (LightSources.lights[lightIndex].attenuation.y * fragDistance) +
                               (LightSources.lights[lightIndex].attenuation.z * fragDistance * fragDistance));
    attenuation = clamp(attenuation, 0.0, 1.0);
    attenuation = 1.0 - attenuation;

    // Combine shadow and attenuation
    return max(shadow, attenuation);
}
