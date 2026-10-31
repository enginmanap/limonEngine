
// Shared Lighting calculations for Forward Rendering and Deferred Lighting Pass
// Used by Forward_ModelAmbient, Forward_ModelAnimated, ModelTransparent, and CombineColorsWithSSAO

#import <./Engine/Shaders/Shared/Lights.glsl>

float pointLightAttenuation(int lightIndex, float fragDistance) {
    // zero or negative would cause numeric issues, max first.
    float attenuationFactor = max(LightSources.lights[lightIndex].attenuation.x +
                                  (LightSources.lights[lightIndex].attenuation.y * fragDistance) +
                                  (LightSources.lights[lightIndex].attenuation.z * fragDistance * fragDistance), 0.0001);

    // To reach 0 at the radius without a steep change, use double root
    float normalizedFragDistance = fragDistance / LightSources.lights[lightIndex].radius;
    float window = clamp(1.0 - pow(normalizedFragDistance, LightSources.lights[lightIndex].falloffExponent), 0.0, 1.0);
    window = window * window;
    return clamp(LightSources.lights[lightIndex].intensity / attenuationFactor, 0.0, 1.0) * window;
}

//0 for directional, they have no position
float lightDistance(int lightIndex, vec3 fragPos, out vec3 lightDirection) {
    if(LightSources.lights[lightIndex].type == 1) {
        lightDirection = normalize(-LightSources.lights[lightIndex].position);
        return 0.0;
    }
    vec3 toLight = LightSources.lights[lightIndex].position - fragPos;
    float fragDistance = length(toLight);
    lightDirection = toLight / fragDistance;
    return fragDistance;
}

void addLight(int lightIndex, vec3 lightDirection, float attenuation, float shadow, vec3 normal, vec3 viewReflection, float shininess,
              inout vec3 directLighting, inout vec3 specularLighting, inout vec3 lightAmbient) {
    float diffuseRate = max(dot(normal, lightDirection), 0.0);
    float specularRate = max(dot(lightDirection, viewReflection), 0.0);
    if(specularRate != 0.0 && shininess != 0.0) {
        specularRate = pow(specularRate, shininess);
    } else {
        specularRate = 0.0;
    }

    // if we were physically based, we should have used (1.0 - shadow) * attenuation but since
    // we are missing GI, we need to account for ambient term and this is where we end up
    float lightFactor = min(1.0 - shadow, attenuation);
    directLighting += (lightFactor * diffuseRate * LightSources.lights[lightIndex].color);
    specularLighting += (lightFactor * specularRate * LightSources.lights[lightIndex].color);
    // we don't check shadow for ambient, unlike GI solutions. intentional
    lightAmbient += attenuation * LightSources.lights[lightIndex].ambient;
}

void sumLights(vec3 fragPos, vec3 normal, float shininess, float viewDistance, float precise_view_z, out vec3 directLighting, out vec3 specularLighting, out vec3 lightAmbient) {
    //don't read material state before calling this, it stays in registers through the light loop (c0b412b6)
    //own loop, merged into the lighting loop the accumulators sit on top of the shadow sampling's register peak
    float shadows[performance_maximumLights];
    for(int i=0; i < performance_maximumLights; ++i){
        shadows[i] = 0.0;
        int lightType = LightSources.lights[i].type;
        if(lightType == 0) {
            continue;
        }
        vec3 lightDirection;
        float fragDistance = lightDistance(i, fragPos, lightDirection);
        //recomputed in the lighting loop, keeping them per light costs registers during shadow sampling
        float diffuseRate = max(dot(normal, lightDirection), 0.0);
        //radius test only inside the point branch, before it the test would mask directional work too
        if(lightType == 1) {
            // We can't calculate bias of directional light because we don't know which cascade is selected
            // it has to be computed within.
            shadows[i] = ShadowCalculationDirectional(i, fragPos, normal, diffuseRate, precise_view_z);
        } else if(fragDistance < LightSources.lights[i].radius) {//out of range, the light is skipped in the lighting loop too
            // Normal offset bias: push comparison point off the surface to avoid self-shadowing.
            // Point light is a perspective cube map where depth precision degrades with distance.
            float normalBias = mix(0.08, 0.02, diffuseRate);
            vec3 biasedFragPos = fragPos + normal * normalBias;
            shadows[i] = ShadowCalculationPoint(biasedFragPos, viewDistance, i);
        }
    }

    directLighting = vec3(0.0);
    specularLighting = vec3(0.0);
    lightAmbient = vec3(0.0);
    vec3 viewReflection = reflect(normalize(fragPos - playerTransforms.position), normal);//one reflect for all lights instead of one per light

    for(int i=0; i < performance_maximumLights; ++i){
        int lightType = LightSources.lights[i].type;
        if(lightType == 0) {
            continue;
        }
        vec3 lightDirection;
        float fragDistance = lightDistance(i, fragPos, lightDirection);
        if(lightType == 1) {
            addLight(i, lightDirection, 1.0, shadows[i], normal, viewReflection, shininess, directLighting, specularLighting, lightAmbient);
        } else if(fragDistance < LightSources.lights[i].radius) {//ambient is attenuated too, so out of range adds 0
            addLight(i, lightDirection, pointLightAttenuation(i, fragDistance), shadows[i], normal, viewReflection, shininess, directLighting, specularLighting, lightAmbient);
        }
    }
}

vec3 combineLighting(vec3 directLighting, vec3 specularLighting, vec3 lightAmbient, vec3 albedo, vec3 materialSpecular, vec3 materialAmbient, out vec3 totalAmbient) {
    totalAmbient = materialAmbient + lightAmbient;
    return (directLighting + totalAmbient) * albedo + specularLighting * materialSpecular;//the highlight is not tinted by albedo
}
