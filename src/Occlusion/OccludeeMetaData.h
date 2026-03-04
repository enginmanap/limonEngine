//
// Created by engin on 03/03/2026.
//

#ifndef LIMONENGINE_OCCLUDERMETADATA_H
#define LIMONENGINE_OCCLUDERMETADATA_H
#include "GameObjects/Model.h"


struct OcculudeeMetaData {
    const Model::MeshMeta* meshMeta;
    const Model* model;
    uint32_t lod;
    float averageDepth;
    RenderList* renderList;

    OcculudeeMetaData() = default;

    OcculudeeMetaData(const Model::MeshMeta * meshMeta, const Model * model, uint32_t lod, float averageDepth, RenderList* renderList) :
    meshMeta(meshMeta), model(model), lod(lod), averageDepth(averageDepth), renderList(renderList) {};
};
#endif //LIMONENGINE_OCCLUDERMETADATA_H