//
// Created by Engin Manap on 06/11/2026.
//

#ifndef LIMONENGINE_RENDERLISTDATA_H
#define LIMONENGINE_RENDERLISTDATA_H

#include <cstdint>
#include <vector>
#include <limits>
#include <memory>
#include <glm/glm.hpp>
#include "../Utils/FlatHashTable.h"
#include "../limonAPI/Graphics/GraphicsInterface.h"//NR_MAX_MATERIALS

class GraphicsProgram;
class MeshAsset;
class Material;
class Model;
class RenderList;

//the insides of RenderList, everything in here is private and only RenderList and RenderListFinalizer are friends
namespace RenderListInternals {

constexpr uint32_t bitsToHold(uint64_t valueCount) {
    uint32_t bits = 0;
    while (bits < 64 && (UINT64_C(1) << bits) < valueCount) {
        ++bits;
    }
    return bits;
}

/**
 * The data of one list, and the code that fills it and draws it. RenderList decides which runs when.
 *
 * What the data looks like in each state:
 * - building (the adds): drawDataList[0, usedDrawDataCount) is in add order, each entry found again through
 *   the lookup table. Instances are not grouped yet, each add only appends a DrawInstanceRequest pointing at its
 *   draw. firstInstance and instanceCount mean nothing yet.
 * - finalized (RenderListFinalizer ran): drawDataList is in render order, and every draw's instances sit
 *   together in allDrawInstancesHolder, in the same order as the draws. The lookup table and the requests are
 *   stale, nothing reads them until clear.
 * - render walks drawDataList front to back and copies each draw's range into batches for the model index UBO.
 */
class RenderListData {
    friend class ::RenderList;
    friend class RenderListFinalizer;

    //one band per LOD level, drawn finest first
    static constexpr uint32_t LOD_BAND_COUNT = 16;

    //field widths of the packed keys, derived from the limits so raising a limit either still fits or stops the build here
    static constexpr uint32_t LOD_BAND_BITS = bitsToHold(LOD_BAND_COUNT);
    static constexpr uint32_t MATERIAL_SLOT_BITS = bitsToHold(NR_MAX_MATERIALS);
    static constexpr uint32_t MESH_RENDER_LIST_ID_BITS = 32;
    static_assert(LOD_BAND_BITS + MATERIAL_SLOT_BITS + MESH_RENDER_LIST_ID_BITS <= 64,
                  "RenderList draw lookup key: MeshAsset::LOD_MAX_LEVEL_COUNT and NR_MAX_MATERIALS no longer fit next to a 32 bit mesh id, lower one of them or widen the key");

    typedef uint32_t DrawDataListIndex;//position in drawDataSet.drawDataList, add order before finalize and render order after
    typedef uint32_t AllDrawInstancesHolderIndex;//position in finalizedData.allDrawInstancesHolder

    //packed into one integer, fields from the highest bits down
    typedef uint64_t LodBand_MaterialSlot_MeshRenderListId;//band | slot in the upper 32 bits, the whole 32 bit mesh id below

    //one per mesh, LOD band and material: one instanced draw
    struct DrawData {
        std::shared_ptr<const Material> material;
        std::shared_ptr<MeshAsset> mesh;
        uint32_t lod = 0;
        float depth = std::numeric_limits<float>::lowest(); //of the nearest instance, larger is nearer. Starting at 0 would tie everything past twice the near plane
        AllDrawInstancesHolderIndex firstInstance = 0;//set by finalize, its instances are allDrawInstancesHolder[firstInstance, firstInstance + instanceCount)
        uint32_t instanceCount = 0;//set by finalize, 0 while building
        bool isAnimated = false;//any instance is animated, the whole draw then runs the animated shader path
        bool needsPose = false;//an instance is animated or hangs from a bone, the rest have nothing for World's pose pass to evaluate
        bool neverSimplified = false;//drawn at lod 0 but sorted after every band, see drawOrderBand
    };

