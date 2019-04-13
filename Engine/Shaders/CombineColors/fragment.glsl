#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D diffuseSpecularLighted;
uniform sampler2D depthMap;

void main()
{

    vec4 baseColor = texture(diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    finalColor = baseColor;
    gl_FragDepth = texture(depthMap, from_vs.textureCoordinates).r;
}



