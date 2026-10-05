#ifndef LIMONENGINE_FRAMEDATA_H
#define LIMONENGINE_FRAMEDATA_H

#include <algorithm>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Graphics/UniformBlockData.h"
#include "FrameResourceHandles.h"

//one world's CPU side copy of what goes into the frame resources, kept so frames without a tick can send it again
struct FrameData {
    std::vector<glm::vec4> modelTransformTexels;//4x2 block per model, grows in full rows with the highest model written
    uint32_t usedModelTransformRows = 0;
    std::vector<glm::vec4> boneTransformTexels;//one row per rig, grows with the highest rig written
    uint32_t usedBoneTransformRows = 0;
    UniformBlockData lightBlock;//refilled on ticks, kept to reuse the allocation
    UniformBlockData playerBlock;

    void setModelTransform(uint32_t modelID, const glm::mat4& worldTransform) {
        if (modelID >= NR_MAX_MODELS) {
            std::cerr << "Model ID " << modelID << " is past the model transform texture, it can't be rendered." << std::endl;
            return;
        }
        const uint32_t modelsPerBand = MODEL_TRANSFORM_TEXTURE_WIDTH / 4;
        const uint32_t bandRow = 2 * (modelID / modelsPerBand);
        const uint32_t worldTexel = bandRow * MODEL_TRANSFORM_TEXTURE_WIDTH + 4 * (modelID % modelsPerBand);
        const uint32_t normalTexel = worldTexel + 2;
        if (bandRow + 2 > usedModelTransformRows) {
            usedModelTransformRows = bandRow + 2;
            modelTransformTexels.resize(usedModelTransformRows * MODEL_TRANSFORM_TEXTURE_WIDTH);
        }
        const glm::mat4 normalMatrix = glm::transpose(glm::inverse(worldTransform));
        for (uint32_t column = 0; column < 4; ++column) {
            const uint32_t quadOffset = (column / 2) * MODEL_TRANSFORM_TEXTURE_WIDTH + column % 2;
            modelTransformTexels[worldTexel + quadOffset] = worldTransform[column];
            modelTransformTexels[normalTexel + quadOffset] = column < 3 ? normalMatrix[column] : glm::vec4(0.0f);
        }
    }

    void setBoneTransforms(uint32_t rigID, const std::vector<glm::mat4>& boneTransforms) {
        if (rigID >= NR_MAX_RIGS) {
            std::cerr << "Rig ID " << rigID << " is past the bone transform texture, it can't be rendered." << std::endl;
            return;
        }
        if (boneTransforms.size() > NR_BONE) {
            std::cerr << "Too many bones, can't upload more than " << NR_BONE << " ignoring the rest." << std::endl;
        }
        const uint32_t rowWidth = 4 * NR_BONE;
        if (boneTransformTexels.size() < (rigID + 1) * rowWidth) {
            boneTransformTexels.resize((rigID + 1) * rowWidth);
        }
        const size_t copiedBoneCount = std::min(boneTransforms.size(), static_cast<size_t>(NR_BONE));
        for (size_t boneIndex = 0; boneIndex < NR_BONE; ++boneIndex) {
            for (uint32_t column = 0; column < 4; ++column) {
                boneTransformTexels[rigID * rowWidth + 4 * boneIndex + column] = boneIndex < copiedBoneCount ? boneTransforms[boneIndex][column] : glm::vec4(0.0f);
            }
        }
        usedBoneTransformRows = std::max(usedBoneTransformRows, rigID + 1);
    }

    //the frame resources are shared with other worlds and swap every frame, so this sends everything we have, not only what changed
    void upload(GraphicsInterface* graphicsWrapper, const FrameResourceHandles& handles) const {
        if (usedModelTransformRows != 0) {
            graphicsWrapper->writeFrameBufferedTexture(handles.modelTransformTexture, 0, 0, MODEL_TRANSFORM_TEXTURE_WIDTH, static_cast<int>(usedModelTransformRows),
                                                       GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::FLOAT, modelTransformTexels.data());
        }
        if (usedBoneTransformRows != 0) {
            graphicsWrapper->writeFrameBufferedTexture(handles.boneTransformTexture, 0, 0, 4 * NR_BONE, static_cast<int>(usedBoneTransformRows),
                                                       GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::FLOAT, boneTransformTexels.data());
        }
        graphicsWrapper->writeUniformBuffer(handles.lightBlockBuffer, lightBlock);
        graphicsWrapper->writeUniformBuffer(handles.playerBlockBuffer, playerBlock);
    }
};

#endif //LIMONENGINE_FRAMEDATA_H
