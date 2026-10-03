#define_option shadow_cascadeCount
#define_option shadow_cascadeLimitList

#import <./Engine/Shaders/Shared/Lights.glsl>
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/Material.frag>

// Experiment pipeline, see Engine/forward_split_renderPipeline.xml. Shadow sampling is a single tap
// for both light types, no rotation, no poisson disk, no early out probe and no cascade blend.
// Shadows are hard and aliased on purpose, this exists to measure a ceiling not to look right.
// performance_maximumLights comes from Shared/Lights.glsl.

uniform sampler2DArrayShadow pre_shadowDirectional;
uniform samplerCubeArrayShadow pre_shadowPoint;

const float cascadePlaneDistances[shadow_cascadeCount] = float[](shadow_cascadeLimitList);

out vec4 finalColor;

in VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    flat int materialIndex;
} from_vs;

float _shadowDirectional(int lightIndex, vec3 biasedFragPos, float precise_view_z) {
    int layer = shadow_cascadeCount - 1;
    for (int i = 0; i < shadow_cascadeCount; ++i) {
        if (precise_view_z < cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }

    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(biasedFragPos, 1.0);
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;
    if (projectedCoordinates.z >= 1.0) {
        return 0.0;
    }

    // sampler2DArrayShadow returns 1.0 if not in shadow
    return 1.0 - texture(pre_shadowDirectional, vec4(projectedCoordinates.xy, layer, projectedCoordinates.z));
}

float _shadowPoint(int lightIndex, vec3 biasedFragPos) {
    vec3 fragToLight = biasedFragPos - LightSources.lights[lightIndex].position;
    float fragDistance = length(fragToLight);
    if (LightSources.lights[lightIndex].radius < fragDistance) {
        return 0.0; // _pointAttenuation is already 0 out here, this only skips the cube map sample
    }

    float normalizedFragDistance = fragDistance / LightSources.lights[lightIndex].radius;
    float lit = texture(pre_shadowPoint, vec4(fragToLight, lightIndex), normalizedFragDistance);

    return 1.0 - lit;
}

float _pointAttenuation(int lightIndex, float fragDistance) {
    float attenuationFactor = max(LightSources.lights[lightIndex].attenuation.x +
                                  (LightSources.lights[lightIndex].attenuation.y * fragDistance) +
                                  (LightSources.lights[lightIndex].attenuation.z * fragDistance * fragDistance), 0.0001);
    float normalizedFragDistance = fragDistance / LightSources.lights[lightIndex].radius;
    float window = clamp(1.0 - pow(normalizedFragDistance, LightSources.lights[lightIndex].falloffExponent), 0.0, 1.0);
    window = window * window;
    return clamp(LightSources.lights[lightIndex].intensity / attenuationFactor, 0.0, 1.0) * window;
}

void main(void) {
    vec3 world_space_normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);
    vec4 albedo = getSimpleMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);
    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;
    vec3 materialAmbient = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);

    vec3 directLighting = vec3(0.0);
    vec3 lightAmbient = vec3(0.0);
    vec3 viewDirectory = normalize(playerTransforms.position - from_vs.fragPos);

    for(int i = 0; i < performance_maximumLights; ++i) {
        int lightType = LightSources.lights[i].type;
        if(lightType == 0) {
            continue;
        }

        vec3 lightDirectory;
        float normalBias;
        if(lightType == 1) {
            lightDirectory = normalize(-LightSources.lights[i].position);
            normalBias = 0.01;// orthographic over a large area, small bias is enough
        } else {
            lightDirectory = normalize(LightSources.lights[i].position - from_vs.fragPos);
            normalBias = 0.08;// cube map depth precision degrades with distance, needs more
        }

        float diffuseRate = max(dot(world_space_normal, lightDirectory), 0.0);
        vec3 reflectDirectory = reflect(-lightDirectory, world_space_normal);
        float specularRate = max(dot(viewDirectory, reflectDirectory), 0.0);
        if(specularRate != 0.0 && shininess != 0.0) {
            specularRate = pow(specularRate, shininess);
        } else {
            specularRate = 0.0;
        }

        vec3 biasedFragPos = from_vs.fragPos + world_space_normal * normalBias;

        float shadow;
        float attenuation = 1.0;//directional light has no falloff, it stays 1 all the way
        if(lightType == 1) {
            shadow = _shadowDirectional(i, biasedFragPos, precise_view_z);
        } else {
            shadow = _shadowPoint(i, biasedFragPos);
            attenuation = _pointAttenuation(i, length(LightSources.lights[i].position - from_vs.fragPos));
        }

        // min, not multiply, same reasoning as Shared/Lighting.frag
        directLighting += (min(1.0 - shadow, attenuation) * (diffuseRate + specularRate) * LightSources.lights[i].color);
        // attenuated by distance but never by shadow, ambient is what lifts shadowed surfaces off black
        lightAmbient += attenuation * LightSources.lights[i].ambient;
    }

    vec3 totalAmbient = materialAmbient + lightAmbient;
    finalColor = vec4((directLighting + totalAmbient) * albedo.rgb, albedo.a);
}
