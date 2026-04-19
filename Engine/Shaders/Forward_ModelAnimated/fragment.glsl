#define_option CascadeCount
#define_option CascadeLimitList
#define_option maximumPointLights

#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/PointShadow.frag>
#import <./Engine/Shaders/Shared/DirectionalShadow.frag>
#import <./Engine/Shaders/Shared/Material.frag>

out vec4 finalColor;

in VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;    // World-space normal
    vec3 fragPos;   // World-space fragment position
    flat int materialIndex;
} from_vs;

#import <./Engine/Shaders/Shared/Lighting.frag>

void main(void) {
    vec3 world_space_normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);
    vec4 albedo = getSimpleMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);

    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;
    vec3 materialAmbient = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);
    float viewDistance = length(playerTransforms.position - from_vs.fragPos);

    vec3 fullyLitColor = calculateLighting(from_vs.fragPos, world_space_normal, albedo.rgb, shininess, materialAmbient, viewDistance, precise_view_z, gl_FragCoord.z);

    finalColor = vec4(fullyLitColor, albedo.a);
}