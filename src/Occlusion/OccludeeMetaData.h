//
// Created by engin on 03/03/2026.
//

#ifndef LIMONENGINE_OCCLUDERMETADATA_H
#define LIMONENGINE_OCCLUDERMETADATA_H
#include "GameObjects/Model.h"


//one per model, its box is the model's. A visible one adds all of its meshes
struct OcculudeeMetaData {
    const Model* model;
    uint32_t lod;
    float averageDepth;
    RenderList* renderList;
    const std::vector<uint64_t>* renderTags;//renderList's key, a model with differing meshes only adds the ones matching it

    OcculudeeMetaData() = default;

    OcculudeeMetaData(const Model * model, uint32_t lod, float averageDepth, RenderList* renderList, const std::vector<uint64_t>* renderTags) :
    model(model), lod(lod), averageDepth(averageDepth), renderList(renderList), renderTags(renderTags) {};
};
#endif //LIMONENGINE_OCCLUDERMETADATA_H
