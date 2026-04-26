
// Shared Lighting calculations for Forward Rendering and Deferred Lighting Pass
// Used by Forward_ModelAmbient, Forward_ModelAnimated, ModelTransparent, and CombineColorsWithSSAO

#define_option maximumPointLights

vec3 calculateLighting(vec3 fragPos, vec3 normal, vec3 albedo, float shininess, vec3 materialAmbient, float viewDistance, float precise_view_z, float depth, out vec3 totalAmbient) {
    vec3 directLighting = vec3(0.0);
    vec3 lightAmbient = vec3(0.0);
    vec3 viewDirectory = normalize(playerTransforms.position - fragPos);

    for(int i=0; i < maximumPointLights; ++i){
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
                shadow = ShadowCalculationDirectional(i, fragPos, precise_view_z, normal);
            } else if (LightSources.lights[i].type == 2){ // Point light
                float point_bias = 0.005;
                shadow = ShadowCalculationPoint(fragPos, point_bias, viewDistance, i);
            }

            directLighting += ((1.0 - shadow) * (diffuseRate + specularRate) * LightSources.lights[i].color);
            lightAmbient += LightSources.lights[i].ambient;
        }
    }

    totalAmbient = materialAmbient + lightAmbient;
    vec3 fullyLitColor = (directLighting + totalAmbient) * albedo;

    return fullyLitColor;
}
