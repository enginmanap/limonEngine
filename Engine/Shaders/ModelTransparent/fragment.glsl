#version 330
#extension GL_ARB_texture_cube_map_array : enable

#define NR_POINT_LIGHTS 4
#define NR_MAX_MATERIALS 200

#define_option CascadeCount
#define_option CascadeLimitList

layout (location = 0) out vec4 outputColor;

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
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

struct material {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    int isMap; 	//using the last 4, ambient=8, diffuse=4, specular=2, opacity = 1
};

layout (std140) uniform MaterialInformationBlock {
    material materials[NR_MAX_MATERIALS];
} AllMaterialsArray;

in VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[NR_POINT_LIGHTS];
    flat int depthMapLayer;
    flat int materialIndex;
} from_vs;

uniform sampler2DArray pre_shadowDirectional;
uniform samplerCubeArray pre_shadowPoint;

uniform sampler2D ambientSampler;
uniform sampler2D diffuseSampler;
uniform sampler2D specularSampler;
uniform sampler2D opacitySampler;
uniform sampler2D normalSampler;

vec3 pointSampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

uniform vec3 ssaoKernel[128];
uniform int ssaoSampleCount;

float ShadowCalculationDirectional(float bias, int lightIndex){
    float cascadePlaneDistances[CascadeCount] = float[](CascadeLimitList);
    vec4 fragPosViewSpace = playerTransforms.camera * vec4(from_vs.fragPos, 1.0);
    float depthValue = abs(fragPosViewSpace.z);
    int layer = CascadeCount - 1;
    for (int i = 0; i < CascadeCount; ++i) {
        if (depthValue < cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    vec4 fragPosLightSpace = LightSources.lights[lightIndex].shadowMatrices[layer] * vec4(from_vs.fragPos, 1.0);
    // perform perspective divide
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLightSpace as coords)
    float closestDepth = texture(pre_shadowDirectional, vec3(projectedCoordinates.xy, layer)).r;
    // Get depth of current fragment from light's perspective
    float currentDepth = projectedCoordinates.z;
    if (currentDepth  >= 1.0)
    {
        return 0.0;
    }
    float shadow = 0.0;
    if(currentDepth < 1.0){
        vec2 texelSize = 1.0 / textureSize(pre_shadowDirectional, 0).xy;//this has to be level 0, because its not layer but LOD/MIP
        for(int x = -1; x <= 1; ++x){
            for(int y = -1; y <= 1; ++y){
                float pcfDepth = texture(pre_shadowDirectional, vec3(projectedCoordinates.xy + vec2(x, y) * texelSize, layer)).r;
                if(currentDepth + bias > pcfDepth) {
                    shadow += 1.0;
                }
            }
        }
        shadow /= 9.0;
    }

    return shadow;
}

float ShadowCalculationPoint(vec3 fragPos, float bias, float viewDistance, int lightIndex)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - LightSources.lights[lightIndex].position;
    float fragDistance = length(fragToLight);
    if(LightSources.lights[lightIndex].farPlanePoint < fragDistance) {
        return 1.0;//if outside of the active distance, in shadow
    }
    // use the light to fragment vector to sample from the depth map
    float closestDepth = texture(pre_shadowPoint, vec4(fragToLight, lightIndex)).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= LightSources.lights[lightIndex].farPlanePoint;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // now test for shadows
    float shadow = 0.0;
    int samples  = 20;
    float diskRadius = (1.0 + (viewDistance / LightSources.lights[lightIndex].farPlanePoint)) / 25.0;
    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(pre_shadowPoint, vec4(fragToLight + pointSampleOffsetDirections[i] * diskRadius, lightIndex)).r;
        closestDepth *= LightSources.lights[lightIndex].farPlanePoint;   // Undo mapping [0;1]
        if(currentDepth + bias > closestDepth)
            shadow += 1.0;
    }
    //now calculate attenuation
    float attenuation = 1.0 / (LightSources.lights[lightIndex].attenuation.x +
                              (LightSources.lights[lightIndex].attenuation.y * fragDistance) +
                               (LightSources.lights[lightIndex].attenuation.z * fragDistance * fragDistance));
    attenuation = clamp(attenuation, 0.0, 1.0);
    attenuation = 1 - attenuation;
    if(attenuation == 1) {
        shadow = 1.0;
    } else {
        shadow /= float(samples);
        shadow = max(shadow, attenuation);
    }

    return shadow;
}

vec3 calcViewSpacePos(vec3 screen) {
    vec4 temp = vec4(screen.x, screen.y, screen.z, 1);
    temp *= playerTransforms.inverseProjection;
    vec3 camera_space = temp.xyz / temp.w;
    return camera_space;
}

void main(void) {
        vec4 objectColor;
        if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0004)!=0) {
            if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0001)!=0) { //if there is a opacity map, and it with diffuse
                vec4 opacity = texture(opacitySampler, from_vs.textureCoord);
                if(opacity.a < 0.05) {
                    discard;
                }
                objectColor = texture(diffuseSampler, from_vs.textureCoord);
                objectColor.w =  opacity.a;//FIXME some other textures used x
            } else {
                objectColor = texture(diffuseSampler, from_vs.textureCoord);
                if(objectColor.a < 0.05) {
                    discard;
                }
            }
        } else {
            objectColor = vec4(AllMaterialsArray.materials[from_vs.materialIndex].diffuse, 1.0);
        }

        vec3 normal = from_vs.normal;

        if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0010) != 0) {
            normal = -1 * vec3(texture(normalSampler, from_vs.textureCoord));
        }

    vec3 lightingColorFactor;
        if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0008)!=0) {
            lightingColorFactor = vec3(texture(ambientSampler, from_vs.textureCoord));
        } else {
            lightingColorFactor = AllMaterialsArray.materials[from_vs.materialIndex].ambient;
        }

        float shadow;
        for(int i=0; i < NR_POINT_LIGHTS; ++i){
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
                if(specularRate != 0 && AllMaterialsArray.materials[from_vs.materialIndex].shininess != 0) {
                    specularRate = pow(specularRate, AllMaterialsArray.materials[from_vs.materialIndex].shininess);
                    vec3 specularColor = vec3(texture(specularSampler, from_vs.textureCoord));
                    float specularAverage = (specularColor.x + specularColor.y + specularColor.z) / 3;
                    specularRate = specularRate * specularAverage;
                    //specularRate = specularRate * materialSpecular;//we should get specularMap to here
                } else {
                    specularRate = 0;
                }
                float viewDistance = length(playerTransforms.position - from_vs.fragPos);
                float bias = 0.0;
                if(LightSources.lights[i].type == 1) {//directional light
                    shadow = ShadowCalculationDirectional(bias, i);
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
