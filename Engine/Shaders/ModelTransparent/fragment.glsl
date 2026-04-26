#define_option maximumPointLights
#define_option CascadeCount
#define_option CascadeLimitList

#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/PointShadow.frag>
#import <./Engine/Shaders/Shared/DirectionalShadow.frag>
#import <./Engine/Shaders/Shared/Material.frag>

layout (location = 0) out vec4 outputColor;

in VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[maximumPointLights];
    flat int materialIndex;
} from_vs;

#import <./Engine/Shaders/Shared/Lighting.frag>

void main(void) {
    vec4 objectColor = getMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);
    vec3 normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);
    vec3 materialAmbient = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);
    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);
    float viewDistance = length(playerTransforms.position - from_vs.fragPos);

    vec3 totalAmbient;
    vec3 fullyLitColor = calculateLighting(from_vs.fragPos, normal, objectColor.rgb, shininess, materialAmbient, viewDistance, precise_view_z, gl_FragCoord.z, totalAmbient);

    outputColor = vec4(fullyLitColor, objectColor.a);
}