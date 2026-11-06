//
// Created by Engin Manap on 21/12/2024.
//

#ifndef RENDERLIST_H
#define RENDERLIST_H

#ifdef RENDER_LIST_BAND_MAP
#include "BandMapRenderList.h"
#else
#include "RenderListData.h"
#include "RenderListFinalizer.h"
#endif
#include "../Utils/ForceInline.h"

/**
 * The models one camera draws for one tag combination as instanced draws, or a single model for
 * Model::convertSingleModelToRenderList. A camera's lists are rebuilt from scratch every tick and drawn again as
 * they are on frames without a tick. One tick of a list:
 * 1. clear()
 * 2. addMeshMaterial() per visible model mesh: finds or appends the DrawData for its LOD band, material and mesh,
 *    and appends a DrawInstanceRequest for the model. Nothing is sorted or grouped yet
 * 3. finalize(): puts drawDataList in render order and lays the instances out behind it, its steps are on top of
 *    RenderListFinalizer
 * 4. render(), any number of times: drawDataList front to back, each DrawData is one instanced draw of its range
 *    of allDrawInstancesHolder. collectModelsNeedingPose reads the same finalized data
 * A finalized list is never changed, the next change starts again from clear(). An out of order call exits.
 *
 * The data and the code are in RenderListInternals: RenderListData holds the list and what reads it in each
 * state, RenderListFinalizer step 3. This class only checks the order of the calls. Every call here must stay
 * in the header and force inlined, or it adds a call per add.
 * The RENDER_LIST_BAND_MAP cmake option swaps both for the old BandMapRenderList, to compare the two.
 */
class RenderList {
    //clear leaves a list empty, which renders as nothing and takes adds. finalize closes it, an add after that exits
    enum class BuildState { EMPTY, BUILDING, FINALIZED };

#ifdef RENDER_LIST_BAND_MAP
    RenderListInternals::BandMapRenderList bandMap;
#else
    RenderListInternals::RenderListData data;
    RenderListInternals::RenderListFinalizer finalizer;
#endif
    BuildState buildState = BuildState::EMPTY;

    //out of line so the error printing isn't copied into every caller's add loop, and the header needs no iostream
    [[noreturn]] static void exitOnWrongBuildState(const char *operation, const char *problem);

    LIMON_FORCE_INLINE void exitIfFinalized(const char *operation) const {
        if (buildState == BuildState::FINALIZED) {
            exitOnWrongBuildState(operation, " after finalize, a finalized list only changes through clear!");
        }
    }

    LIMON_FORCE_INLINE void exitIfBuilding(const char *operation) const {
        if (buildState == BuildState::BUILDING) {
            exitOnWrongBuildState(operation, " before finalize!");
        }
    }

public:
    LIMON_FORCE_INLINE void addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth, int32_t rigIdOverride = -1) {
        exitIfFinalized("got an add");
#ifdef RENDER_LIST_BAND_MAP
        bandMap.addMeshMaterial(material, meshAsset, model, lod, maxDepth, rigIdOverride);
#else
        data.addMeshMaterial(material, meshAsset, model, lod, maxDepth, rigIdOverride);
#endif
        buildState = BuildState::BUILDING;
    }

    LIMON_FORCE_INLINE void finalize() {
        exitIfFinalized("finalized twice");
#ifndef RENDER_LIST_BAND_MAP
        finalizer.finalize(data);
#endif
        buildState = BuildState::FINALIZED;
    }

    //appends the model of every instance whose draw needs a pose, a model with several meshes comes more than once
    LIMON_FORCE_INLINE void collectModelsNeedingPose(std::vector<uint32_t> &modelIds) const {
        exitIfBuilding("read for poses");
#ifdef RENDER_LIST_BAND_MAP
        bandMap.collectModelsNeedingPose(modelIds);
#else
        data.collectModelsNeedingPose(modelIds);
#endif
    }

    LIMON_FORCE_INLINE void render(GraphicsInterface* graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated = false) const {
        exitIfBuilding("rendered");
#ifdef RENDER_LIST_BAND_MAP
        bandMap.render(graphicsWrapper, renderProgram, forceNotAnimated);
#else
        data.render(graphicsWrapper, renderProgram, forceNotAnimated);
#endif
    }

    LIMON_FORCE_INLINE void clear() {
#ifdef RENDER_LIST_BAND_MAP
        bandMap.clear();
#else
        data.clear();
#endif
        buildState = BuildState::EMPTY;
    }
};

#endif //RENDERLIST_H
