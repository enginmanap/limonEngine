//
// Created by engin on 03/03/2026.
//

#ifndef LIMONENGINE_OCCLUDERMETADATA_H
#define LIMONENGINE_OCCLUDERMETADATA_H
#include "GameObjects/Model.h"


//one per model and camera however many of its lists match, its box is the model's. A visible one goes to every matching list
struct OcculudeeMetaData {
    const CullingEntry* cullingEntry;
    uint32_t lod;
    float averageDepth;

    OcculudeeMetaData() = default;

    OcculudeeMetaData(const CullingEntry* cullingEntry, uint32_t lod, float averageDepth) :
    cullingEntry(cullingEntry), lod(lod), averageDepth(averageDepth) {};
};
#endif //LIMONENGINE_OCCLUDERMETADATA_H
