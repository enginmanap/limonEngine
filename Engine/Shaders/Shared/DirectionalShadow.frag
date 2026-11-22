#import <./Engine/Shaders/Shared/Lights.glsl>

#define_option shadow_cascadeCount
#define_option shadow_cascadeLimitList
#define_option shadow_directionalSampleCount
#define_option shadow_cascadeBlendFraction
#define_option shadow_normalOffsetMinTexels
#define_option shadow_cascadeFilterRadiusList

uniform sampler2DArrayShadow pre_shadowDirectional;

const float cascadePlaneDistances[shadow_cascadeCount] = float[](shadow_cascadeLimitList);
const float cascadeFilterRadii[shadow_cascadeCount] = float[](shadow_cascadeFilterRadiusList);//OrthographicCamera pads caster culling with these too

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

//World::fillLightBlock writes the cascades in this order
float _ShadowTexelWorldSize(int lightIndex, int layer) {
    if (layer < 4) return vec4(LightSources.lights[lightIndex].pointAttenuationOrShadowTexels, LightSources.lights[lightIndex].pointIntensityOrShadowTexel)[layer];
    if (layer == 4) return LightSources.lights[lightIndex].pointRadiusOrShadowTexel;
    return LightSources.lights[lightIndex].pointFalloffOrShadowTexel;
}

//keep it out of _SampleCascadeShadow, there it is 0.1 ms slower on UHD 630
vec3 _NormalOffsetPosition(int lightIndex, int layer, vec3 world_space_frag_pos, vec3 worldNormal, float sinLightAngle) {
    float normalOffsetTexels = shadow_normalOffsetMinTexels + (cascadeFilterRadii[layer] + 1.0) * sinLightAngle;//less and the farthest tap (+1 for the 2x2 compare) goes under grazing surfaces, acne
    return world_space_frag_pos + worldNormal * (normalOffsetTexels * _ShadowTexelWorldSize(lightIndex, layer));
}

float _SampleCascadeShadow(int lightIndex, int layer, vec3 biasedFragPos, mat2 rot) {
    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(biasedFragPos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float currentDepth = projectedCoordinates.z;
    if (currentDepth >= 1.0) return 0.0;

    vec2 texelSize = 1.0 / vec2(textureSize(pre_shadowDirectional, 0).xy);
    float compareDepth = currentDepth;
    vec2 tapScale = texelSize * cascadeFilterRadii[layer];

    float shadow = 0.0;
    for (int i = 0; i < shadow_directionalSampleCount; ++i) {
        // sampler2DArrayShadow returns 1.0 if not in shadow (compareDepth <= texture_depth), 0.0 if in shadow
        float lit = texture(pre_shadowDirectional, vec4(projectedCoordinates.xy + (rot * _poissonDisk[i]) * tapScale, layer, compareDepth));
        shadow += (1.0 - lit);
    }
    return shadow / float(shadow_directionalSampleCount);
}

float ShadowCalculationDirectional(int lightIndex, vec3 world_space_frag_pos, vec3 worldNormal, float diffuseRate, float precise_view_z){
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

    float sinLightAngle = sqrt(max(1.0 - diffuseRate * diffuseRate, 0.0));
    vec3 biasedFragPos = _NormalOffsetPosition(lightIndex, layer, world_space_frag_pos, worldNormal, sinLightAngle);

    // Calculate rotation matrix once per directional light
    float rotAngle = _random(biasedFragPos, 0) * 6.28318530718;
    float s = sin(rotAngle);
    float c = cos(rotAngle);
    mat2 rot = mat2(c, -s, s, c);

    // Sample the primary cascade
    float shadow = _SampleCascadeShadow(lightIndex, layer, biasedFragPos, rot);

    // Blend with the next cascade if within the transition zone
    if (layer < shadow_cascadeCount - 1) {
        float blendRegion = splitDist * shadow_cascadeBlendFraction;
        float threshold = splitDist - blendRegion;

        if (precise_view_z > threshold) {
            vec3 nextBiasedFragPos = _NormalOffsetPosition(lightIndex, layer + 1, world_space_frag_pos, worldNormal, sinLightAngle);
            float nextShadow = _SampleCascadeShadow(lightIndex, layer + 1, nextBiasedFragPos, rot);
            float factor = (precise_view_z - threshold) / blendRegion;
            factor = smoothstep(0.0, 1.0, factor);
            shadow = mix(shadow, nextShadow, factor);
        }
    }

    return shadow;
}
