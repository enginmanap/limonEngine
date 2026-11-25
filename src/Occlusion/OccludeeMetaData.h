//
// Created by engin on 03/03/2026.
//

#ifndef LIMONENGINE_OCCLUDERMETADATA_H
#define LIMONENGINE_OCCLUDERMETADATA_H
#include "GameObjects/Model.h"


//one per model, its box is the model's. A visible one adds all of its meshes
struct OcculudeeMetaData {
    const CullingEntry* cullingEntry;
    uint32_t lod;
    float averageDepth;
    RenderList* renderList;
    TagMask renderTagMask;//renderList's tags, a model with differing meshes only adds the ones matching it

    OcculudeeMetaData() = default;

    OcculudeeMetaData(const CullingEntry* cullingEntry, uint32_t lod, float averageDepth, RenderList* renderList, const TagMask &renderTagMask) :
    cullingEntry(cullingEntry), lod(lod), averageDepth(averageDepth), renderList(renderList), renderTagMask(renderTagMask) {};
};
#endif //LIMONENGINE_OCCLUDERMETADATA_H