    //one model mesh asking to be drawn. Kept in add order, finalize copies the instances into their draw's range
    struct DrawInstanceRequest {
        DrawDataListIndex drawIndex;//the add order position, finalize moves the draws and translates this with renderOrderIndexByAddIndex
        glm::uvec4 instance;// x = model object Id. y: material index to use. z: bone transform index.
    };

    /**
     * The part of one DrawData's instances that sits in the current batch. render collects these so a batch's
     * instances go to the model index UBO in one write instead of one per draw.
     *
     * A DrawData with more instances than a batch holds becomes several of these, in consecutive batches.
     * Which is unavoidable because the batch size limit is set by driver.
     */
    struct PendingDraw {
        DrawDataListIndex drawIndex;
        uint32_t batchIndicesOffset = 0;//where this piece starts in batchIndices, passed to GPU as poor mans base index
        uint32_t instanceCount = 0;
    };

    //the draws, the only data that lives through every state
    struct DrawDataSet {
        std::vector<DrawData> drawDataList;//writes: addMeshMaterial, RenderListFinalizer (reorders, sets instance ranges), clear  reads: everything
        uint32_t usedDrawDataCount = 0;//entries in use, the vector can be longer so clear doesn't free them. writes: addMeshMaterial, clear  reads: everything
    };

    //only the adds need these, finalize reads the requests once and nothing reads either after that
    struct BuildData {
        FlatHashTable<LodBand_MaterialSlot_MeshRenderListId, DrawDataListIndex> drawDataListIndexBy_LodBand_MaterialSlot_MeshRenderListId;//add order positions. writes: addMeshMaterial, clear  reads: addMeshMaterial
        std::vector<DrawInstanceRequest> drawInstanceRequests;//in add order. writes: addMeshMaterial, clear  reads: RenderListFinalizer
    };

    //empty until finalize fills it
    struct FinalizedData {
        std::vector<glm::uvec4> allDrawInstancesHolder;//render order, contiguous per DrawData. writes: RenderListFinalizer, clear  reads: render, collectModelsNeedingPose
    };

    //render scratch, mutable because render is const. Members so a frame doesn't allocate
    mutable std::vector<glm::uvec4> batchIndices;//instances of the current batch. writes: render, processRenderBatch  reads: processRenderBatch
    mutable std::vector<PendingDraw> pendingDraws;//draws of the current batch. writes: render, processRenderBatch  reads: processRenderBatch

    //clear() keeps every vector's capacity, the next tick fills the same memory
    DrawDataSet drawDataSet;
    BuildData buildData;
    FinalizedData finalizedData;

    static LodBand_MaterialSlot_MeshRenderListId pack_LodBand_MaterialSlot_MeshRenderListId(uint32_t lodBand, uint32_t materialSlot, uint32_t meshRenderListId);

    //the band a draw sorts in: its lod, except a model that never simplified goes after every band so a far one isn't drawn first
    static uint32_t drawOrderBand(uint32_t lod, bool neverSimplified);

    //the steps of addMeshMaterial
    static DrawDataListIndex findOrAppendDrawData(DrawDataSet &drawDataSet, BuildData &buildData, uint32_t lod, bool neverSimplified,
                                                  const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset);
    static void updateDrawData(DrawData &draw, const Model *model, uint32_t lod, float maxDepth);
    static void appendDrawInstanceRequest(BuildData &buildData, DrawDataListIndex drawIndex, const glm::uvec4 &instance);

    void processRenderBatch(GraphicsInterface* graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated,
                    bool &lastAnimationState, std::shared_ptr<const Material> &lastMaterial) const;

    void addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth, int32_t rigIdOverride);
    void collectModelsNeedingPose(std::vector<uint32_t> &modelIds) const;
    void render(GraphicsInterface* graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated) const;
    void clear();
};

}

#endif //LIMONENGINE_RENDERLISTDATA_H
