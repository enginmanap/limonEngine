#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D diffuseSpecularLighted;
uniform sampler2D ambientWithSSAO;

void main()
{

    vec4 baseColor = texture(diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    vec3 ambientFactor = texture(ambientWithSSAO, from_vs.textureCoordinates).rgb;

    finalColor = baseColor + vec4(ambientFactor, 0.0);
}



