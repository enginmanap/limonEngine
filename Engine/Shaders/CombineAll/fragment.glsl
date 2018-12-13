#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D diffuseSpecularLighted;

void main()
{

    vec4 baseColor = texture(diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    finalColor = baseColor;
}



