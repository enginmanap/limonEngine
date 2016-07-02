#version 330

#define NR_POINT_LIGHTS 4

uniform sampler2D boxSampler;
uniform vec3 cameraPosition;
uniform vec3 ambientColor;
uniform float specularStrength;

struct LightSource
{
		vec3 lightPos;
		vec3 lightColor;
};

layout (std140) uniform LightSourceBlock
{
    LightSource lights[NR_POINT_LIGHTS];
} LightSources;


in vec2 vs_fs_textureCoord;
in vec3 vs_fs_normal;
in vec3 vs_fs_fragPos;
out vec4 finalColor;


void main(void)
{
		vec4 objectColor = texture(boxSampler, vs_fs_textureCoord);
		vec3 lightingColorFactor = vec3(0,0,0);
        for(int i=0; i < NR_POINT_LIGHTS; ++i){
            // Diffuse Lighting
            vec3 lightDirectory = normalize(LightSources.lights[i].lightPos - vs_fs_fragPos);
            float diffuseRate = max(dot(vs_fs_normal, lightDirectory), 0.0);

            // Specular
            vec3 viewDirectory = normalize(cameraPosition - vs_fs_fragPos);
            vec3 reflectDirectory = reflect(-lightDirectory, vs_fs_normal);
            float specularRate = pow(max(dot(viewDirectory, reflectDirectory), 0.0), 32);
            specularRate = specularStrength * specularRate;//same variable as above

            lightingColorFactor += ambientColor + (diffuseRate + specularRate) * LightSources.lights[i].lightColor;
        }

        finalColor = vec4(lightingColorFactor,1.0f) * objectColor;

}
