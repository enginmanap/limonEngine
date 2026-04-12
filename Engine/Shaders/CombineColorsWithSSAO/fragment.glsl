

#define_option CascadeCount
#define_option CascadeLimitList

#define NR_POINT_LIGHTS 4
#define NR_MAX_MATERIALS 200

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

// G-Buffer inputs
uniform sampler2D pre_gNormalMap;      // World-space normal, packed
uniform sampler2D pre_gAlbedoSpecMap;  // Albedo (rgb), Material Index (a)
uniform sampler2D pre_gAmbientMap;     // Ambient Map (rgb)
uniform sampler2D pre_ssao;
uniform sampler2D pre_depthMap;
uniform sampler2DArray pre_shadowDirectional;
uniform samplerCubeArray pre_shadowPoint;

struct material {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    int isMap;
};

layout (std140) uniform MaterialInformationBlock {
    material materials[NR_MAX_MATERIALS];
} AllMaterialsArray;

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    mat4 inverseProjection;
    mat4 inverseCamera;
    mat3 transposeInverseCamera;
    vec3 position; // Camera world position
    vec3 cameraSpacePosition;
    vec2 noiseScale;
    int time;
} playerTransforms;

struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position; // World-space position/direction
    float farPlanePoint;
    vec3 color;
    int type; //1 Directional, 2 point
	vec3 attenuation;
	vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

vec3 pointSampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

