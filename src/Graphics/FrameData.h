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
    std::vector<glm::vec4> modelTransformTexels = std::vector<glm::vec4>(2 * 4 * NR_MAX_MODELS);//row 0 world transforms, row 1 normal matrices
    uint32_t usedModelTransformColumns = 0;
    std::vector<glm::vec4> boneTransformTexels;//one row per rig, grows with the highest rig written
    uint32_t usedBoneTransformRows = 0;
    UniformBlockData lightBlock;//refilled on ticks, kept to reuse the allocation
    UniformBlockData playerBlock;

    void setModelTransform(uint32_t modelID, const glm::mat4& worldTransform) {
        if (modelID >= NR_MAX_MODELS) {
            std::cerr << "Model ID " << modelID << " is past the model transform texture, it can't be rendered." << std::endl;
            return;
        }
        const uint32_t rowWidth = 4 * NR_MAX_MODELS;
        const glm::mat4 normalMatrix = glm::transpose(glm::inverse(worldTransform));
        for (uint32_t column = 0; column < 4; ++column) {
            modelTransformTexels[4 * modelID + column] = worldTransform[column];
            modelTransformTexels[rowWidth + 4 * modelID + column] = column < 3 ? normalMatrix[column] : glm::vec4(0.0f);
        }
        usedModelTransformColumns = std::max(usedModelTransformColumns, 4 * (modelID + 1));
    }

    void setBoneTransforms(uint32_t rigID, const std::vector<glm::mat4>& boneTransforms) {
        if (rigID >= NR_MAX_MODELS) {
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
        //only the used columns of the two rows, a full width write would also send the unused tail
        for (uint32_t row = 0; row < 2 && usedModelTransformColumns != 0; ++row) {
            graphicsWrapper->writeFrameBufferedTexture(handles.modelTransformTexture, 0, static_cast<int>(row), static_cast<int>(usedModelTransformColumns), 1,
                                                       GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::FLOAT,
                                                       modelTransformTexels.data() + row * 4 * NR_MAX_MODELS);
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
