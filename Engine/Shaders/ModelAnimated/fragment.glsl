#version 330
#extension GL_ARB_texture_cube_map_array : enable

#define_option CascadeCount
#define_option CascadeLimitList

#define NR_MAX_MATERIALS 200

layout (location = 0) out vec4 gNormal;       // World-space normal (xyz)
layout (location = 1) out vec4 gAlbedoSpec;   // Albedo (rgb), Shininess (a)
layout (location = 2) out vec4 gAmbient;      // Ambient color (rgb)

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    mat4 inverseProjection;
    mat4 inverseCamera;
    mat3 transposeInverseCamera;
    vec3 position;
    vec3 cameraSpacePosition;
    vec2 noiseScale;
    int time;
} playerTransforms;

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

uniform sampler2D ambientSampler;
uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform sampler2D opacitySampler;
uniform sampler2D normalSampler;

void main(void) {
    vec3 world_space_normal = normalize(from_vs.normal);
    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0010) != 0) {
         world_space_normal = -1 * vec3(texture(normalSampler, from_vs.textureCoord));
    }
    gNormal = vec4(world_space_normal, 1.0);

    vec4 albedo = vec4(1.0);
    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0004)!=0) {
        albedo = texture(diffuseSampler, from_vs.textureCoord);
    } else {
        albedo = vec4(AllMaterialsArray.materials[from_vs.materialIndex].diffuse, 1.0);
    }

    float shininess = AllMaterialsArray.materials[from_vs.materialIndex].shininess;
    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0002)!=0) {
        vec3 specularColor = vec3(texture(specularSampler, from_vs.textureCoord));
        float specularAverage = (specularColor.x + specularColor.y + specularColor.z) / 3.0;
    }
    gAlbedoSpec = vec4(albedo.rgb, shininess / 256.0);

    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0008) != 0) {
        gAmbient.rgb = vec3(texture(ambientSampler, from_vs.textureCoord));
    } else {
        gAmbient.rgb = AllMaterialsArray.materials[from_vs.materialIndex].ambient;
    }
}
