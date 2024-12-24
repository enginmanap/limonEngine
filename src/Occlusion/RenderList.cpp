//
// Created by Engin Manap on 21/12/2024.
//

#include "RenderList.h"

#include "../GameObjects/Model.h"
void RenderList::addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, Model *model, uint32_t lod, float maxDepth) {
    auto materialIterator = getOrCreateMaterialEntry(material);
    auto meshIterator = materialIterator->second.getOrCreateMeshEntry(meshAsset);
    auto requestedObjectIterator = std::find_if(meshIterator->second.indices.begin(), meshIterator->second.indices.end(), [model](const glm::uvec4& entry) { return entry.x == model->getWorldObjectID(); });
    if (requestedObjectIterator == meshIterator->second.indices.end()) {
        meshIterator->second.indices.emplace_back(model->getWorldObjectID(), material->getMaterialIndex(), 0, 0);
        meshIterator->second.depth = std::max(meshIterator->second.depth, maxDepth);
        meshIterator->second.lod = std::min(meshIterator->second.lod, lod);
        meshIterator->second.isAnimated = meshIterator->second.isAnimated || model->isAnimated();
        meshIterator->second.boneTransforms = model->getBoneTransforms();
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