// Poisson Disk samples PCF
vec2 poissonDisk[16] = vec2[](
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

vec3 ReconstructWorldPos(vec2 texCoords, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = playerTransforms.inverseProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = playerTransforms.inverseCamera * viewSpacePosition;
    return worldSpacePosition.xyz;
}

float random(vec3 seed, int i){
    vec4 seed4 = vec4(seed, i);
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float SampleCascadeShadow(int lightIndex, int layer, vec3 world_space_frag_pos, float depthBias) {
    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(world_space_frag_pos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float currentDepth = projectedCoordinates.z;
    if (currentDepth >= 1.0) return 0.0;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(pre_shadowDirectional, 0).xy);

    float filterRadius = 2.0 + float(layer) * 0.5;

    float rotAngle = random(world_space_frag_pos, 0) * 6.28318530718;
    float s = sin(rotAngle);
    float c = cos(rotAngle);
    mat2 rot = mat2(c, -s, s, c);

    for(int i = 0; i < 16; ++i){
        vec2 offset = rot * poissonDisk[i];
        float pcfDepth = texture(pre_shadowDirectional, vec3(projectedCoordinates.xy + offset * texelSize * filterRadius, layer)).r;
        if(currentDepth - depthBias > pcfDepth) {
            shadow += 1.0;
        }
    }
    return shadow / 16.0;
}

float ShadowCalculationDirectional(float depthBias, int lightIndex, vec3 world_space_frag_pos, float precise_view_z){
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

    // Sample the primary cascade
    float shadow = SampleCascadeShadow(lightIndex, layer, world_space_frag_pos, depthBias);

    // Blend with the next cascade if within the transition zone
    if (layer < CascadeCount - 1) {
        float blendRegion = splitDist * 0.10; // 10% overlap
        float threshold = splitDist - blendRegion;

        if (precise_view_z > threshold) {
            float nextShadow = SampleCascadeShadow(lightIndex, layer + 1, world_space_frag_pos, depthBias);
            float factor = (precise_view_z - threshold) / blendRegion;
            factor = smoothstep(0.0, 1.0, factor);
            shadow = mix(shadow, nextShadow, factor);
        }
    }

    return shadow;
}

float ShadowCalculationPoint(vec3 world_space_frag_pos, float bias, float viewDistance, int lightIndex)
{
    vec3 fragToLight = world_space_frag_pos - LightSources.lights[lightIndex].position;
    float fragDistance = length(fragToLight);
    if(LightSources.lights[lightIndex].farPlanePoint < fragDistance) {
        return 1.0;
    }
    float closestDepth = texture(pre_shadowPoint, vec4(fragToLight, lightIndex)).r;
    closestDepth *= LightSources.lights[lightIndex].farPlanePoint;
    float currentDepth = fragDistance;
    float shadow = 0.0;
    int samples  = 20;
    float diskRadius = (1.0 + (viewDistance / LightSources.lights[lightIndex].farPlanePoint)) / 50.0;
    for(int i = 0; i < samples; ++i) {
        float pcfDepth = texture(pre_shadowPoint, vec4(fragToLight + pointSampleOffsetDirections[i] * diskRadius, lightIndex)).r;
        pcfDepth *= LightSources.lights[lightIndex].farPlanePoint;
        if(currentDepth - bias > pcfDepth)
            shadow += 1.0;
    }
    float attenuation = 1.0 / (LightSources.lights[lightIndex].attenuation.x +
                              (LightSources.lights[lightIndex].attenuation.y * fragDistance) +
                               (LightSources.lights[lightIndex].attenuation.z * fragDistance * fragDistance));
    attenuation = clamp(attenuation, 0.0, 1.0);
    attenuation = 1.0 - attenuation;
    if(attenuation == 1.0) {
        shadow = 1.0;
    } else {
        shadow /= float(samples);
        shadow = max(shadow, attenuation);
    }

    return shadow;
}

vec3 unpackNormal(vec2 pa) {
    vec2 p = pa * 2.0 - 1.0;

    // Reconstruct Y-Up vector, appearently other up vectors require different formula
    vec3 n = vec3(p.x, 1.0 - abs(p.x) - abs(p.y), p.y);

    if (n.y < 0.0) {
        float oldX = n.x;
        n.x = (1.0 - abs(n.z)) * (oldX >= 0.0 ? 1.0 : -1.0);
        n.z = (1.0 - abs(oldX)) * (n.z >= 0.0 ? 1.0 : -1.0);
    }

    return normalize(n);
}

void main()
{
    float depth = texture(pre_depthMap, from_vs.textureCoordinates).r;
    vec3 fragPos = ReconstructWorldPos(from_vs.textureCoordinates, depth);
    vec3 normal = unpackNormal(texture(pre_gNormalMap, from_vs.textureCoordinates).xy);
    vec4 albedoSpec = texture(pre_gAlbedoSpecMap, from_vs.textureCoordinates);
    vec3 albedo = albedoSpec.rgb;
    int materialIndex = int(albedoSpec.a * 255.0);

    // Retrieve material information from constant buffer
    float shininess = AllMaterialsArray.materials[materialIndex].shininess;
    vec3 materialAmbient = AllMaterialsArray.materials[materialIndex].ambient;

    // If the material has an ambient map, it will be in the gAmbientMap
    if((AllMaterialsArray.materials[materialIndex].isMap & 0x0008) != 0) {
        materialAmbient = texture(pre_gAmbientMap, from_vs.textureCoordinates).rgb;
    }

    float ssao = texture(pre_ssao, from_vs.textureCoordinates).r;

    vec4 clip_space_pos = vec4(from_vs.textureCoordinates * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 view_space_pos = playerTransforms.inverseProjection * clip_space_pos;
    float precise_view_z = abs(view_space_pos.z / view_space_pos.w);

    vec3 directLighting = vec3(0.0);
    vec3 lightAmbient = vec3(0.0);
    vec3 viewDirectory = normalize(playerTransforms.position - fragPos);
    float viewDistance = length(playerTransforms.position - fragPos);

    for(int i=0; i < NR_POINT_LIGHTS; ++i){
        if(LightSources.lights[i].type != 0) {
            vec3 lightDirectory;
            if(LightSources.lights[i].type == 1) { // Directional Light
                lightDirectory = normalize(-LightSources.lights[i].position);
            } else if(LightSources.lights[i].type == 2) { // Point Light
                lightDirectory = normalize(LightSources.lights[i].position - fragPos);
            }

            float diffuseRate = max(dot(normal, lightDirectory), 0.0);
            vec3 reflectDirectory = reflect(-lightDirectory, normal);
            float specularRate = max(dot(viewDirectory, reflectDirectory), 0.0);
            if(specularRate != 0.0 && shininess != 0.0) {
                specularRate = pow(specularRate, shininess);
            } else {
                specularRate = 0.0;
            }

            float shadow = 0.0;
            if(LightSources.lights[i].type == 1) { // Directional light
                // Apply a slope-scaled normal offset bias in world space
                float NdotL = max(dot(normal, lightDirectory), 0.0);
                float world_bias = max(1.5 * (1.0 - NdotL), 0.05); // Bias from 0.05 to 1.5 world units
                vec3 biased_fragPos = fragPos + normal * world_bias;
                shadow = ShadowCalculationDirectional(0.0, i, biased_fragPos, precise_view_z);
            } else if (LightSources.lights[i].type == 2){ // Point light
                float point_bias = 0.005;
                shadow = ShadowCalculationPoint(fragPos, point_bias, viewDistance, i);
            }

            directLighting += ((1.0 - shadow) * (diffuseRate + specularRate) * LightSources.lights[i].color);
            lightAmbient += LightSources.lights[i].ambient;
        }
    }

    vec3 totalAmbient = materialAmbient + lightAmbient;
    vec3 fullyLitColor = (directLighting + totalAmbient) * albedo;
    vec3 occludedAmbient = totalAmbient * ssao;
    finalColor = vec4(fullyLitColor - occludedAmbient, 1.0);
    gl_FragDepth = depth;
}
