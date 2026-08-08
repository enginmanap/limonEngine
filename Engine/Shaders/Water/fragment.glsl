
#import <./Engine/Shaders/Shared/Lights.glsl>
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/PointShadow.frag>
#import <./Engine/Shaders/Shared/DirectionalShadow.frag>
#import <./Engine/Shaders/Shared/Lighting.frag>

out vec4 finalColor;

in VS_FS {
    vec3 worldPos;
    flat vec3 normal; // must match the "flat" qualifier on the vertex shader's output - see there
    vec2 texCoord;
} from_vs;

// Surface albedo authoring - the actual lighting (direction/color/shadows) below comes from the
// scene's real lights via calculateLighting(), not from these.
uniform vec3 shallowColor;
uniform vec3 deepColor;

// A high specular power makes the highlight hyper-sensitive to any residual normal noise (small slope
// changes swing pow(specularRate, SHININESS) a lot near the peak) - keeping this moderate helps the
// glint stay a broad, stable highlight tracking the swell rather than sparkly/chaotic.
const float SHININESS = 32.0;
const vec3 MATERIAL_AMBIENT = vec3(0.02);

void main(void) {
    vec3 viewDir = normalize(playerTransforms.position - from_vs.worldPos);
    float fresnel = pow(1.0 - max(dot(from_vs.normal, viewDir), 0.0), 5.0);
    vec3 albedo = mix(deepColor, shallowColor, fresnel);

    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.worldPos, 1.0);
    float precise_view_z = abs(fragPosViewSpace.z);
    float viewDistance = length(playerTransforms.position - from_vs.worldPos);

    vec3 totalAmbient;
    vec3 lit = calculateLighting(from_vs.worldPos, from_vs.normal, albedo, SHININESS, MATERIAL_AMBIENT,
                                  viewDistance, precise_view_z, gl_FragCoord.z, totalAmbient);

    finalColor = vec4(lit, 1.0);
}
