//
// Created by Engin Manap on 21/12/2024.
//

#include "RenderListData.h"

#include "../GameObjects/Model.h"
#include "../Assets/MeshAsset.h"
#include <algorithm>
#include <iostream>

namespace RenderListInternals {

RenderListData::LodBand_MaterialSlot_MeshRenderListId RenderListData::pack_LodBand_MaterialSlot_MeshRenderListId(uint32_t lodBand, uint32_t materialSlot, uint32_t meshRenderListId) {
    return ((((uint64_t)lodBand << MATERIAL_SLOT_BITS) | materialSlot) << MESH_RENDER_LIST_ID_BITS) | meshRenderListId;
}

uint32_t RenderListData::drawOrderBand(uint32_t lod, bool neverSimplified) {
    static_assert(LOD_BAND_COUNT == MeshAsset::LOD_MAX_LEVEL_COUNT, "a LOD level with no band would index past the bands");
    return neverSimplified ? LOD_BAND_COUNT - 1 : lod;
}

void RenderListData::addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth, int32_t rigIdOverride) {
    if (meshAsset->getRenderListId() == MeshAsset::NO_RENDER_LIST_ID) {
        std::cerr << "Mesh of model " << model->getWorldObjectID() << " reached a render list without its GPU part, or after its model was freed!" << std::endl;
        exit(-1);
    }
    const bool neverSimplified = model->getModelAsset()->getLodLadder().getMeshLodCount() <= 1;
    const DrawDataListIndex drawIndex = findOrAppendDrawData(drawDataSet, buildData, lod, neverSimplified, material, meshAsset);
    updateDrawData(drawDataSet.drawDataList[drawIndex], model, lod, maxDepth);
    const uint32_t rigId = rigIdOverride >= 0 ? (uint32_t)rigIdOverride : model->getRigId();
    appendDrawInstanceRequest(buildData, drawIndex, glm::uvec4(model->getWorldObjectID(), material->getMaterialIndex(), rigId, 0));
}

RenderListData::DrawDataListIndex RenderListData::findOrAppendDrawData(DrawDataSet &drawDataSet, BuildData &buildData, uint32_t lod, bool neverSimplified,
                                                              const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset) {
    const uint32_t materialSlot = material->getMaterialIndex();
    const LodBand_MaterialSlot_MeshRenderListId drawKey = pack_LodBand_MaterialSlot_MeshRenderListId(drawOrderBand(lod, neverSimplified), materialSlot, meshAsset->getRenderListId());
    std::vector<DrawData>& drawDataList = drawDataSet.drawDataList;
    const DrawDataListIndex* existing = buildData.drawDataListIndexBy_LodBand_MaterialSlot_MeshRenderListId.find(drawKey);
    if (existing != nullptr) {
        if (drawDataList[*existing].material != material) {
            std::cerr << "Two materials share material slot " << materialSlot << " in one render list!" << std::endl;
            exit(-1);
        }
        if (drawDataList[*existing].mesh != meshAsset) {
            std::cerr << "Two meshes share render list id " << meshAsset->getRenderListId() << " in one render list!" << std::endl;
            exit(-1);
        }
        return *existing;
    }
    const DrawDataListIndex drawIndex = drawDataSet.usedDrawDataCount++;
    if (drawIndex == drawDataList.size()) {
        drawDataList.emplace_back();
    }
    DrawData& newDraw = drawDataList[drawIndex];
    newDraw = DrawData();
    newDraw.material = material;
    newDraw.mesh = meshAsset;
    newDraw.neverSimplified = neverSimplified;
    buildData.drawDataListIndexBy_LodBand_MaterialSlot_MeshRenderListId.insert(drawKey, drawIndex);
    return drawIndex;
}

void RenderListData::updateDrawData(DrawData &draw, const Model *model, uint32_t lod, float maxDepth) {
    draw.lod = lod;
    draw.isAnimated = draw.isAnimated || model->isAnimated();
    draw.needsPose = draw.needsPose || model->isAnimated() || model->getParentBoneID() != -1;
    draw.depth = std::max(draw.depth, maxDepth);
}

void RenderListData::appendDrawInstanceRequest(BuildData &buildData, DrawDataListIndex drawIndex, const glm::uvec4 &instance) {
    buildData.drawInstanceRequests.push_back(DrawInstanceRequest{drawIndex, instance});
}

void RenderListData::collectModelsNeedingPose(std::vector<uint32_t> &modelIds) const {
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        const DrawData& draw = drawDataSet.drawDataList[drawIndex];
        if (!draw.needsPose) {
            continue;
        }
        for (AllDrawInstancesHolderIndex instanceIndex = draw.firstInstance; instanceIndex < draw.firstInstance + draw.instanceCount; ++instanceIndex) {
            modelIds.emplace_back(finalizedData.allDrawInstancesHolder[instanceIndex].x);
        }
    }
}

