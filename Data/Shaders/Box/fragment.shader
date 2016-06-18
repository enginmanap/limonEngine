#version 330

in vec2 vs_fs_textureCoord;
in vec3 vs_fs_normal;
in vec3 vs_fs_fragPos;
out vec4 finalColor;

uniform sampler2D boxSampler;

void main(void)
{

        //FIXME these should not be hardcoded
		vec3 lightPos =  vec3(25.0f, 50.0f, 25.0f);
		vec3 lightColor =  vec3(1.0f, 1.0f, 1.0f);


		vec4 objectColor = texture(boxSampler, vs_fs_textureCoord);

        // Diffuse Lighting
        vec3 lightDir = normalize(lightPos - vs_fs_fragPos);
        float diffuseRate = max(dot(vs_fs_normal, lightDir), 0.0);
        vec3 diffuseColorFactor = diffuseRate * lightColor;

        finalColor = vec4(diffuseColorFactor,1.0f) * objectColor;

}
