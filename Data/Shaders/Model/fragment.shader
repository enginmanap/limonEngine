#version 330

#define NR_POINT_LIGHTS 4

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
};

in VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    vec4 fragPosLightSpace;
} from_vs;

out vec4 finalColor;

uniform Material material;
uniform sampler2D diffuseSampler;
uniform sampler2D shadowSampler;
uniform vec3 cameraPosition;

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;



float ShadowCalculation(vec4 fragPosLightSpace, vec3 lightDirection)
{
    // perform perspective divide
    vec3 projectedCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // Transform to [0,1] range
    projectedCoordinates = projectedCoordinates * 0.5 + 0.5;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLightSpace as coords)
    float closestDepth = texture(shadowSampler, projectedCoordinates.xy).r;
    // Get depth of current fragment from light's perspective
    float currentDepth = projectedCoordinates.z;
    // Check whether current frag position is in shadow
    float bias = max(0.05 * (1.0 - dot(from_vs.normal, lightDirection)), 0.005);
    float shadow = (currentDepth - bias) > closestDepth  ? 1.0 : 0.0;

    return shadow;
}

void main(void)
{
		vec4 objectColor = texture(diffuseSampler, from_vs.textureCoord);
		vec3 lightingColorFactor = vec3(0,0,0);
        //FIXME this forces single light
		float shadow = ShadowCalculation(from_vs.fragPosLightSpace, normalize(LightSources.lights[0].position - from_vs.fragPos));

        for(int i=0; i < NR_POINT_LIGHTS; ++i){
            // Diffuse Lighting
            vec3 lightDirectory = normalize(LightSources.lights[i].position - from_vs.fragPos);
            float diffuseRate = max(dot(from_vs.normal, lightDirectory), 0.0);

            // Specular
            vec3 viewDirectory = normalize(cameraPosition - from_vs.fragPos);
            vec3 reflectDirectory = reflect(-lightDirectory, from_vs.normal);
            float specularRate = pow(max(dot(viewDirectory, reflectDirectory), 0.0), 32);
            specularRate = material.shininess * specularRate;//same variable as above

            lightingColorFactor += material.ambient + ((1.0 - shadow) * (diffuseRate + specularRate) * LightSources.lights[i].color);
        }



        finalColor = vec4(lightingColorFactor,1.0f) * objectColor;

}
