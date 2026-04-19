#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

#define NR_MAX_MATERIALS 200

uniform sampler2D ambientSampler;
uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform sampler2D opacitySampler;
uniform sampler2D normalSampler;

struct material {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    int isMap; 	//using the last 4, ambient=8, diffuse=4, specular=2, opacity = 1
};

layout (std140) uniform MaterialInformationBlock {
    material materials[NR_MAX_MATERIALS];
} AllMaterialsArray;

vec4 getMaterialAlbedo(int materialIndex, vec2 uv) {
    vec4 objectColor;
    if((AllMaterialsArray.materials[materialIndex].isMap & 0x0004)!=0) {
        if((AllMaterialsArray.materials[materialIndex].isMap & 0x0001)!=0) { //if there is a opacity map, and it with diffuse
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
    if((AllMaterialsArray.materials[materialIndex].isMap & 0x0010) != 0) {
        normal = -1.0 * vec3(texture(normalSampler, uv));
    }
    return normal;
}

vec3 getMaterialAmbient(int materialIndex, vec2 uv) {
    vec3 lightingColorFactor;
    if((AllMaterialsArray.materials[materialIndex].isMap & 0x0008)!=0) {
        lightingColorFactor = vec3(texture(ambientSampler, uv));
    } else {
        lightingColorFactor = AllMaterialsArray.materials[materialIndex].ambient;
    }
    return lightingColorFactor;
}

// For G-Buffer passes that only need simple albedo extraction
vec4 getSimpleMaterialAlbedo(int materialIndex, vec2 uv) {
    vec4 albedo = vec4(1.0);
    if((AllMaterialsArray.materials[materialIndex].isMap & 0x0004)!=0) {
        albedo = texture(diffuseSampler, uv);
    } else {
        albedo = vec4(AllMaterialsArray.materials[materialIndex].diffuse, 1.0);
    }
    return albedo;
}

#endif // MATERIAL_GLSL
