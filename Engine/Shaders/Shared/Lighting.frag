
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

vec3 calculateLighting(vec3 fragPos, vec3 normal, vec3 albedo, float shininess, vec3 materialAmbient, float viewDistance, float precise_view_z, float depth, out vec3 totalAmbient) {
    vec3 directLighting = vec3(0.0);
    vec3 lightAmbient = vec3(0.0);
    vec3 viewDirectory = normalize(playerTransforms.position - fragPos);

    for(int i=0; i < performance_maximumLights; ++i){
        int lightType = LightSources.lights[i].type;
        if(lightType != 0) {
            vec3 lightPos = LightSources.lights[i].position;
            vec3 lightDirectory;
            if(lightType == 1) { // Directional Light
                lightDirectory = normalize(-lightPos);
            } else { // Point Light
                lightDirectory = normalize(lightPos - fragPos);
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
            float attenuation = 1.0;//Since directional has no attenuation, default to 1
            if(lightType == 1) {
                // We can't calculate bias of directional light because we don't know which cascade is selected
                // it has to be computed within. diffuseRate is already computed above, pass it through
                // instead of recalculating lightDirectory/diffuseRate again in there.
                shadow = ShadowCalculationDirectional(i, fragPos, normal, diffuseRate, precise_view_z);
            } else {
                // Normal offset bias: push comparison point off the surface to avoid self-shadowing.
                // Point light is a perspective cube map where depth precision degrades with distance.
                float normalBias = mix(0.08, 0.02, diffuseRate);
                vec3 biasedFragPos = fragPos + normal * normalBias;
                shadow = ShadowCalculationPoint(biasedFragPos, viewDistance, i);
                attenuation = pointLightAttenuation(i, length(lightPos - fragPos));
            }

            // if we were physically based, we should have used (1.0 - shadow) * attenuation but since
            // we are missing GI, we need to account for ambient term and this is where we end up
            directLighting += (min(1.0 - shadow, attenuation) * (diffuseRate + specularRate) * LightSources.lights[i].color);
            // we don't check shadow for ambient, unlike GI solutions. intentional
            lightAmbient += attenuation * LightSources.lights[i].ambient;
        }
    }

    totalAmbient = materialAmbient + lightAmbient;
    vec3 fullyLitColor = (directLighting + totalAmbient) * albedo;

    return fullyLitColor;
}
