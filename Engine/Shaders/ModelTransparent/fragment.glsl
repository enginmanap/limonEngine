
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

void main(void) {
        vec4 objectColor = getMaterialAlbedo(from_vs.materialIndex, from_vs.textureCoord);

        vec3 normal = getMaterialNormal(from_vs.materialIndex, from_vs.textureCoord, from_vs.normal);

        vec3 lightingColorFactor = getMaterialAmbient(from_vs.materialIndex, from_vs.textureCoord);

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
                if(LightSources.lights[i].type == 1) {//directional light
                    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
                    shadow = ShadowCalculationDirectional(i, from_vs.fragPos, abs(fragPosViewSpace.z), normal);
                } else if (LightSources.lights[i].type == 2){//point light
                    shadow = ShadowCalculationPoint(from_vs.fragPos, bias, viewDistance, i);
                }
                lightingColorFactor += ((1.0 - shadow) * (diffuseRate + specularRate) * LightSources.lights[i].color) + LightSources.lights[i].ambient;
            }
        }
        outputColor = vec4(
        min(lightingColorFactor.x, 1.0),
        min(lightingColorFactor.y, 1.0),
        min(lightingColorFactor.z, 1.0),
        1.0) * objectColor;

}
