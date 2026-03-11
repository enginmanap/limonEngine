#version 330 core
#extension GL_ARB_texture_cube_map_array : enable

#define_option CascadeCount
#define_option CascadeLimitList

#define NR_POINT_LIGHTS 4

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D gNormalMap;      // World-space normal
uniform sampler2D gAlbedoSpecMap;  // Albedo (rgb), Shininess (a)
uniform sampler2D gAmbientMap;     // Ambient (rgb)
uniform sampler2D pre_ssao;
uniform sampler2D pre_depthMap;

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

uniform sampler2DArray pre_shadowDirectional;
uniform samplerCubeArray pre_shadowPoint;

vec3 pointSampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

vec3 ReconstructWorldPos(vec2 texCoords, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = playerTransforms.inverseProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = playerTransforms.inverseCamera * viewSpacePosition;
    return worldSpacePosition.xyz;
}


float ShadowCalculationDirectional(float depthBias, int lightIndex, vec3 world_space_frag_pos, float precise_view_z){
    float cascadePlaneDistances[CascadeCount] = float[](CascadeLimitList);
    // Use the precise view Z for cascade selection to avoid instability
    float depthValue = precise_view_z;
    int layer = CascadeCount - 1;
    for (int i = 0; i < CascadeCount; ++i) {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(world_space_frag_pos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;

    float currentDepth = projectedCoordinates.z;
    if (currentDepth >= 1.0)
    {
        return 0.0;
    }

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(pre_shadowDirectional, 0).xy;
    for(int x = -1; x <= 1; ++x){
        for(int y = -1; y <= 1; ++y){
            float pcfDepth = texture(pre_shadowDirectional, vec3(projectedCoordinates.xy + vec2(x, y) * texelSize, layer)).r;
            if(currentDepth - depthBias > pcfDepth) {
                shadow += 1.0;
            }
        }
    }
    shadow /= 9.0;

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
    attenuation = 1 - attenuation;
    if(attenuation == 1) {
        shadow = 1.0;
    } else {
        shadow /= float(samples);
        shadow = max(shadow, attenuation);
    }

    return shadow;
}

void main()
{
    float depth = texture(pre_depthMap, from_vs.textureCoordinates).r;
    vec3 fragPos = ReconstructWorldPos(from_vs.textureCoordinates, depth);
    vec3 normal = texture(gNormalMap, from_vs.textureCoordinates).xyz;
    vec4 albedoSpec = texture(gAlbedoSpecMap, from_vs.textureCoordinates);
    vec3 albedo = albedoSpec.rgb;
    float shininess = albedoSpec.a * 256.0;
    vec3 materialAmbient = texture(gAmbientMap, from_vs.textureCoordinates).rgb;
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
            if(specularRate != 0 && shininess != 0) {
                specularRate = pow(specularRate, shininess);
            } else {
                specularRate = 0;
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
