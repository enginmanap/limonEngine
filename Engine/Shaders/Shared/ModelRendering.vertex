
#define NR_MAX_MODELS 4096
#define NR_BONE 128

uniform sampler2D allModelTransformsTexture;
uniform sampler2D allBoneTransformsTexture;
uniform bool isAnimated;

layout (std140) uniform ModelIndexBlock {
    uvec4 models[NR_MAX_MODELS];
} instance;

// Internal helper function
mat4 _getMatrixFromRigTexture(uint rigIndex, uint boneIndex) {
    mat4 matrix;
    matrix[0] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u)    , int(rigIndex)), 0);
    matrix[1] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 1, int(rigIndex)), 0);
    matrix[2] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 2, int(rigIndex)), 0);
    matrix[3] = texelFetch(allBoneTransformsTexture, ivec2(int(boneIndex*4u) + 3, int(rigIndex)), 0);
    return matrix;
}

// Internal helper function
void _processAnimation(in vec4 inPosition, in vec3 inNormal, in uvec4 boneIDs, in vec4 boneWeights, out vec4 outPosition, out vec3 outNormal) {
    outPosition = inPosition;
    outNormal = inNormal;

    if (isAnimated) {
        uint skeletonId = instance.models[gl_InstanceID].z;
        mat4 boneTransform  = _getMatrixFromRigTexture(skeletonId, boneIDs[0]) * boneWeights[0];
             boneTransform += _getMatrixFromRigTexture(skeletonId, boneIDs[1]) * boneWeights[1];
             boneTransform += _getMatrixFromRigTexture(skeletonId, boneIDs[2]) * boneWeights[2];
             boneTransform += _getMatrixFromRigTexture(skeletonId, boneIDs[3]) * boneWeights[3];

        outPosition = boneTransform * inPosition;
        outNormal = mat3(boneTransform) * inNormal;
    }
}

// Main function to be called by shaders
void calculateWorldPositionAndNormal(in vec4 position, in vec3 normal, in uvec4 boneIDs, in vec4 boneWeights, out vec3 worldPosition, out vec3 worldNormal) {
    vec4 localPosition;
    vec3 localNormal;
    _processAnimation(position, normal, boneIDs, boneWeights, localPosition, localNormal);

    mat4 modelTransform;
    int modelOffset = 4*int(instance.models[gl_InstanceID].x);
    modelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 0), 0);
    modelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 0), 0);
    modelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 0), 0);
    modelTransform[3] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 3, 0), 0);

    mat3 transposeInverseModelTransform;
    transposeInverseModelTransform[0] = texelFetch(allModelTransformsTexture, ivec2(modelOffset    , 1), 0).xyz;
    transposeInverseModelTransform[1] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 1, 1), 0).xyz;
    transposeInverseModelTransform[2] = texelFetch(allModelTransformsTexture, ivec2(modelOffset + 2, 1), 0).xyz;

    worldPosition = vec3(modelTransform * localPosition);
    worldNormal = normalize(transposeInverseModelTransform * localNormal);
}
