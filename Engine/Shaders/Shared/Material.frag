#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#define NR_MAX_MATERIALS 200
#define CUTOUT_ALPHA_THRESHOLD 0.99 //ModelTransparentCutout draws at or above with depth, ModelTransparent blends below

uniform sampler2D ambientSampler;
uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform sampler2D opacitySampler;
uniform sampler2D normalSampler;

struct material {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    uint specularAndMaps;//rgb the specular color, a the map bits, test them with materialHasMap
};

layout (std140) uniform MaterialInformationBlock {
    material materials[NR_MAX_MATERIALS];
} AllMaterialsArray;

//the map bits of Material::getTextureSlots, where they sit in specularAndMaps: the alpha byte, bits 24-31
#define MATERIAL_OPACITY_MAP  0x01000000u
#define MATERIAL_SPECULAR_MAP 0x02000000u
#define MATERIAL_DIFFUSE_MAP  0x04000000u
#define MATERIAL_AMBIENT_MAP  0x08000000u
#define MATERIAL_NORMAL_MAP   0x10000000u

//tests the bit where it is, unpackUnorm4x8(specularAndMaps).a would give the map bits as a float divided by 255
bool materialHasMap(int materialIndex, uint mapBit) {
    return (AllMaterialsArray.materials[materialIndex].specularAndMaps & mapBit) != 0u;
}

vec4 getMaterialAlbedo(int materialIndex, vec2 uv) {
    vec4 objectColor;
    if(materialHasMap(materialIndex, MATERIAL_DIFFUSE_MAP)) {
        if(materialHasMap(materialIndex, MATERIAL_OPACITY_MAP)) { //if there is a opacity map, and it with diffuse
            vec4 opacity = texture(opacitySampler, uv);
            if(opacity.a < 0.05) {
                // We shouldn't strictly discard in a helper, but it's the current logic.
                // A better approach is to return alpha and let the caller discard.
                // Keeping it as is for minimal disruption, though it limits reusability if someone *doesn't* want to discard.
                discard;
            }
            objectColor = texture(diffuseSampler, uv);
            objectColor.w = opacity.a;
        } else {
            objectColor = texture(diffuseSampler, uv);
            if(objectColor.a < 0.05) {
                discard;
            }
        }
    } else {
        objectColor = vec4(AllMaterialsArray.materials[materialIndex].diffuse, 1.0);
    }
    return objectColor;
}

vec3 getMaterialNormal(int materialIndex, vec2 uv, vec3 vertexNormal) {
    vec3 normal = vertexNormal;
    if(materialHasMap(materialIndex, MATERIAL_NORMAL_MAP)) {
        normal = -1.0 * vec3(texture(normalSampler, uv));
    }
    return normal;
}

vec3 getMaterialAmbient(int materialIndex, vec2 uv) {
    vec3 lightingColorFactor;
    if(materialHasMap(materialIndex, MATERIAL_AMBIENT_MAP)) {
        lightingColorFactor = vec3(texture(ambientSampler, uv));
    } else {
        lightingColorFactor = AllMaterialsArray.materials[materialIndex].ambient;
    }
    return lightingColorFactor;
}

vec3 getMaterialSpecular(int materialIndex, vec2 uv) {
    if(materialHasMap(materialIndex, MATERIAL_SPECULAR_MAP)) {
        return vec3(texture(specularSampler, uv));
    }
    return unpackUnorm4x8(AllMaterialsArray.materials[materialIndex].specularAndMaps).rgb;
}

// For G-Buffer passes that only need simple albedo extraction
vec4 getSimpleMaterialAlbedo(int materialIndex, vec2 uv) {
    vec4 albedo = vec4(1.0);
    if(materialHasMap(materialIndex, MATERIAL_DIFFUSE_MAP)) {
        albedo = texture(diffuseSampler, uv);
    } else {
        albedo = vec4(AllMaterialsArray.materials[materialIndex].diffuse, 1.0);
    }
    return albedo;
}

#endif // MATERIAL_GLSL
