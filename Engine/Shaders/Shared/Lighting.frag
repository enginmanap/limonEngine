
// Shared Lighting calculations for Forward Rendering and Deferred Lighting Pass
// Used by Forward_ModelAmbient, Forward_ModelAnimated, ModelTransparent, and CombineColorsWithSSAO

#define_option maximumPointLights

vec3 calculateLighting(vec3 fragPos, vec3 normal, vec3 albedo, float shininess, vec3 materialAmbient, float viewDistance, float precise_view_z, float depth, out vec3 totalAmbient) {
    vec3 directLighting = vec3(0.0);
    vec3 lightAmbient = vec3(0.0);
    vec3 viewDirectory = normalize(playerTransforms.position - fragPos);

    for(int i=0; i < maximumPointLights; ++i){
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

            // Normal offset bias: push comparison point off the surface to avoid self-shadowing.
            // Magnitude differs by type — directional is orthographic over a large area (small bias
            // needed), point is a perspective cube map where depth precision degrades with distance.
            float normalBias = (lightType == 1)
                ? mix(0.01, 0.005, diffuseRate)
                : mix(0.08,  0.02, diffuseRate);
            vec3 biasedFragPos = fragPos + normal * normalBias;

            float shadow = 0.0;
            if(lightType == 1) {
                shadow = ShadowCalculationDirectional(i, biasedFragPos, precise_view_z);
            } else {
                shadow = ShadowCalculationPoint(biasedFragPos, viewDistance, i);
            }

            directLighting += ((1.0 - shadow) * (diffuseRate + specularRate) * LightSources.lights[i].color);
            lightAmbient += LightSources.lights[i].ambient;
        }
    }

    totalAmbient = materialAmbient + lightAmbient;
    vec3 fullyLitColor = (directLighting + totalAmbient) * albedo;

    return fullyLitColor;
}
