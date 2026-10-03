#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

// Experiment pipeline, see Engine/forward_split_renderPipeline.xml. No skinning at all, so the bone
// texture and the isAnimated branch are gone and this compiles at 4 threads instead of 2.
// The block below is a copy of Shared/ModelRendering.vert with nothing tying them together, change
// the model index or transform texture layout in the engine and this drifts silently.

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;

uniform sampler2D allModelTransformsTexture;

//NR_MODEL_INDEX_BATCH comes from shader header sent by backend.
layout (std140) uniform ModelIndexBlock {
    uvec4 models[NR_MODEL_INDEX_BATCH];
} instance;

uniform int modelIndexOffset;

out VS_FS {
    vec2 textureCoord;
    vec3 normal;
    vec3 fragPos;
    flat int materialIndex;
} to_fs;

// gl_InstanceID starts from 0 for each model, real difference is modelIndexOffset
uvec4 getModelIndexEntry() {
    return instance.models[gl_InstanceID + modelIndexOffset];
}

void main(void) {
    to_fs.textureCoord = textureCoordinate;

    int modelOffset = 4 * int(getModelIndexEntry().x);

    mat4 modelTransform;
    modelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 0), 0);
    modelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 0), 0);
    modelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 0), 0);
    modelTransform[3] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 3, 0), 0);

    mat3 transposeInverseModelTransform;
    transposeInverseModelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 1), 0).xyz;
    transposeInverseModelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 1), 0).xyz;
    transposeInverseModelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 1), 0).xyz;

    to_fs.fragPos = vec3(modelTransform * position);
    to_fs.normal = normalize(transposeInverseModelTransform * normal);

    to_fs.materialIndex = int(getModelIndexEntry().y);
    gl_Position = playerTransforms.cameraProjection * vec4(to_fs.fragPos, 1.0);
}
