#define_option maximumLights

#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/PointShadow.frag>
#import <./Engine/Shaders/Shared/DirectionalShadow.frag>
#import <./Engine/Shaders/Shared/Material.frag>

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outAmbient;
layout (location = 2) out vec2 outNormal;

vec2 packNormal(vec3 n) {
    float invL1 = 1.0 / (abs(n.x) + abs(n.y) + abs(n.z));
    vec2 p = n.xz * invL1;
    if (n.y < 0.0) {
        float oldX = p.x;
        p.x = (1.0 - abs(p.y)) * (oldX >= 0.0 ? 1.0 : -1.0);
        p.y = (1.0 - abs(oldX)) * (p.y >= 0.0 ? 1.0 : -1.0);
    }
    return p * 0.5 + 0.5;
}

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
    vec4 albedo = getSimpleMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);

    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;
    vec3 materialAmbient = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);
    float viewDistance = length(playerTransforms.position - from_vs.fragPos);

    vec3 totalAmbient;
    vec3 fullyLitColor = calculateLighting(from_vs.fragPos, world_space_normal, albedo.rgb, shininess, materialAmbient, viewDistance, precise_view_z, gl_FragCoord.z, totalAmbient);

    vec3 ambientComponent = totalAmbient * albedo.rgb;
    outColor   = vec4(fullyLitColor, albedo.a);
    outAmbient = vec4(ambientComponent, 1.0);
    outNormal  = packNormal(world_space_normal);
}
