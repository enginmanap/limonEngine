
#define_option maximumPointLights
#import <./Engine/Shaders/Shared/PlayerInformation.glsl>
#import <./Engine/Shaders/Shared/Material.frag>

uniform sampler2D Texture;
uniform sampler2DArray TextureArray;
uniform samplerCubeArray TextureCubeArray;
uniform int isArray;
uniform float layer;


in vec2 Frag_UV;
in vec4 Frag_Color;
out vec4 Out_Color;

/** Model rendering definitions */
struct LightSource {
    mat4 shadowMatrices[6];
    vec3 position;
    float farPlanePoint;
    vec3 color;
    int type; //1 Directional, 2 point
    vec3 attenuation;
    vec3 ambient;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[maximumPointLights];
} LightSources;

in VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[maximumPointLights];
    flat int depthMapLayer;
    flat int materialIndex;
} from_vs;

/** Model rendering definitions */

uniform int renderModelIMGUI;

vec4 renderModel() {
    vec4 objectColor = getMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);
    vec3 normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);

    vec3 lightingColorFactor = vec3(0.0, 0.0, 0.0);

    float shadow;
    for(int i=0; i < maximumPointLights; ++i){
        if(LightSources.lights[i].type != 0) {
            // Diffuse Lighting
            vec3 lightDirectory;
            if(LightSources.lights[i].type == 1) {
                lightDirectory = -1.0 * LightSources.lights[i].position;
            } else if(LightSources.lights[i].type == 2) {
                lightDirectory = normalize(LightSources.lights[i].position - from_vs.fragPos);
            }
            float diffuseRate = max(dot(normal, lightDirectory), 0.0);
            // Specular
            vec3 viewDirectory = normalize(playerTransforms.position - from_vs.fragPos);
            vec3 reflectDirectory = reflect(-lightDirectory, normal);
            float specularRate = max(dot(viewDirectory, reflectDirectory), 0.0);
            if(specularRate != 0.0 && AllMaterialsArray.materials[from_vs.materialIndex].shininess != 0.0) {
                specularRate = pow(specularRate, AllMaterialsArray.materials[from_vs.materialIndex].shininess);
                vec3 specularColor = vec3(texture(specularSampler, from_vs.textureCoord));
                float specularAverage = (specularColor.x + specularColor.y + specularColor.z) / 3.0;
                specularRate = specularRate * specularAverage;
                //specularRate = specularRate * materialSpecular;//we should get specularMap to here
            } else {
                specularRate = 0.0;
            }
            float viewDistance = length(playerTransforms.position - from_vs.fragPos);
            float bias = 0.0;
            lightingColorFactor += ((diffuseRate + specularRate) * LightSources.lights[i].color) + LightSources.lights[i].ambient;
        }
    }
    return vec4(
    min(lightingColorFactor.x, 1.0),
    min(lightingColorFactor.y, 1.0),
    min(lightingColorFactor.z, 1.0),
    1.0) * objectColor;
}

void main() {
    if (renderModelIMGUI == 0) {
        if (isArray == 2) {
            if (int(mod(layer, 6.0)) == 0) {
                Out_Color = Frag_Color * texture(TextureCubeArray, vec4(1.0, Frag_UV.st, (layer) / 6.0));
            } else if (int(mod(layer, 6.0)) == 1) {
                Out_Color = Frag_Color * texture(TextureCubeArray, vec4(Frag_UV.s, 1.0, Frag_UV.t, (layer - 1.0) / 6.0));
            } else if (int(mod(layer, 6.0)) == 2) {
                Out_Color = Frag_Color * texture(TextureCubeArray, vec4(Frag_UV.st, 1.0, (layer - 2.0) / 6.0));
            } else if (int(mod(layer, 6.0)) == 3) {
                Out_Color = Frag_Color * texture(TextureCubeArray, vec4(-1.0, Frag_UV.st, (layer - 3.0) / 6.0));
            } else if (int(mod(layer, 6.0)) == 4) {
                Out_Color = Frag_Color * texture(TextureCubeArray, vec4(Frag_UV.s, -1.0, Frag_UV.t, (layer - 4.0) / 6.0));
            } else if (int(mod(layer, 6.0)) == 5) {
                Out_Color = Frag_Color * texture(TextureCubeArray, vec4(Frag_UV.st, -1.0, (layer - 5.0) / 6.0));
            } else {
                Out_Color = vec4(0.0, 0.7, 0.7, 0.0);
            }
        } else if (isArray == 1) {
            Out_Color = Frag_Color * texture(TextureArray, vec3(Frag_UV.st, layer));
        } else {
            Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
        }
    } else {
        Out_Color = renderModel();
    }
}
