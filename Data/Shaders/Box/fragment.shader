#version 330

struct Light {
    vec3 position;
    vec3 color;
};

uniform sampler2D boxSampler;
uniform vec3 cameraPosition;
uniform vec3 ambientColor;
uniform float specularStrength;
uniform Light light;

in vec2 vs_fs_textureCoord;
in vec3 vs_fs_normal;
in vec3 vs_fs_fragPos;
out vec4 finalColor;


void main(void)
{
		vec4 objectColor = texture(boxSampler, vs_fs_textureCoord);

        // Diffuse Lighting
        vec3 lightDirectory = normalize(light.position - vs_fs_fragPos);
        float diffuseRate = max(dot(vs_fs_normal, lightDirectory), 0.0);

        // Specular
        vec3 viewDirectory = normalize(cameraPosition - vs_fs_fragPos);
        vec3 reflectDirectory = reflect(-lightDirectory, vs_fs_normal);
        float specularRate = pow(max(dot(viewDirectory, reflectDirectory), 0.0), 32);
        specularRate = specularStrength * specularRate;//same variable as above

        vec3 lightingColorFactor = ambientColor + (diffuseRate + specularRate) * light.color;

        finalColor = vec4(lightingColorFactor,1.0f) * objectColor;

}
