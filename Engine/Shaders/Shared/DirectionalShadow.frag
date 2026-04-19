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

#define_option CascadeCount
#define_option CascadeLimitList

uniform sampler2DArray pre_shadowDirectional;

vec2 _poissonDisk[16] = vec2[](
   vec2( -0.94201624, -0.39906216 ),
   vec2( 0.94558609, -0.76890725 ),
   vec2( -0.094184101, -0.92938870 ),
   vec2( 0.34495938, 0.29387760 ),
   vec2( -0.91588581, 0.45771432 ),
   vec2( -0.81544232, -0.87912464 ),
   vec2( -0.38277543, 0.27676845 ),
   vec2( 0.97484398, 0.75648379 ),
   vec2( 0.44323325, -0.97511554 ),
   vec2( 0.53742981, -0.47373420 ),
   vec2( -0.26496911, -0.41893023 ),
   vec2( 0.79197514, 0.19090188 ),
   vec2( -0.24188840, 0.99706507 ),
   vec2( -0.81409955, 0.91437590 ),
   vec2( 0.19984126, 0.78641367 ),
   vec2( 0.14383161, -0.14100790 )
);

float _random(vec3 seed, int i){
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float _SampleCascadeShadow(int lightIndex, int layer, vec3 world_space_frag_pos, float depthBias) {
    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(world_space_frag_pos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float currentDepth = projectedCoordinates.z;
    if (currentDepth >= 1.0) return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(pre_shadowDirectional, 0).xy);
    float filterRadius = 2.0 + float(layer) * 0.5;

    float rotAngle = _random(world_space_frag_pos, 0) * 6.28318530718;
    float s = sin(rotAngle);
    float c = cos(rotAngle);
    mat2 rot = mat2(c, -s, s, c);

    for(int i = 0; i < 16; ++i){
        vec2 offset = rot * _poissonDisk[i];
        float pcfDepth = texture(pre_shadowDirectional, vec3(projectedCoordinates.xy + offset * texelSize * filterRadius, layer)).r;
        if(currentDepth - depthBias > pcfDepth) {
            shadow += 1.0;
        }
    }
    return shadow / 16.0;
}

float ShadowCalculationDirectional(int lightIndex, vec3 world_space_frag_pos, float precise_view_z, vec3 normal){
    float cascadePlaneDistances[CascadeCount] = float[](CascadeLimitList);

    int layer = -1;
    float splitDist = 0.0;

    for (int i = 0; i < CascadeCount; ++i) {
        if (precise_view_z < cascadePlaneDistances[i]) {
            layer = i;
            splitDist = cascadePlaneDistances[i];
            break;
        }
    }
    if (layer == -1) layer = CascadeCount - 1;

    // Apply a slope-scaled normal offset bias in world space
    vec3 lightDirectory = normalize(-LightSources.lights[lightIndex].position);
    float NdotL = max(dot(normal, lightDirectory), 0.0);
    float world_bias = max(1.5 * (1.0 - NdotL), 0.05);
    vec3 biased_fragPos = world_space_frag_pos + normal * world_bias;

    // Sample the primary cascade
    float shadow = _SampleCascadeShadow(lightIndex, layer, biased_fragPos, 0.0);

    // Blend with the next cascade if within the transition zone
    if (layer < CascadeCount - 1) {
        float blendRegion = splitDist * 0.10; // 10% overlap
        float threshold = splitDist - blendRegion;

        if (precise_view_z > threshold) {
            float nextShadow = _SampleCascadeShadow(lightIndex, layer + 1, biased_fragPos, 0.0);
            float factor = (precise_view_z - threshold) / blendRegion;
            factor = smoothstep(0.0, 1.0, factor);
            shadow = mix(shadow, nextShadow, factor);
        }
    }

    return shadow;
}
