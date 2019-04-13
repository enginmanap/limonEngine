#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D diffuseSpecularLighted;
uniform sampler2D ambient;
uniform sampler2D ssao;
uniform sampler2D depthMap;

void main()
{

    vec4 baseColor = texture(diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    vec3 ambientFactor = texture(ambient, from_vs.textureCoordinates).rgb;
    float ssao = texture(ssao, from_vs.textureCoordinates).r;

    // we already added %100 ambient, but we should have occluded ssao of it. Now remove that part
    // so if ssao = 0.7, we should remove ambient * 0.7. The issue here is, what if the base color was clamped
    finalColor = baseColor - (vec4(ambientFactor, 0.0) * ssao);
    gl_FragDepth = texture(depthMap, from_vs.textureCoordinates).r;
}



