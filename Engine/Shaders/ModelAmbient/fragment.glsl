
#import <./Engine/Shaders/Shared/Material.frag>

// G-Buffer outputs (Matching ModelAnimated fragment shader)
layout (location = 0) out vec2 gNormal;       // World-space normal (xyz)
layout (location = 1) out vec4 gAlbedoSpec;   // Albedo (rgb), MaterialIndex (a)
layout (location = 2) out vec3 gAmbient;      // Ambient Map (rgb)

in VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    flat int materialIndex;
} from_vs;

vec2 packNormal(vec3 n) {
    // Project onto the L1 manifold (Note: Using Y as the "Up" axis)
    float invL1 = 1.0 / (abs(n.x) + abs(n.y) + abs(n.z));
    vec2 p = n.xz * invL1; // Use X and Z for the plane

    // Fold based on Y (The World-Space Up axis)
    if (n.y < 0.0) {
        float oldX = p.x;
        p.x = (1.0 - abs(p.y)) * (oldX >= 0.0 ? 1.0 : -1.0);
        p.y = (1.0 - abs(oldX)) * (p.y >= 0.0 ? 1.0 : -1.0);
    }

    return p * 0.5 + 0.5;
}

void main(void) {
    // Normal (World-space)
    vec3 world_space_normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);
    gNormal = packNormal(world_space_normal);

    // Albedo and Specular (Shininess)
    vec4 albedo = getSimpleMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);

    // Output materialIndex in alpha instead of shininess
    gAlbedoSpec = vec4(albedo.rgb, float(from_vs.materialIndex) / 255.0);

    // Ambient Color (Only from map)
    gAmbient.rgb = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);
}
