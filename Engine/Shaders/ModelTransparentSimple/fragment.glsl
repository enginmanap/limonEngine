#define_option shadow_cascadeCount
#define_option shadow_cascadeLimitList

#import <./Engine/Shaders/Shared/Lights.glsl>
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/Material.frag>

//experiment pipeline (forward_split_renderPipeline.xml), single tap hard shadows on purpose, it measures a ceiling

uniform sampler2DArrayShadow pre_shadowDirectional;
uniform samplerCubeArrayShadow pre_shadowPoint;

const float cascadePlaneDistances[shadow_cascadeCount] = float[](shadow_cascadeLimitList);

layout (location = 0) out vec4 outputColor;

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

    //sampler2DArrayShadow returns 1.0 if not in shadow
    return 1.0 - texture(pre_shadowDirectional, vec4(projectedCoordinates.xy, layer, projectedCoordinates.z));
}

float _shadowPoint(int lightIndex, vec3 biasedFragPos) {
    vec3 fragToLight = biasedFragPos - LightSources.lights[lightIndex].position;
    float fragDistance = length(fragToLight);
    if (LightSources.lights[lightIndex].radius < fragDistance) {
        return 0.0;//attenuation is 0 here anyway, this only skips the sample
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

//0 for directional, they have no position
float _lightDistance(int lightIndex, out vec3 lightDirection) {
    if(LightSources.lights[lightIndex].type == 1) {
        lightDirection = normalize(-LightSources.lights[lightIndex].position);
        return 0.0;
    }
    vec3 toLight = LightSources.lights[lightIndex].position - from_vs.fragPos;
    float fragDistance = length(toLight);
    lightDirection = toLight / fragDistance;
    return fragDistance;
}

void _addLight(int lightIndex, vec3 lightDirection, float attenuation, float shadow, vec3 world_space_normal, vec3 viewReflection, float shininess,
               inout vec3 diffuseAndAmbientLighting, inout vec3 specularLighting) {
    float diffuseRate = max(dot(world_space_normal, lightDirection), 0.0);
    float specularRate = max(dot(lightDirection, viewReflection), 0.0);
    if(specularRate != 0.0 && shininess != 0.0) {
        specularRate = pow(specularRate, shininess);
    } else {
        specularRate = 0.0;
    }
    //min, not multiply, same as Shared/Lighting.frag
    float lightFactor = min(1.0 - shadow, attenuation);
    diffuseAndAmbientLighting += (lightFactor * diffuseRate * LightSources.lights[lightIndex].color);
    specularLighting += (lightFactor * specularRate * LightSources.lights[lightIndex].color);
    //never shadowed, ambient is what keeps shadowed surfaces off black
    diffuseAndAmbientLighting += attenuation * LightSources.lights[lightIndex].ambient;
}

vec3 _litColor(vec3 world_space_normal) {
    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);

    //own loop, keeps the accumulators and material reads out of the shadow sampling's registers
    float shadows[performance_maximumLights];
    for(int i = 0; i < performance_maximumLights; ++i) {
        shadows[i] = 0.0;
        int lightType = LightSources.lights[i].type;
        if(lightType == 1) {
            shadows[i] = _shadowDirectional(i, from_vs.fragPos + world_space_normal * 0.01, precise_view_z);//ortho, a small bias is enough
        } else if(lightType != 0) {
            shadows[i] = _shadowPoint(i, from_vs.fragPos + world_space_normal * 0.08);//cube map precision drops with distance, needs more
        }
    }

    vec3 diffuseAndAmbientLighting = vec3(0.0);//light ambient summed in too, both only get multiplied by albedo
    vec3 specularLighting = vec3(0.0);
    vec3 viewReflection = reflect(normalize(from_vs.fragPos - playerTransforms.position), world_space_normal);

    for(int i = 0; i < performance_maximumLights; ++i) {
        int lightType = LightSources.lights[i].type;
        if(lightType == 0) {
            continue;
        }

        vec3 lightDirection;
        float fragDistance = _lightDistance(i, lightDirection);
        //radius test only inside the point branch, before it the test would mask directional work too
        if(lightType == 1) {
            _addLight(i, lightDirection, 1.0, shadows[i], world_space_normal, viewReflection, shininess, diffuseAndAmbientLighting, specularLighting);
        } else if(fragDistance < LightSources.lights[i].radius) {//out of range adds 0, ambient is attenuated too
            _addLight(i, lightDirection, _pointAttenuation(i, fragDistance), shadows[i], world_space_normal, viewReflection, shininess, diffuseAndAmbientLighting, specularLighting);
        }
    }

    //material reads after the light loops, so they don't stay in registers through them
    vec3 albedo = getSimpleMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord).rgb;
    vec3 materialAmbient = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);
    vec3 materialSpecular = getMaterialSpecular(from_vs.materialIndex, from_vs.textureCoord);
    return (diffuseAndAmbientLighting + materialAmbient) * albedo + specularLighting * materialSpecular;//the highlight is not tinted by albedo
}

void main(void) {
    float alpha = getMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord).a;//opacity map alpha, getSimpleMaterialAlbedo doesn't read it
    if(alpha >= CUTOUT_ALPHA_THRESHOLD) {
        discard;//ModelTransparentCutoutSimple drew these, this stage writes no depth
    } else {//v3d keeps running discarded pixels, in the else a fully discarded group skips this
        vec3 world_space_normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);
        outputColor = vec4(_litColor(world_space_normal), alpha);
    }
}
