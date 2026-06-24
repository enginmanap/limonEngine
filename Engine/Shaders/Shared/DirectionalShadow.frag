#ifndef LIGHT_DEFINITIONS
#define LIGHT_DEFINITIONS
#define_option performance_maximumLights
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
    LightSource lights[performance_maximumLights];
} LightSources;
#endif

#define_option shadow_cascadeCount
#define_option shadow_cascadeLimitList
#define_option shadow_directionalSampleCount

uniform sampler2DArrayShadow pre_shadowDirectional;

const float cascadePlaneDistances[shadow_cascadeCount] = float[](shadow_cascadeLimitList);

// Vogel Spiral Disk: Inherently progressive (any N prefix is well-distributed)
// Prevents shadow crawling when changing sample counts (2, 4, 8, 16)
const vec2 _poissonDisk[16] = vec2[](
    vec2(0.176461, 0.000000),
    vec2(-0.225549, 0.207521),
    vec2(-0.038166, -0.393450),
    vec2(0.448074, 0.133241),
    vec2(-0.525541, 0.073400),
    vec2(0.301986, -0.502856),
    vec2(0.096338, 0.629851),
    vec2(-0.520445, -0.444280),
    vec2(0.720896, 0.111956),
    vec2(-0.536104, 0.552277),
    vec2(0.030582, -0.809424),
    vec2(0.596001, 0.604112),
    vec2(-0.878486, -0.099182),
    vec2(0.718361, -0.573210),
    vec2(-0.210086, 0.927429),
    vec2(-0.472145, -0.863390)
);

float _random(vec3 seed, int i){
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float _SampleCascadeShadow(int lightIndex, int layer, vec3 world_space_frag_pos, mat2 rot) {
    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(world_space_frag_pos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float currentDepth = projectedCoordinates.z;
    if (currentDepth >= 1.0) return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(pre_shadowDirectional, 0).xy);
    float filterRadius = 2.0 + float(layer) * 0.5;
    float compareDepth = currentDepth;

    // Early-out: center sample is fully lit, skip full PCF loop
    if(texture(pre_shadowDirectional, vec4(projectedCoordinates.xy, layer, compareDepth)) == 1.0) {
        return 0.0;
    }

    for(int i = 0; i < shadow_directionalSampleCount; ++i){
        vec2 offset = rot * _poissonDisk[i];
        // sampler2DArrayShadow returns 1.0 if not in shadow (compareDepth <= texture_depth), 0.0 if in shadow
        float lit = texture(pre_shadowDirectional, vec4(projectedCoordinates.xy + offset * texelSize * filterRadius, layer, compareDepth));
        shadow += (1.0 - lit);
    }
    return shadow / float(shadow_directionalSampleCount);
}

float ShadowCalculationDirectional(int lightIndex, vec3 world_space_frag_pos, float precise_view_z){
    int layer = -1;
    float splitDist = 0.0;

    for (int i = 0; i < shadow_cascadeCount; ++i) {
        if (precise_view_z < cascadePlaneDistances[i]) {
            layer = i;
            splitDist = cascadePlaneDistances[i];
            break;
        }
    }
    if (layer == -1) layer = shadow_cascadeCount - 1;

    // Calculate rotation matrix once per directional light
    float rotAngle = _random(world_space_frag_pos, 0) * 6.28318530718;
    float s = sin(rotAngle);
    float c = cos(rotAngle);
    mat2 rot = mat2(c, -s, s, c);

    // Sample the primary cascade
    float shadow = _SampleCascadeShadow(lightIndex, layer, world_space_frag_pos, rot);

    // Blend with the next cascade if within the transition zone
    if (layer < shadow_cascadeCount - 1) {
        float blendRegion = splitDist * 0.10;
        float threshold = splitDist - blendRegion;

        if (precise_view_z > threshold) {
            float nextShadow = _SampleCascadeShadow(lightIndex, layer + 1, world_space_frag_pos, rot);
            float factor = (precise_view_z - threshold) / blendRegion;
            factor = smoothstep(0.0, 1.0, factor);
            shadow = mix(shadow, nextShadow, factor);
        }
    }

    return shadow;
}
