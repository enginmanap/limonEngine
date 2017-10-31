#version 330
#extension GL_ARB_texture_cube_map_array : enable

#define NR_POINT_LIGHTS 4

layout (std140) uniform PlayerTransformBlock {
    mat4 camera;
    mat4 projection;
    mat4 cameraProjection;
    vec3 position;
} playerTransforms;

struct Material {
    vec3 ambient;
    vec3 specular;//currently not used. Should be a map
    float shininess;
};

struct LightSource
{
		mat4 lightSpaceMatrix;
		vec3 position;
		vec3 color;
        int type; //0 Directional, 1 point
};

in VS_FS {
    vec3 boneColor;
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace[NR_POINT_LIGHTS];
} from_vs;

out vec4 finalColor;

uniform Material material;
uniform sampler2D diffuseSampler;
uniform sampler2DArray shadowSamplerDirectional;
uniform samplerCubeArray shadowSamplerPoint;
uniform float farPlanePoint;//FIXME this should set once, and shared between programs

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;

vec3 pointSampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float ShadowCalculationDirectional(vec4 fragPosLightSpace, vec3 lightDirection, float bias, float viewDistance, float lightIndex)
{
    // perform perspective divide
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLightSpace as coords)
    float closestDepth = texture(shadowSamplerDirectional, vec3(projectedCoordinates.xy, lightIndex)).r;
    // Get depth of current fragment from light's perspective
    float currentDepth = projectedCoordinates.z;
    float shadow = 0.0;
    if(currentDepth < 1.0){
        vec2 texelSize = 1.0 / textureSize(shadowSamplerDirectional, 0).xy;
        for(int x = -1; x <= 1; ++x){
            for(int y = -1; y <= 1; ++y){
                float pcfDepth = texture(shadowSamplerDirectional, vec3(projectedCoordinates.xy + vec2(x, y) * texelSize, lightIndex)).r;
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
    // use the light to fragment vector to sample from the depth map
    float closestDepth = texture(shadowSamplerPoint, vec4(fragToLight, lightIndex)).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= farPlanePoint;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // now test for shadows
    float shadow = 0.0;
    int samples  = 20;
    float diskRadius = (1.0 + (viewDistance / farPlanePoint)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(shadowSamplerPoint, vec4(fragToLight + pointSampleOffsetDirections[i] * diskRadius, lightIndex)).r;
        closestDepth *= farPlanePoint;   // Undo mapping [0;1]
        if(currentDepth + bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return shadow;
}

void main(void)
{
		vec4 objectColor = texture(diffuseSampler, from_vs.textureCoord);
		vec3 lightingColorFactor = material.ambient;
        float shadow;
        for(int i=0; i < NR_POINT_LIGHTS; ++i){
            // Diffuse Lighting
            vec3 lightDirectory = normalize(LightSources.lights[i].position - from_vs.fragPos);
            float diffuseRate = max(dot(from_vs.normal, lightDirectory), 0.0);
            // Specular
            vec3 viewDirectory = normalize(playerTransforms.position - from_vs.fragPos);
            vec3 reflectDirectory = reflect(-lightDirectory, from_vs.normal);
            float specularRate = max(dot(viewDirectory, reflectDirectory), 0.0);
            if(specularRate != 0 && material.shininess != 0) {
                specularRate = pow(specularRate, material.shininess);
                //specularRate = specularRate * materialSpecular;//we should get specularMap to here
            } else {
                specularRate = 0;
            }
            float viewDistance = length(playerTransforms.position - from_vs.fragPos);
            float bias = 0.0;
            if(LightSources.lights[i].type == 0) {//directional light
                shadow = ShadowCalculationDirectional(from_vs.fragPosLightSpace[i], normalize(LightSources.lights[i].position - from_vs.fragPos), bias, viewDistance, i);
            } else {//point light
                shadow = ShadowCalculationPoint(from_vs.fragPos, bias, viewDistance, i);
            }
            lightingColorFactor += ((1.0 - shadow) * (diffuseRate + specularRate) * LightSources.lights[i].color);
        }

        finalColor = vec4(
        min(lightingColorFactor.x, 1.0),
        min(lightingColorFactor.y, 1.0),
        min(lightingColorFactor.z, 1.0),
        1.0) * objectColor;

}
