#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_diffuseSpecularLighted;
uniform sampler2D pre_depthMap;

void main()
{

    vec4 baseColor = texture(pre_diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    finalColor = baseColor;
    gl_FragDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;
}



