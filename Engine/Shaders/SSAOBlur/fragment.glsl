#version 330 core

layout (location = 1) out float occlusion;

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

uniform sampler2D pre_ssaoResult;

void main() {

//we want depth aware
	float selfDepth = texture(pre_ssaoResult, from_vs.textureCoordinates).r;

    vec2 texelSize = 1.0 / vec2(textureSize(pre_ssaoResult, 0));
    float result = 0.0;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
			float sampleDepth = texture(pre_ssaoResult, from_vs.textureCoordinates + offset).r;
			if(abs(sampleDepth - selfDepth) < 0.4) {
				result += sampleDepth;
			} else {
				result += selfDepth;
			}
        }
    }
    occlusion = result / (5.0 *5.0 );
}