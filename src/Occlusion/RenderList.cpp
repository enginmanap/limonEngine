//
// Created by Engin Manap on 21/12/2024.
//

#include "RenderList.h"

#include "../GameObjects/Model.h"
void RenderList::addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth, int32_t rigIdOverride) {
    std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation>::iterator materialIterator;
    getOrCreateMaterialEntry(material, materialIterator);
    auto meshIterator = materialIterator->second.getOrCreateMeshEntry(meshAsset);
    auto requestedObjectIterator = std::find_if(meshIterator->second.indices.begin(), meshIterator->second.indices.end(), [model](const glm::uvec4& entry) { return entry.x == model->getWorldObjectID(); });
    if (requestedObjectIterator == meshIterator->second.indices.end()) {
        uint32_t rigId = rigIdOverride >= 0 ? (uint32_t)rigIdOverride : model->getRigId();
        meshIterator->second.indices.emplace_back(model->getWorldObjectID(), material->getMaterialIndex(), rigId, 0);
        meshIterator->second.depth = std::max(meshIterator->second.depth, maxDepth);
        meshIterator->second.lod = std::min(meshIterator->second.lod, lod);
        meshIterator->second.isAnimated = meshIterator->second.isAnimated || model->isAnimated();
        materialIterator->second.maxDepthPerMesh[meshAsset] = std::max(materialIterator->second.maxDepthPerMesh[meshAsset], maxDepth);//This is the max depth of this material
        materialIterator->second.meshRenderPriorityMap.clear();//Why? because we don't know if we need to sort the list again
        maxDepthPerMaterial[material] = std::max(maxDepthPerMaterial[material], maxDepth);
        materialRenderPriorityMap.clear();//Why? because we don't know if we need to sort the list again
    }
}

void RenderList::removeMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, uint32_t modelId) {
    auto materialIterator = perMaterialMeshMap.find(material);
    if(materialIterator != perMaterialMeshMap.end()) {
        auto meshIterator = materialIterator->second.meshesToRender.find(meshAsset);
        if (meshIterator != materialIterator->second.meshesToRender.end()) {
            PerMeshRenderInformation& perMeshRenderInformation = meshIterator->second;
            perMeshRenderInformation.indices.erase(
                std::remove_if(perMeshRenderInformation.indices.begin(), perMeshRenderInformation.indices.end(), [modelId](const glm::uvec4& entry) { return entry.x == modelId; }), perMeshRenderInformation.indices.end());
            if (perMeshRenderInformation.indices.empty()) {
                //WE don't remove it intentionally, because it causes iterator invalidation
            }
        }
        materialIterator->second.meshRenderPriorityMap.clear();
        materialRenderPriorityMap.clear();
    }
}

/**
 *
 * @param modelId model ID to remove
 */
void RenderList::removeModelFromAll(uint32_t modelId) {
    for (auto& materialIterator: perMaterialMeshMap) {
        for (auto& meshIterator: materialIterator.second.meshesToRender) {
            PerMeshRenderInformation& perMeshRenderInformation = meshIterator.second;
            perMeshRenderInformation.indices.erase(
                std::remove_if(perMeshRenderInformation.indices.begin(), perMeshRenderInformation.indices.end(), [modelId](const glm::uvec4& entry) { return entry.x == modelId; }), perMeshRenderInformation.indices.end());
            if (perMeshRenderInformation.indices.empty()) {
                //WE don't remove it intentionally, because it causes iterator invalidation
            }
            materialIterator.second.meshRenderPriorityMap.clear();
            materialRenderPriorityMap.clear();
        }
    }
}

/**
 * Uploads the batch model indexes, and renders the batch
 *
 * Sharing AnimationState and Material state as minor optimisation, so next batch don't need to
 * set them again if no change
 */
void RenderList::processRenderBatch(GraphicsInterface *graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated,
                            bool &lastAnimationState, std::shared_ptr<const Material> &lastMaterial) const {
    if (pendingDraws.empty()) {
        return;
    }
    graphicsWrapper->setModelIndexesUBO(batchIndices);
    uint32_t materialSwitchCount = 0;
    for (const PendingDraw &pendingDraw : pendingDraws) {
        if (!forceNotAnimated && lastAnimationState != pendingDraw.isAnimated) {
            renderProgram->setUniform("isAnimated", pendingDraw.isAnimated);
            lastAnimationState = pendingDraw.isAnimated;
        }
        if (renderProgram->isMaterialRequired() && lastMaterial != pendingDraw.material) {
            pendingDraw.material->activateTextures(graphicsWrapper);
            ++materialSwitchCount;
        }
        lastMaterial = pendingDraw.material;
        renderProgram->setUniform("modelIndexOffset", (int)pendingDraw.indexOffset);
        graphicsWrapper->renderInstanced(renderProgram->getID(), pendingDraw.mesh->getVao(), pendingDraw.mesh->getEbo(),
                                         pendingDraw.mesh->getTriangleCount()[pendingDraw.lod] * 3,
                                         pendingDraw.mesh->getOffsets()[pendingDraw.lod], pendingDraw.instanceCount);
    }
    graphicsWrapper->reportBatch(materialSwitchCount);
    pendingDraws.clear();
    batchIndices.clear();
}

