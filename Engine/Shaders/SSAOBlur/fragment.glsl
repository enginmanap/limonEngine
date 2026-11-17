
#define_option ssao_width
#define_option display_width
#ifndef ssao_width
#define ssao_width 960
#endif
#ifndef display_width
#define display_width 1920
#endif
//Same formula as SSAOKernelRenderMethod::generateSSAONoiseTexture, change both. Each AO texel rotates the kernel by its
//texel in the rotation tile, so averaging exactly one tile gives every output texel each rotation once. A wider window
//would count some rotations twice and leave the tile pattern visible, a narrower one would not cancel the noise
#define SSAO_ROTATION_TILE_UNCLAMPED ((4 * ssao_width + display_width / 2) / display_width)
#if SSAO_ROTATION_TILE_UNCLAMPED < 1
#define SSAO_ROTATION_TILE 1
#elif SSAO_ROTATION_TILE_UNCLAMPED > 4
#define SSAO_ROTATION_TILE 4
#else
#define SSAO_ROTATION_TILE SSAO_ROTATION_TILE_UNCLAMPED
#endif

in VS_FS {
    vec2 textureCoordinates;
} from_vs;

//occlusion in x and linear view depth in y, as packHalf2x16 halves of one texel. Depth travels with occlusion so one
//gather gets both for 4 texels, instead of a second texture read per texel
uniform sampler2D pre_ssaoResult;

layout (location = 1) out float occlusion;

//relative, because a fixed depth difference rejects too much up close and too little far away
const float RELATIVE_DEPTH_TOLERANCE = 0.05;

//The window covers texel offsets windowStart .. windowStart + SSAO_ROTATION_TILE - 1 on each axis, read as 2x2 gathers.
//Even tiles can't be centered, they sit half a texel low. A 3 wide tile is read with 2x2 gathers and the 4th row
//and column is masked out.
const int windowStart = -(SSAO_ROTATION_TILE / 2);
const int windowEnd = windowStart + SSAO_ROTATION_TILE;
const int gathersPerAxis = (SSAO_ROTATION_TILE + 1) / 2;
//textureGather returns the 2x2 block in this order, relative to its lower left texel
const ivec2 gatherComponentOffsets[4] = ivec2[](ivec2(0, 1), ivec2(1, 1), ivec2(1, 0), ivec2(0, 0));

void main() {
    vec2  texelSize = 1.0 / vec2(textureSize(pre_ssaoResult, 0));
    float centerDepth = unpackHalf2x16(floatBitsToUint(texelFetch(pre_ssaoResult, ivec2(gl_FragCoord.xy), 0).r)).y;
    float depthTolerance = centerDepth * RELATIVE_DEPTH_TOLERANCE;
    float occlusionSum = 0.0, weightSum = 0.0;
    for (int gatherY = 0; gatherY < gathersPerAxis; ++gatherY) {
        for (int gatherX = 0; gatherX < gathersPerAxis; ++gatherX) {
            ivec2 blockLowerLeft = ivec2(windowStart + 2 * gatherX, windowStart + 2 * gatherY);
            //sampling at the corner shared by the 4 texels makes the gather return exactly that block
            vec4 packedBlock = textureGather(pre_ssaoResult, from_vs.textureCoordinates + (vec2(blockLowerLeft) + 0.5) * texelSize);
            for (int component = 0; component < 4; ++component) {
                ivec2 texelOffset = blockLowerLeft + gatherComponentOffsets[component];
                //constant after unrolling, so the mask costs nothing
                float insideWindow = (texelOffset.x < windowEnd && texelOffset.y < windowEnd) ? 1.0 : 0.0;
                vec2 occlusionAndDepth = unpackHalf2x16(floatBitsToUint(packedBlock[component]));
                float weight = insideWindow * step(abs(occlusionAndDepth.y - centerDepth), depthTolerance);
                occlusionSum += occlusionAndDepth.x * weight;
                weightSum    += weight;
            }
        }
    }
    //the center always passes its own depth test, the guard only matters for a NaN depth
    occlusion = (weightSum > 0.0) ? occlusionSum / weightSum : 0.0;
}
