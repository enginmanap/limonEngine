//
// Created by Engin Manap on 21/12/2024.
//

#include "RenderList.h"

#include "../GameObjects/Model.h"
#include "../Assets/MeshAsset.h"
#include <algorithm>

static_assert(RenderList::LOD_BAND_COUNT == MeshAsset::LOD_MAX_LEVEL_COUNT, "a LOD level with no band would index past the bands");
static_assert(RenderList::LOD_BAND_COUNT <= 16, "band masks are uint16_t");

void RenderList::addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth, int32_t rigIdOverride) {
    //a model that never simplified stays at level 0 at any distance, the last band keeps a far one from being drawn first
    const uint32_t bandIndex = model->getModelAsset()->getLodLadder().getMeshLodCount() <= 1 ? LOD_BAND_COUNT - 1 : lod;
    Band& band = bands[bandIndex];
    PerMaterialRenderInformation& perMaterialRenderInformation = band.materials[material];
    PerMeshRenderInformation& perMeshRenderInformation = perMaterialRenderInformation.meshes[meshAsset];
    perMeshRenderInformation.lod = lod;
    const uint32_t modelId = model->getWorldObjectID();
    std::vector<glm::uvec4>::const_iterator requestedObjectIterator = std::find_if(perMeshRenderInformation.indices.begin(), perMeshRenderInformation.indices.end(), [modelId](const glm::uvec4& entry) { return entry.x == modelId; });
    if (requestedObjectIterator == perMeshRenderInformation.indices.end()) {
        uint16_t& bandMask = meshBandMasks[meshAsset];
        const uint16_t otherBands = bandMask & ~(uint16_t)(1u << bandIndex);
        if (otherBands != 0) {
            //lists that are not cleared every frame still hold this object at its previous level
            removeFromBands(material, meshAsset, modelId, otherBands);
        }
        bandMask |= (uint16_t)(1u << bandIndex);
        uint32_t rigId = rigIdOverride >= 0 ? (uint32_t)rigIdOverride : model->getRigId();
        perMeshRenderInformation.indices.emplace_back(modelId, material->getMaterialIndex(), rigId, 0);
        perMeshRenderInformation.isAnimated = perMeshRenderInformation.isAnimated || model->isAnimated();
        perMeshRenderInformation.depth = std::max(perMeshRenderInformation.depth, maxDepth);
        perMaterialRenderInformation.depth = std::max(perMaterialRenderInformation.depth, maxDepth);
        perMaterialRenderInformation.meshOrder.clear();
        band.materialOrder.clear();
    }
    //outside the add, a model kept in a list that isn't cleared can be attached to a bone later
    perMeshRenderInformation.needsPose = perMeshRenderInformation.needsPose || model->isAnimated() || model->getParentBoneID() != -1;
}

void RenderList::removeFromBands(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, uint32_t modelId, uint16_t bandMask) {
    for (uint32_t bandIndex = 0; bandIndex < LOD_BAND_COUNT; ++bandIndex) {
        if ((bandMask & (1u << bandIndex)) == 0) {
            continue;
        }
        MaterialMap::iterator materialIterator = bands[bandIndex].materials.find(material);
        if (materialIterator == bands[bandIndex].materials.end()) {
            continue;
        }
        MeshMap::iterator meshIterator = materialIterator->second.meshes.find(meshAsset);
        if (meshIterator == materialIterator->second.meshes.end()) {
            continue;
        }
        //emptied entries stay until cleanUpEmptyRenderLists, removing here would invalidate iterators
        std::vector<glm::uvec4>& indices = meshIterator->second.indices;
        indices.erase(std::remove_if(indices.begin(), indices.end(), [modelId](const glm::uvec4& entry) { return entry.x == modelId; }), indices.end());
    }
}

void RenderList::removeMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, uint32_t modelId) {
    std::unordered_map<std::shared_ptr<MeshAsset>, uint16_t>::const_iterator maskIterator = meshBandMasks.find(meshAsset);
    if (maskIterator != meshBandMasks.end()) {
        removeFromBands(material, meshAsset, modelId, maskIterator->second);
    }
}

/**
 *
 * @param modelId model ID to remove
 */
void RenderList::removeModelFromAll(uint32_t modelId) {
    for (Band& band : bands) {
        for (MaterialEntry& materialEntry : band.materials) {
            for (MeshEntry& meshEntry : materialEntry.second.meshes) {
                std::vector<glm::uvec4>& indices = meshEntry.second.indices;
                indices.erase(std::remove_if(indices.begin(), indices.end(), [modelId](const glm::uvec4& entry) { return entry.x == modelId; }), indices.end());
            }
        }
    }
}

bool RenderList::isMaterialNearer(const MaterialEntry* first, const MaterialEntry* second) {
    return first->second.depth > second->second.depth;
}

bool RenderList::isMeshNearer(const MeshEntry* first, const MeshEntry* second) {
    return first->second.depth > second->second.depth;
}

void RenderList::sortBands() const {
    for (const Band& band : bands) {
        if (band.materialOrder.empty() && !band.materials.empty()) {
            for (const MaterialEntry& materialEntry : band.materials) {
                band.materialOrder.emplace_back(&materialEntry);
            }
            std::sort(band.materialOrder.begin(), band.materialOrder.end(), isMaterialNearer);
        }
        for (const MaterialEntry* materialEntry : band.materialOrder) {
            const PerMaterialRenderInformation& perMaterialRenderInformation = materialEntry->second;
            if (perMaterialRenderInformation.meshOrder.empty() && !perMaterialRenderInformation.meshes.empty()) {
                for (const MeshEntry& meshEntry : perMaterialRenderInformation.meshes) {
                    perMaterialRenderInformation.meshOrder.emplace_back(&meshEntry);
                }
                std::sort(perMaterialRenderInformation.meshOrder.begin(), perMaterialRenderInformation.meshOrder.end(), isMeshNearer);
            }
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
    for (Band& band : bands) {
        for (MaterialMap::iterator materialIterator = band.materials.begin(); materialIterator != band.materials.end();) {
            for (MeshMap::iterator meshIterator = materialIterator->second.meshes.begin(); meshIterator != materialIterator->second.meshes.end();) {
                if (meshIterator->second.indices.empty()) {
                    meshIterator = materialIterator->second.meshes.erase(meshIterator);
                    materialIterator->second.meshOrder.clear();//it pointed at the erased node
                } else {
                    ++meshIterator;
                }
            }
            if (materialIterator->second.meshes.empty()) {
                materialIterator = band.materials.erase(materialIterator);
                band.materialOrder.clear();
            } else {
                ++materialIterator;
            }
        }
    }
}
