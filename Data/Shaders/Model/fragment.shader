#version 330

#define NR_POINT_LIGHTS 4

struct Material {
    vec3 ambient;
    vec3 specular;//currently not used. Should be a map
    float shininess;
};

struct LightSource
{
		vec3 lightPos;
		vec3 lightColor;
};

in VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
} from_vs;

out vec4 finalColor;

uniform Material material;
uniform sampler2D diffuseSampler;
uniform vec3 cameraPosition;

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;





void main(void)
{
		vec4 objectColor = texture(diffuseSampler, from_vs.textureCoord);
		vec3 lightingColorFactor = vec3(0,0,0);
        for(int i=0; i < NR_POINT_LIGHTS; ++i){
            // Diffuse Lighting
            vec3 lightDirectory = normalize(LightSources.lights[i].lightPos - from_vs.fragPos);
            float diffuseRate = max(dot(from_vs.normal, lightDirectory), 0.0);

            // Specular
            vec3 viewDirectory = normalize(cameraPosition - from_vs.fragPos);
            vec3 reflectDirectory = reflect(-lightDirectory, from_vs.normal);
            float specularRate = pow(max(dot(viewDirectory, reflectDirectory), 0.0), 32);
            specularRate = material.shininess * specularRate;//same variable as above

            lightingColorFactor += material.ambient + (diffuseRate + specularRate) * LightSources.lights[i].lightColor;
        }

        finalColor = vec4(lightingColorFactor,1.0f) * objectColor;

}
