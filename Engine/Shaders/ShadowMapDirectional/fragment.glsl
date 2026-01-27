#version 330 core

#define NR_MAX_MATERIALS 200

layout(location = 0) out vec2 FragColor;

in VS_FS {
    vec2 textureCoord;
    flat int materialIndex;
} from_vs;

struct material {
    vec3 ambient;
    float shininess;
    vec3 diffuse;
    int isMap; 	//using the last 4, ambient=8, diffuse=4, specular=2, opacity = 1
};

layout (std140) uniform MaterialInformationBlock {
    material materials[NR_MAX_MATERIALS];
} AllMaterialsArray;

uniform float alphaThreshold;

uniform bool enableDithering;
uniform float ditherOpacity;  // 0.0 = Invisible, 1.0 = Opaque

const float EVSM_EXPONENT = 20.0;

uniform sampler2D diffuseSampler;
uniform sampler2D opacitySampler;

// 4x4 Bayer Dithering
const float bayer4x4[16] = float[](
0.0/16.0, 8.0/16.0, 2.0/16.0, 10.0/16.0,
12.0/16.0, 4.0/16.0, 14.0/16.0, 6.0/16.0,
3.0/16.0, 11.0/16.0, 1.0/16.0, 9.0/16.0,
15.0/16.0, 7.0/16.0, 13.0/16.0, 5.0/16.0
);

void main() {
    vec4 objectColor;
    float alpha = 1.0;

    if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0004)!=0) {
        if((AllMaterialsArray.materials[from_vs.materialIndex].isMap & 0x0001)!=0) { //if there is a opacity map, and it with diffuse
             vec4 opacity = texture(opacitySampler, from_vs.textureCoord);
             objectColor = texture(diffuseSampler, from_vs.textureCoord);
             objectColor.w =  opacity.a;//FIXME some other textures used x
        } else {
            objectColor = texture(diffuseSampler, from_vs.textureCoord);
        }
        alpha = objectColor.w;
        if(alpha < 0.95) {
            discard;
        }
    }

    // dithering for transparent objects
    if (alpha < 0.95) {
        // Find pixel position in the 4x4 grid
        int x = int(gl_FragCoord.x) % 4;
        int y = int(gl_FragCoord.y) % 4;
        int index = y * 4 + x;

        // Get the threshold from the matrix
        float limit = bayer4x4[index];

        float finalOpacity = alpha * 0.5;
        if (finalOpacity < limit) {
            discard;
        }
    }

    float depth = gl_FragCoord.z;
    depth = clamp(depth, 0.0, 1.0);

    float pos = exp(EVSM_EXPONENT * depth);
    FragColor = vec2(pos, pos * pos);
}