void RenderList::render(GraphicsInterface *graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated) const {
   bool lastAnimationState = false;
   RenderListIterator renderListIterator = this->getIterator();
    if (renderListIterator.isEnd()) {
        return;
    }
   if (forceNotAnimated) {
       renderProgram->setUniform("isAnimated", false);
   } else {
       renderProgram->setUniform("isAnimated", renderListIterator.get().isAnimated);
       lastAnimationState = renderListIterator.get().isAnimated;
   }

   const uint32_t batchCapacity = graphicsWrapper->getModelIndexBatchCapacity();
   std::shared_ptr<const Material> lastMaterial = nullptr;
   batchIndices.clear();
   pendingDraws.clear();

   // instead of rendering each list separately, and updating the model indice ubo after each render, we combine the ubo write.
   // otherwise the ubo write creates a flush/block.
   for (; !renderListIterator.isEnd(); ++renderListIterator) {
       const PerMeshRenderInformation& meshRenderInformation = renderListIterator.get();
       if (meshRenderInformation.indices.empty()) {
           std::cerr << "Empty meshInfo" << std::endl;
           continue;
       }
       uint32_t consumedInstanceCount = 0;
       while (consumedInstanceCount < meshRenderInformation.indices.size()) {
           uint32_t remainingInstanceCount = (uint32_t)meshRenderInformation.indices.size() - consumedInstanceCount;
           uint32_t freeSlotCount = batchCapacity - (uint32_t)batchIndices.size();
           if (remainingInstanceCount > freeSlotCount && !batchIndices.empty()) {
               // This model indice list doesn't fit the buffer anymore, so process and empty the buffer so it would fit
               processRenderBatch(graphicsWrapper, renderProgram, forceNotAnimated, lastAnimationState, lastMaterial);
               freeSlotCount = batchCapacity;
           }
           //If the model indice count is more than the buffer itself, this part would split model indices so it will fit
           uint32_t sliceInstanceCount = std::min(remainingInstanceCount, freeSlotCount);
           PendingDraw pendingDraw;
           pendingDraw.mesh = renderListIterator.getMesh();
           pendingDraw.material = renderListIterator.getMaterial();
           pendingDraw.lod = meshRenderInformation.lod;
           pendingDraw.indexOffset = (uint32_t)batchIndices.size();
           pendingDraw.instanceCount = sliceInstanceCount;
           pendingDraw.isAnimated = meshRenderInformation.isAnimated;
           pendingDraws.emplace_back(pendingDraw);

           batchIndices.insert(batchIndices.end(), meshRenderInformation.indices.begin() + consumedInstanceCount,
                               meshRenderInformation.indices.begin() + consumedInstanceCount + sliceInstanceCount);
           consumedInstanceCount += sliceInstanceCount;
       }
   }
   processRenderBatch(graphicsWrapper, renderProgram, forceNotAnimated, lastAnimationState, lastMaterial);
}

void RenderList::cleanUpEmptyRenderLists() {
    //When a model is no longer visible, it will be removed. That means it is possible some Per Mesh Render Informations have no indices. We should clean them up
    auto materialIterator = perMaterialMeshMap.begin();
    int iterationCount = 0;
    for (; materialIterator != perMaterialMeshMap.end();) {
        iterationCount++;
        for (auto it2 = materialIterator->second.meshesToRender.begin(); it2 != materialIterator->second.meshesToRender.end();) {
            if (it2->second.indices.empty()) {
                materialIterator->second.maxDepthPerMesh.erase(it2->first);
                for (auto it3 = materialIterator->second.meshRenderPriorityMap.begin(); it3 != materialIterator->second.meshRenderPriorityMap.end(); ++it3) {
                    if (it3->second == it2->first) {
                        materialIterator->second.meshRenderPriorityMap.erase(it3);
                        break;
                    }
                }
                it2 = materialIterator->second.meshesToRender.erase(it2);
            } else {
                ++it2;
            }
        }
        if (materialIterator->second.meshesToRender.empty()) {
            maxDepthPerMaterial.erase(materialIterator->first);
            for (auto it2 = materialRenderPriorityMap.begin(); it2 != materialRenderPriorityMap.end(); ++it2) {
                if (it2->second == materialIterator->first) {
                    materialRenderPriorityMap.erase(it2);
                    break;
                }
            }
            materialIterator = perMaterialMeshMap.erase(materialIterator);
        } else {
            ++materialIterator;
        }
    }
}
