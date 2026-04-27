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

#define_option PointShadowSampleCount

uniform samplerCubeArrayShadow pre_shadowPoint;

// 3D Fibonacci Spiral (Progressive): any prefix N is well-distributed on a sphere.
// Prevents shadow crawling when changing PointShadowSampleCount.
vec3 _pointSampleOffsetDirections[20] = vec3[](
    vec3(0.000000, 0.000000, 1.000000),
    vec3(0.254181, 0.231940, 0.938883),
    vec3(-0.354146, 0.353381, 0.865917),
    vec3(0.187313, -0.612053, 0.768254),
    vec3(0.165684, 0.730303, 0.662660),
    vec3(-0.536761, -0.528406, 0.551689),
    vec3(0.728956, 0.081829, 0.435773),
    vec3(-0.640243, 0.457813, 0.315004),
    vec3(0.288289, -0.803874, 0.190333),
    vec3(0.192537, 0.884501, 0.063162),
    vec3(-0.627690, -0.669868, -0.065582),
    vec3(0.852445, 0.197992, -0.195000),
    vec3(-0.808080, 0.407987, -0.323871),
    vec3(0.505085, -0.814324, -0.450337),
    vec3(-0.024520, 0.942732, -0.573516),
    vec3(-0.463287, -0.738155, -0.690858),
    vec3(0.781845, 0.247489, -0.800918),
    vec3(-0.854085, 0.284752, -0.902340),
    vec3(0.686524, -0.638426, -0.993043),
    vec3(-0.334053, 0.749007, -1.071661)
);

float ShadowCalculationPoint(vec3 world_space_frag_pos, float bias, float viewDistance, int lightIndex)
{
    vec3 fragToLight = world_space_frag_pos - LightSources.lights[lightIndex].position;
    float fragDistance = length(fragToLight);
    if(LightSources.lights[lightIndex].farPlanePoint < fragDistance) {
        return 1.0; // Occluded if beyond the light's far plane
    }

    // The shadow map stores distance normalized by farPlanePoint
    float normalizedFragDistance = fragDistance / LightSources.lights[lightIndex].farPlanePoint;

    // Sample the closest depth (hardware comparison returns 1.0 if normalizedFragDistance <= texture_depth)
    float closestDepthLit = texture(pre_shadowPoint, vec4(fragToLight, lightIndex), normalizedFragDistance);

    float currentDepth = fragDistance;
    float shadow = 0.0;
    int samples  = PointShadowSampleCount;
    float diskRadius = (1.0 + (viewDistance / LightSources.lights[lightIndex].farPlanePoint)) / 50.0;
    float compareDepth = (currentDepth - bias) / LightSources.lights[lightIndex].farPlanePoint;

    for(int i = 0; i < samples; ++i) {
        // samplerCubeArrayShadow returns 1.0 if not in shadow (compareDepth <= texture_depth), 0.0 if in shadow
        float lit = texture(pre_shadowPoint, vec4(fragToLight + _pointSampleOffsetDirections[i] * diskRadius, lightIndex), compareDepth);
        shadow += (1.0 - lit);
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
