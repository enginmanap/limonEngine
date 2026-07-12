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

// Branchless orthonormal basis from a single unit vector (Duff et al. 2017, "Building an Orthonormal
// Basis, Revisited"). Used to jitter PCF taps along the receiver surface's own tangent plane.
void _buildTangentBasis(vec3 n, out vec3 b1, out vec3 b2) {
    float sign_ = n.z >= 0.0 ? 1.0 : -1.0;
    float a = -1.0 / (sign_ + n.z);
    float b = n.x * n.y * a;
    b1 = vec3(1.0 + sign_ * n.x * n.x * a, sign_ * b, -sign_ * n.x);
    b2 = vec3(b, sign_ + n.y * n.y * a, -n.y);
}

float _SampleCascadeShadow(int lightIndex, int layer, vec3 world_space_frag_pos, vec3 worldNormal, mat2 rot) {
    mat4 shadowMatrix = LightSources.lights[lightIndex].shadowMatrices[layer];
    vec4 fragPosLightSpace = shadowMatrix * vec4(world_space_frag_pos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float currentDepth = projectedCoordinates.z;
    if (currentDepth >= 1.0) return 0.0;

    float filterRadius = 4.0 + float(layer) * 0.5;
    vec2 texelSize = 1.0 / vec2(textureSize(pre_shadowDirectional, 0).xy);

    // Analytic receiver-plane depth: PCF taps are still offset in real shadow-map texels via
    // texelSize/filterRadius, exactly as before -- that part was already correctly calibrated and is
    // what fixed the stair-stepping. What's fixed here is the compare depth: instead of reusing one
    // compareDepth for the whole kernel (which caused acne once the radius got wide enough for a
    // sloped surface's true depth to diverge from it noticeably), solve for the depth a point *on the
    // receiver's own tangent plane* would have at each tap's UV offset. The shadow projection is
    // linear (orthographic), so a world-space step d maps to light-space (du,dv,dz) via
    // (shadowMatrix * vec4(d,0)).xyz * 0.5. Building that mapping for the tangent/bitangent directions
    // gives an invertible 2x2 system -- no guessed world-space texel size needed anywhere.
    vec3 tangent, bitangent;
    _buildTangentBasis(worldNormal, tangent, bitangent);
    vec3 dT = (shadowMatrix * vec4(tangent, 0.0)).xyz * 0.5;
    vec3 dB = (shadowMatrix * vec4(bitangent, 0.0)).xyz * 0.5;
    float det = dT.x * dB.y - dB.x * dT.y;
    // Degenerate only if the tangent plane projects edge-on into the light (near-zero area in light
    // space); guard against divide-by-zero/NaN in that rare case by falling back to no depth slope.
    float invDet = (abs(det) > 1e-9) ? (1.0 / det) : 0.0;

    float shadow = 0.0;
    for(int i = 0; i < shadow_directionalSampleCount; ++i){
        vec2 offset = (rot * _poissonDisk[i]) * texelSize * filterRadius;
        float a = (dB.y * offset.x - dB.x * offset.y) * invDet;
        float b = (dT.x * offset.y - dT.y * offset.x) * invDet;
        float tapDepth = currentDepth + a * dT.z + b * dB.z;
        // sampler2DArrayShadow returns 1.0 if not in shadow (tap depth <= texture_depth), 0.0 if in shadow
        float lit = texture(pre_shadowDirectional, vec4(projectedCoordinates.xy + offset, layer, tapDepth));
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

    // we calculate normal bias after cascade selection, so we can scale it based on the cascade.
    // The bias needed for first cascade is tiny, compared to last cascade. We would need to pass
    // texel size to calculate it correctly, this is an approxmate.
    float baseNormalBias = mix(0.01, 0.100, diffuseRate);
    vec3 biasedFragPos = world_space_frag_pos + worldNormal * (baseNormalBias * (cascadePlaneDistances[layer] / cascadePlaneDistances[0]));

    // Calculate rotation matrix once per directional light
    float rotAngle = _random(biasedFragPos, 0) * 6.28318530718;
    float s = sin(rotAngle);
    float c = cos(rotAngle);
    mat2 rot = mat2(c, -s, s, c);

    // Sample the primary cascade
    float shadow = _SampleCascadeShadow(lightIndex, layer, biasedFragPos, worldNormal, rot);

    // Blend with the next cascade if within the transition zone
    if (layer < shadow_cascadeCount - 1) {
        float blendRegion = splitDist * 0.10;
        float threshold = splitDist - blendRegion;

        if (precise_view_z > threshold) {
            // layer+1 has a larger cascade scale (coarser texels) than layer, so it needs its own,
            // bigger bias here -- reusing biasedFragPos (scaled for layer) would under-bias this
            // sample and cause acne right in the blend band.
            vec3 nextBiasedFragPos = world_space_frag_pos + worldNormal * (baseNormalBias * (cascadePlaneDistances[layer + 1] / cascadePlaneDistances[0]));
            float nextShadow = _SampleCascadeShadow(lightIndex, layer + 1, nextBiasedFragPos, worldNormal, rot);
            float factor = (precise_view_z - threshold) / blendRegion;
            factor = smoothstep(0.0, 1.0, factor);
            shadow = mix(shadow, nextShadow, factor);
        }
    }

    return shadow;
}
