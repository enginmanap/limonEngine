//
// Created by Engin Manap on 06/11/2026.
//

#include "RenderListFinalizer.h"

#include "../Material.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace RenderListInternals {

void RenderListFinalizer::finalize(RenderListData &renderListData) {
    const BuildData &buildData = renderListData.buildData;
    DrawDataSet &drawDataSet = renderListData.drawDataSet;
    FinalizedData &finalizedData = renderListData.finalizedData;

    sortDrawsByBandMaterialDepth(drawDataSet);
    cutIntoMaterialSpans();
    sortSpansByNearestDraw();
    moveDrawsIntoRenderOrder(drawDataSet);

    countInstancesPerDraw(buildData, drawDataSet);
    assignInstanceRanges(drawDataSet);
    scatterInstancesIntoDraws(buildData, finalizedData);
#ifndef NDEBUG
    exitOnDuplicateInstances(drawDataSet, finalizedData);
#endif
}

void RenderListFinalizer::sortDrawsByBandMaterialDepth(const DrawDataSet &drawDataSet) {
    if ((uint64_t)drawDataSet.usedDrawDataCount > (UINT64_C(1) << DRAW_INDEX_BITS)) {
        std::cerr << "Render list has more draws than its sort key can address!" << std::endl;
        exit(-1);
    }
    drawSortKeys.clear();
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        const DrawData& draw = drawDataSet.drawDataList[drawIndex];
        drawSortKeys.emplace_back(pack_LodBand_MaterialSlot_Depth_DrawDataListIndex(RenderListData::drawOrderBand(draw.lod, draw.neverSimplified), draw.material->getMaterialIndex(), draw.depth, drawIndex));
    }
    std::sort(drawSortKeys.begin(), drawSortKeys.end());
}

void RenderListFinalizer::cutIntoMaterialSpans() {
    //a span is as near as its first draw, the sort put the nearest first
    materialSpans.clear();
    materialSpanSortKeys.clear();
    for (DrawSortKeysIndex position = 0; position < drawSortKeys.size(); ++position) {
        if (position == 0 || unpack_LodBand_MaterialSlot(drawSortKeys[position]) != unpack_LodBand_MaterialSlot(drawSortKeys[position - 1])) {
            materialSpanSortKeys.emplace_back(pack_LodBand_Depth_MaterialSpansIndex(drawSortKeys[position], (MaterialSpansIndex)materialSpans.size()));
            materialSpans.push_back(MaterialSpan{position, position});
        }
        materialSpans.back().end = position + 1;
    }
}

void RenderListFinalizer::sortSpansByNearestDraw() {
    std::sort(materialSpanSortKeys.begin(), materialSpanSortKeys.end());
}

void RenderListFinalizer::moveDrawsIntoRenderOrder(DrawDataSet &drawDataSet) {
    //the requests still hold the add time positions, renderOrderIndexByAddIndex translates them
    drawDataInRenderOrder.clear();
    renderOrderIndexByAddIndex.resize(drawDataSet.usedDrawDataCount);
    for (LodBand_Depth_MaterialSpansIndex spanSortKey : materialSpanSortKeys) {
        const MaterialSpan& span = materialSpans[unpack_MaterialSpansIndex(spanSortKey)];
        for (DrawSortKeysIndex position = span.begin; position < span.end; ++position) {
            const DrawDataListIndex addIndex = unpack_DrawDataListIndex(drawSortKeys[position]);
            renderOrderIndexByAddIndex[addIndex] = (DrawDataListIndex)drawDataInRenderOrder.size();
            drawDataInRenderOrder.push_back(std::move(drawDataSet.drawDataList[addIndex]));
        }
    }
    drawDataSet.drawDataList.swap(drawDataInRenderOrder);
}

void RenderListFinalizer::countInstancesPerDraw(const BuildData &buildData, DrawDataSet &drawDataSet) const {
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        drawDataSet.drawDataList[drawIndex].instanceCount = 0;
    }
    for (const DrawInstanceRequest& request : buildData.drawInstanceRequests) {
        ++drawDataSet.drawDataList[renderOrderIndexByAddIndex[request.drawIndex]].instanceCount;
    }
}

void RenderListFinalizer::assignInstanceRanges(DrawDataSet &drawDataSet) {
    nextAllDrawInstancesHolderIndexByDraw.resize(drawDataSet.usedDrawDataCount);
    AllDrawInstancesHolderIndex nextFree = 0;
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        drawDataSet.drawDataList[drawIndex].firstInstance = nextFree;
        nextAllDrawInstancesHolderIndexByDraw[drawIndex] = nextFree;
        nextFree += drawDataSet.drawDataList[drawIndex].instanceCount;
    }
}

