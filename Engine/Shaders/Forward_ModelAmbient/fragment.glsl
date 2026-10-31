#import <./Engine/Shaders/Shared/Lights.glsl>
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/PointShadow.frag>
#import <./Engine/Shaders/Shared/DirectionalShadow.frag>
#import <./Engine/Shaders/Shared/Material.frag>

out vec4 finalColor;

in VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    flat int materialIndex;
} from_vs;

#import <./Engine/Shaders/Shared/Lighting.frag>

void main(void) {
    vec3 world_space_normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);
    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);
    float viewDistance = length(playerTransforms.position - from_vs.fragPos);

    vec3 directLighting;
    vec3 specularLighting;
    vec3 lightAmbient;
    sumLights(from_vs.fragPos, world_space_normal, shininess, viewDistance, precise_view_z, directLighting, specularLighting, lightAmbient);

    //material reads after the light loop, so they don't stay in registers through it
    vec4 albedo = getSimpleMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);
    vec3 materialAmbient = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);
    vec3 materialSpecular = getMaterialSpecular(from_vs.materialIndex, from_vs.textureCoord);
    vec3 totalAmbient;
    vec3 fullyLitColor = combineLighting(directLighting, specularLighting, lightAmbient, albedo.rgb, materialSpecular, materialAmbient, totalAmbient);

    finalColor = vec4(fullyLitColor, albedo.a);
}