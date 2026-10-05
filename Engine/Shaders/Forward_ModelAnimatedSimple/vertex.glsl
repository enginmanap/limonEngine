#import <./Engine/Shaders/Shared/PlayerInformation.glsl>

// Experiment pipeline, see Engine/forward_split_renderPipeline.xml. Skinning is unconditional here,
// there is no isAnimated branch, this stage only ever receives animated tags.
// The block below is a copy of Shared/ModelRendering.vert with nothing tying them together, change
// the model index or transform texture layout in the engine and this drifts silently.

#define NR_BONE 128

layout (location = 2) in vec4 position;
layout (location = 3) in vec2 textureCoordinate;
layout (location = 4) in vec3 normal;
layout (location = 5) in uvec4 boneIDs;
layout (location = 6) in vec4 boneWeights;

uniform sampler2D allModelTransformsTexture;
uniform sampler2D allBoneTransformsTexture;

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

mat4 _getMatrixFromRigTexture(uint rigIndex, uint boneIndex) {
    mat4 matrix;
    matrix[0] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u)    , int(rigIndex)), 0);
    matrix[1] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 1, int(rigIndex)), 0);
    matrix[2] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 2, int(rigIndex)), 0);
    matrix[3] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 3, int(rigIndex)), 0);
    return matrix;
}

void main(void) {
    to_fs.textureCoord = textureCoordinate;

    uint skeletonId = getModelIndexEntry().z;
    mat4 boneTransform  = _getMatrixFromRigTexture(skeletonId, boneIDs[0]) * boneWeights[0];
         boneTransform += _getMatrixFromRigTexture(skeletonId, boneIDs[1]) * boneWeights[1];
         boneTransform += _getMatrixFromRigTexture(skeletonId, boneIDs[2]) * boneWeights[2];
         boneTransform += _getMatrixFromRigTexture(skeletonId, boneIDs[3]) * boneWeights[3];

    vec4 localPosition = boneTransform * position;
    vec3 localNormal = mat3(boneTransform) * normal;

    int modelID = int(getModelIndexEntry().x);
    int modelsPerBand = MODEL_TRANSFORM_TEXTURE_WIDTH / 4;
    ivec2 modelTexel = ivec2(4 * (modelID % modelsPerBand), 2 * (modelID / modelsPerBand));

    mat4 modelTransform;
    modelTransform[0] = texelFetch(allModelTransformsTexture, modelTexel, 0);
    modelTransform[1] = texelFetch(allModelTransformsTexture, modelTexel + ivec2(1, 0), 0);
    modelTransform[2] = texelFetch(allModelTransformsTexture, modelTexel + ivec2(0, 1), 0);
    modelTransform[3] = texelFetch(allModelTransformsTexture, modelTexel + ivec2(1, 1), 0);

    mat3 transposeInverseModelTransform;
    transposeInverseModelTransform[0] = texelFetch(allModelTransformsTexture, modelTexel + ivec2(2, 0), 0).xyz;
    transposeInverseModelTransform[1] = texelFetch(allModelTransformsTexture, modelTexel + ivec2(3, 0), 0).xyz;
    transposeInverseModelTransform[2] = texelFetch(allModelTransformsTexture, modelTexel + ivec2(2, 1), 0).xyz;

    to_fs.fragPos = vec3(modelTransform * localPosition);
    to_fs.normal = normalize(transposeInverseModelTransform * localNormal);

    to_fs.materialIndex = int(getModelIndexEntry().y);
    gl_Position = playerTransforms.cameraProjection * vec4(to_fs.fragPos, 1.0);
}