void RenderListFinalizer::scatterInstancesIntoDraws(const BuildData &buildData, FinalizedData &finalizedData) {
    //the request order stays inside each draw
    finalizedData.allDrawInstancesHolder.resize(buildData.drawInstanceRequests.size());
    for (const DrawInstanceRequest& request : buildData.drawInstanceRequests) {
        const DrawDataListIndex drawIndex = renderOrderIndexByAddIndex[request.drawIndex];
        finalizedData.allDrawInstancesHolder[nextAllDrawInstancesHolderIndexByDraw[drawIndex]++] = request.instance;
    }
}

void RenderListFinalizer::exitOnDuplicateInstances(const DrawDataSet &drawDataSet, const FinalizedData &finalizedData) {
    std::vector<uint32_t> modelIds;
    for (DrawDataListIndex drawIndex = 0; drawIndex < drawDataSet.usedDrawDataCount; ++drawIndex) {
        modelIds.clear();
        const DrawData& draw = drawDataSet.drawDataList[drawIndex];
        for (AllDrawInstancesHolderIndex instanceIndex = draw.firstInstance; instanceIndex < draw.firstInstance + draw.instanceCount; ++instanceIndex) {
            modelIds.emplace_back(finalizedData.allDrawInstancesHolder[instanceIndex].x);
        }
        std::sort(modelIds.begin(), modelIds.end());
        if (std::adjacent_find(modelIds.begin(), modelIds.end()) != modelIds.end()) {
            std::cerr << "Render list got the same mesh of a model twice in one pass!" << std::endl;
            exit(-1);
        }
    }
}

//ascending order puts the larger, nearer depth first
uint64_t RenderListFinalizer::toNearestFirstSortableDepth(float depth) {
    uint32_t bits;
    std::memcpy(&bits, &depth, sizeof(bits));
    const uint32_t ascending = (bits & 0x80000000u) != 0 ? ~bits : (bits | 0x80000000u);
    return (uint64_t)(~ascending) >> (32 - SORTABLE_DEPTH_BITS);//dropping low bits keeps the order, only very close depths tie
}

RenderListFinalizer::LodBand_MaterialSlot_Depth_DrawDataListIndex RenderListFinalizer::pack_LodBand_MaterialSlot_Depth_DrawDataListIndex(uint32_t lodBand, uint32_t materialSlot, float depth, DrawDataListIndex drawIndex) {
    return ((((((uint64_t)lodBand << RenderListData::MATERIAL_SLOT_BITS) | materialSlot) << SORTABLE_DEPTH_BITS) | toNearestFirstSortableDepth(depth)) << DRAW_INDEX_BITS) | drawIndex;
}

RenderListFinalizer::LodBand_Depth_MaterialSpansIndex RenderListFinalizer::pack_LodBand_Depth_MaterialSpansIndex(LodBand_MaterialSlot_Depth_DrawDataListIndex nearestDrawOfTheSpan, MaterialSpansIndex spanIndex) {
    const uint64_t lodBand = nearestDrawOfTheSpan >> (RenderListData::MATERIAL_SLOT_BITS + SORTABLE_DEPTH_BITS + DRAW_INDEX_BITS);
    const uint64_t sortableDepth = (nearestDrawOfTheSpan >> DRAW_INDEX_BITS) & ((UINT64_C(1) << SORTABLE_DEPTH_BITS) - 1);
    return (((lodBand << SORTABLE_DEPTH_BITS) | sortableDepth) << MATERIAL_SPANS_INDEX_BITS) | spanIndex;
}

uint64_t RenderListFinalizer::unpack_LodBand_MaterialSlot(LodBand_MaterialSlot_Depth_DrawDataListIndex drawSortKey) {
    return drawSortKey >> (SORTABLE_DEPTH_BITS + DRAW_INDEX_BITS);
}

RenderListFinalizer::DrawDataListIndex RenderListFinalizer::unpack_DrawDataListIndex(LodBand_MaterialSlot_Depth_DrawDataListIndex drawSortKey) {
    return (DrawDataListIndex)(drawSortKey & ((UINT64_C(1) << DRAW_INDEX_BITS) - 1));
}

RenderListFinalizer::MaterialSpansIndex RenderListFinalizer::unpack_MaterialSpansIndex(LodBand_Depth_MaterialSpansIndex materialSpanSortKey) {
    return (MaterialSpansIndex)(materialSpanSortKey & ((UINT64_C(1) << MATERIAL_SPANS_INDEX_BITS) - 1));
}

}
