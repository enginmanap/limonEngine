//
// Created by Engin Manap on 06/11/2026.
//

#ifndef LIMONENGINE_RENDERLISTFINALIZER_H
#define LIMONENGINE_RENDERLISTFINALIZER_H

#include "RenderListData.h"

namespace RenderListInternals {

/**
 * finalize() of one list, turns the add order RenderListData into the render order one.
 * Render order is by LOD band, inside a band materials nearest first, inside a material meshes nearest first.
 * Nothing is sorted while adding, all of it happens here once per tick.
 *
 * Steps 1-4 put the draws in order. Sorting only moves 8 byte keys, the DrawData entries move once in step 4:
 * 1. sortDrawsByBandMaterialDepth(): one drawSortKey per draw (band | material slot | depth | add index), sorted.
 *    Draws of one band and material now sit together, nearest first, but the materials are in slot order
 * 2. cutIntoMaterialSpans(): every run of keys sharing band and material becomes a MaterialSpan, with one
 *    materialSpanSortKey (band | depth of its first, so nearest, draw | span index)
 * 3. sortSpansByNearestDraw(): sorts those, so inside a band the material with the nearest draw goes first
 * 4. moveDrawsIntoRenderOrder(): span by span, the draws move into drawDataInRenderOrder, which is then swapped
 *    with drawDataList. The requests still point at add order positions, renderOrderIndexByAddIndex maps them
 *
 * Steps 5-7 lay the instances out behind each other in that order, a counting sort of the requests by draw:
 * 5. countInstancesPerDraw(): instanceCount of every draw
 * 6. assignInstanceRanges(): firstInstance of every draw, each range right behind the previous draw's
 * 7. scatterInstancesIntoDraws(): every request's instance into the next free slot of its draw's range, so inside
 *    a draw the instances keep their add order
 * Debug builds then exit if a draw got the same model twice.
 *
 * The members are only scratch, kept to reuse the memory between ticks.
 */
class RenderListFinalizer {
    friend class ::RenderList;

    typedef RenderListData::DrawData DrawData;
    typedef RenderListData::DrawInstanceRequest DrawInstanceRequest;
    typedef RenderListData::DrawDataListIndex DrawDataListIndex;
    typedef RenderListData::AllDrawInstancesHolderIndex AllDrawInstancesHolderIndex;
    typedef RenderListData::DrawDataSet DrawDataSet;
    typedef RenderListData::BuildData BuildData;
    typedef RenderListData::FinalizedData FinalizedData;

    typedef uint32_t DrawSortKeysIndex;//position in drawSortKeys
    typedef uint32_t MaterialSpansIndex;//position in materialSpans

    //packed into one integer, fields from the highest bits down
    typedef uint64_t LodBand_MaterialSlot_Depth_DrawDataListIndex;//band | slot | 24 bit depth | the rest, at least 27 bits. The index is the add order one
    typedef uint64_t LodBand_Depth_MaterialSpansIndex;//band | 24 bit depth | the rest

    static constexpr uint32_t SORTABLE_DEPTH_BITS = 24;//a box's average depth only orders draws roughly, 24 bits still tells 100 m from 110 m
    static constexpr uint32_t DRAW_INDEX_BITS = 64 - RenderListData::LOD_BAND_BITS - RenderListData::MATERIAL_SLOT_BITS - SORTABLE_DEPTH_BITS;//whatever is left
    static constexpr uint32_t MATERIAL_SPANS_INDEX_BITS = 64 - RenderListData::LOD_BAND_BITS - SORTABLE_DEPTH_BITS;
    static_assert(DRAW_INDEX_BITS >= 27,
                  "RenderList draw sort key: MeshAsset::LOD_MAX_LEVEL_COUNT and NR_MAX_MATERIALS leave less than 27 bits for the draw index, lower one of them or widen the key");
    static_assert(MATERIAL_SPANS_INDEX_BITS >= DRAW_INDEX_BITS, "RenderList material span sort key: a span index must fit wherever a draw index does");

    //drawSortKeys[begin, end) share LOD band and material, so render binds the material once for all of them
    struct MaterialSpan {
        DrawSortKeysIndex begin;
        DrawSortKeysIndex end;
    };

    std::vector<LodBand_MaterialSlot_Depth_DrawDataListIndex> drawSortKeys;//step 1. writes: sortDrawsByBandMaterialDepth  reads: cutIntoMaterialSpans, moveDrawsIntoRenderOrder
    std::vector<MaterialSpan> materialSpans;//step 2, in drawSortKeys order. writes: cutIntoMaterialSpans  reads: moveDrawsIntoRenderOrder
    std::vector<LodBand_Depth_MaterialSpansIndex> materialSpanSortKeys;//steps 2-3, one per MaterialSpan. writes: cutIntoMaterialSpans, sortSpansByNearestDraw  reads: moveDrawsIntoRenderOrder
    std::vector<DrawData> drawDataInRenderOrder;//step 4, after the swap it holds the previous tick's moved out entries, only kept for the memory. writes: moveDrawsIntoRenderOrder
    std::vector<DrawDataListIndex> renderOrderIndexByAddIndex;//step 4, indexed by DrawInstanceRequest::drawIndex. writes: moveDrawsIntoRenderOrder  reads: countInstancesPerDraw, scatterInstancesIntoDraws
    std::vector<AllDrawInstancesHolderIndex> nextAllDrawInstancesHolderIndexByDraw;//steps 6-7, the next free slot of each draw's range. writes: assignInstanceRanges, scatterInstancesIntoDraws  reads: scatterInstancesIntoDraws

    static uint64_t toNearestFirstSortableDepth(float depth);
    static LodBand_MaterialSlot_Depth_DrawDataListIndex pack_LodBand_MaterialSlot_Depth_DrawDataListIndex(uint32_t lodBand, uint32_t materialSlot, float depth, DrawDataListIndex drawIndex);
    static LodBand_Depth_MaterialSpansIndex pack_LodBand_Depth_MaterialSpansIndex(LodBand_MaterialSlot_Depth_DrawDataListIndex nearestDrawOfTheSpan, MaterialSpansIndex spanIndex);
    static uint64_t unpack_LodBand_MaterialSlot(LodBand_MaterialSlot_Depth_DrawDataListIndex drawSortKey);
    static DrawDataListIndex unpack_DrawDataListIndex(LodBand_MaterialSlot_Depth_DrawDataListIndex drawSortKey);
    static MaterialSpansIndex unpack_MaterialSpansIndex(LodBand_Depth_MaterialSpansIndex materialSpanSortKey);

    //steps 1-4
    void sortDrawsByBandMaterialDepth(const DrawDataSet &drawDataSet);
    void cutIntoMaterialSpans();
    void sortSpansByNearestDraw();
    void moveDrawsIntoRenderOrder(DrawDataSet &drawDataSet);

    //steps 5-7
    void countInstancesPerDraw(const BuildData &buildData, DrawDataSet &drawDataSet) const;
    void assignInstanceRanges(DrawDataSet &drawDataSet);
    void scatterInstancesIntoDraws(const BuildData &buildData, FinalizedData &finalizedData);
    static void exitOnDuplicateInstances(const DrawDataSet &drawDataSet, const FinalizedData &finalizedData);

    void finalize(RenderListData &renderListData);
};

}

#endif //LIMONENGINE_RENDERLISTFINALIZER_H
