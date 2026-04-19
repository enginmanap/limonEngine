
#define_option maximumPointLights
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/PointShadow.frag>
#import <./Engine/Shaders/Shared/DirectionalShadow.frag>
#import <./Engine/Shaders/Shared/Material.frag>

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

vec3 ReconstructWorldPos(vec2 texCoords, float depth) {
    float z = depth * 2.0 - 1.0;
    vec4 clipSpacePosition = vec4(texCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = playerTransforms.inverseProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = playerTransforms.inverseCamera * viewSpacePosition;
    return worldSpacePosition.xyz;
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

    vec3 totalAmbient = materialAmbient + lightAmbient;
    vec3 fullyLitColor = (directLighting + totalAmbient) * albedo;
    vec3 occludedAmbient = totalAmbient * ssao;
    finalColor = vec4(fullyLitColor - occludedAmbient, 1.0);
    gl_FragDepth = depth;
}
