#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D diffuseSpecularLighted;
uniform sampler2D ambient;
uniform sampler2D ssao;

void main()
{

    vec4 baseColor = texture(diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    vec3 ambientFactor = texture(ambient, from_vs.textureCoordinates).rgb;
    float ssao = texture(ssao, from_vs.textureCoordinates).r;

    finalColor = baseColor - (vec4(ambientFactor, 0.0) * ssao);
}