/**
 * Uploads the batch model indexes, and renders the batch
 *
 * Sharing AnimationState and Material state as minor optimisation, so next batch don't need to
 * set them again if no change
 */
void RenderListData::processRenderBatch(GraphicsInterface *graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated,
                            bool &lastAnimationState, std::shared_ptr<const Material> &lastMaterial) const {
    if (pendingDraws.empty()) {
        return;
    }
    graphicsWrapper->setModelIndexesUBO(batchIndices);
    uint32_t materialSwitchCount = 0;
    for (const PendingDraw &pendingDraw : pendingDraws) {
        const DrawData& draw = drawDataSet.drawDataList[pendingDraw.drawIndex];
        if (!forceNotAnimated && lastAnimationState != draw.isAnimated) {
            renderProgram->setUniform("isAnimated", draw.isAnimated);
            lastAnimationState = draw.isAnimated;
        }
        if (renderProgram->isMaterialRequired() && lastMaterial != draw.material) {
            draw.material->activateTextures(graphicsWrapper);
            ++materialSwitchCount;
        }
        lastMaterial = draw.material;
        renderProgram->setUniform("modelIndexOffset", (int)pendingDraw.batchIndicesOffset);
        graphicsWrapper->renderInstanced(renderProgram->getID(), draw.mesh->getVao(), draw.mesh->getEbo(),
                                         draw.mesh->getTriangleCount()[draw.lod] * 3,
                                         draw.mesh->getOffsets()[draw.lod], pendingDraw.instanceCount);
    }
    graphicsWrapper->reportBatch(materialSwitchCount);
    pendingDraws.clear();
    batchIndices.clear();
}

void RenderListData::render(GraphicsInterface *graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated) const {
    if (drawDataSet.usedDrawDataCount == 0) {
        return;
    }
    bool lastAnimationState = false;
    if (forceNotAnimated) {
        renderProgram->setUniform("isAnimated", false);
    } else {
        lastAnimationState = drawDataSet.drawDataList[0].isAnimated;
        renderProgram->setUniform("isAnimated", lastAnimationState);
    }

    const uint32_t batchCapacity = graphicsWrapper->getModelIndexBatchCapacity();
    std::shared_ptr<const Material> lastMaterial = nullptr;
    batchIndices.clear();
    pendingDraws.clear();

    // instead of updating the model indice ubo for each draw, we fill it with as many draws as fit and write once.
    // otherwise each ubo write creates a flush/block.
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        const DrawData& draw = drawDataSet.drawDataList[drawIndex];
        uint32_t consumedInstanceCount = 0;
        while (consumedInstanceCount < draw.instanceCount) {
            uint32_t remainingInstanceCount = draw.instanceCount - consumedInstanceCount;
            uint32_t freeSlotCount = batchCapacity - (uint32_t)batchIndices.size();
            if (remainingInstanceCount > freeSlotCount && !batchIndices.empty()) {
                // This draw's instances don't fit the batch anymore, so render and empty the batch so they would fit
                processRenderBatch(graphicsWrapper, renderProgram, forceNotAnimated, lastAnimationState, lastMaterial);
                freeSlotCount = batchCapacity;
            }
            //If the draw has more instances than a whole batch, this splits it into pieces that fit
            uint32_t pieceInstanceCount = std::min(remainingInstanceCount, freeSlotCount);
            pendingDraws.push_back(PendingDraw{drawIndex, (uint32_t)batchIndices.size(), pieceInstanceCount});

            const std::vector<glm::uvec4>::const_iterator pieceStart = finalizedData.allDrawInstancesHolder.begin() + draw.firstInstance + consumedInstanceCount;
            batchIndices.insert(batchIndices.end(), pieceStart, pieceStart + pieceInstanceCount);
            consumedInstanceCount += pieceInstanceCount;
        }
    }
    processRenderBatch(graphicsWrapper, renderProgram, forceNotAnimated, lastAnimationState, lastMaterial);
}

void RenderListData::clear() {
    //dropping the references lets an unloaded material or mesh go, the draw entries themselves are reused
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        drawDataSet.drawDataList[drawIndex].material.reset();
        drawDataSet.drawDataList[drawIndex].mesh.reset();
    }
    drawDataSet.usedDrawDataCount = 0;
    buildData.drawDataListIndexBy_LodBand_MaterialSlot_MeshRenderListId.clear();
    buildData.drawInstanceRequests.clear();
    finalizedData.allDrawInstancesHolder.clear();
}

}
