
#define_option maximumLights
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

// Returns view-space position (w=1 after divide). Caller derives world pos and view-z from this.
vec4 ReconstructViewPos(vec2 texCoords, float depth) {
    vec4 clipSpacePosition = vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewSpacePosition = playerTransforms.inverseProjection * clipSpacePosition;
    return viewSpacePosition / viewSpacePosition.w;
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

#import <./Engine/Shaders/Shared/Lighting.frag>

void main()
{
    float depth = texture(pre_depthMap, from_vs.textureCoordinates).r;
    vec4 viewSpacePos = ReconstructViewPos(from_vs.textureCoordinates, depth);
    float precise_view_z = abs(viewSpacePos.z);
    vec3 fragPos = (playerTransforms.inverseCamera * viewSpacePos).xyz;
    float viewDistance = length(viewSpacePos.xyz); // camera is at origin in view space

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

    vec3 totalAmbient;
    vec3 fullyLitColor = calculateLighting(fragPos, normal, albedo, shininess, materialAmbient, viewDistance, precise_view_z, depth, totalAmbient);

    vec3 occludedAmbient = totalAmbient * ssao;
    finalColor = vec4(fullyLitColor - occludedAmbient, 1.0);
    gl_FragDepth = depth;
}