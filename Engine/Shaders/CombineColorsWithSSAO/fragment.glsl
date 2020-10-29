#version 330 core

out vec4 finalColor;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_diffuseSpecularLighted;
uniform sampler2D pre_ambient;
uniform sampler2D pre_ssao;
uniform sampler2D pre_depthMap;

void main()
{

    vec4 baseColor = texture(pre_diffuseSpecularLighted, from_vs.textureCoordinates).rgba;
    vec3 ambientFactor = texture(pre_ambient, from_vs.textureCoordinates).rgb;
    float ssao = texture(pre_ssao, from_vs.textureCoordinates).r;

    // we already added %100 ambient, but we should have occluded ssao of it. Now remove that part
    // so if ssao = 0.7, we should remove ambient * 0.7. The issue here is, what if the base color was clamped
    finalColor = baseColor - (vec4(ambientFactor, 0.0) * ssao);
    gl_FragDepth = texture(pre_depthMap, from_vs.textureCoordinates).r;
}



