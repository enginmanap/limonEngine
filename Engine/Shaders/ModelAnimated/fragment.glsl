
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

#define_option CascadeCount
#define_option CascadeLimitList

#define NR_MAX_MATERIALS 200

layout (location = 0) out vec2 gNormal;       // World-space normal (xyz)
layout (location = 1) out vec4 gAlbedoSpec;   // Albedo (rgb), MaterialIndex (a)

struct material {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    int isMap;
};

layout (std140) uniform MaterialInformationBlock {
    material materials[NR_MAX_MATERIALS];
} AllMaterialsArray;

in VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;    // World-space normal
    vec3 fragPos;   // World-space fragment position
    flat int materialIndex;
} from_vs;

uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform sampler2D opacitySampler;
uniform sampler2D normalSampler;

vec2 packNormal(vec3 n) {
    float invL1 = 1.0 / (abs(n.x) + abs(n.y) + abs(n.z));
    vec2 p = n.xz * invL1;

    // Fold based on Y-Up
    if (n.y < 0.0) {
        float oldX = p.x;
        p.x = (1.0 - abs(p.y)) * (oldX >= 0.0 ? 1.0 : -1.0);
        p.y = (1.0 - abs(oldX)) * (p.y >= 0.0 ? 1.0 : -1.0);
    }

    return p * 0.5 + 0.5;
}

void main(void) {
    vec3 world_space_normal = normalize(from_vs.normal);
    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0010) != 0) {
         world_space_normal = -1.0 * vec3(texture(normalSampler, from_vs.textureCoord));
    }
    gNormal = packNormal(world_space_normal);

    vec4 albedo = vec4(1.0);
    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0004)!=0) {
        albedo = texture(diffuseSampler, from_vs.textureCoord);
    } else {
        albedo = vec4(AllMaterialsArray.materials[from_vs.materialIndex].diffuse, 1.0);
    }

    // Output materialIndex in alpha instead of shininess
    gAlbedoSpec = vec4(albedo.rgb, float(from_vs.materialIndex) / 255.0);
}
