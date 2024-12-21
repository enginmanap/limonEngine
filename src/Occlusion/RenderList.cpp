//
// Created by Engin Manap on 21/12/2024.
//

#include "RenderList.h"

#include "../GameObjects/Model.h"
void RenderList::addMeshMaterial(const std::shared_ptr<const Material> &material, std::shared_ptr<MeshAsset> &meshAsset, Model *model, uint32_t lod, float depth) {
    auto materialIterator = getOrCreateMaterialEntry(material);
    auto meshIterator = materialIterator->second.getOrCreateMeshEntry(meshAsset);
    meshIterator->second.indices.emplace_back(model->getWorldObjectID(), material->getMaterialIndex(), 0, 0);
    meshIterator->second.depth = std::max(meshIterator->second.depth, depth);
    meshIterator->second.lod = std::min(meshIterator->second.lod, lod);
    meshIterator->second.isAnimated = meshIterator->second.isAnimated || model->isAnimated();
    meshIterator->second.boneTransforms = model->getBoneTransforms();
    maxDepthPerMaterial[material] = std::max(maxDepthPerMaterial[material], depth);
}