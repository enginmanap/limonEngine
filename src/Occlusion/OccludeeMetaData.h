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

    OcculudeeMetaData() = default;

    OcculudeeMetaData(const Model * model, uint32_t lod, float averageDepth, RenderList* renderList) :
    model(model), lod(lod), averageDepth(averageDepth), renderList(renderList) {};
};
#endif //LIMONENGINE_OCCLUDERMETADATA_H
