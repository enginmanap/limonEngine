//
// Created by Engin Manap on 21/12/2024.
//

#ifndef RENDERLIST_H
#define RENDERLIST_H

#include <cstdint>
#include <vector>
#include <array>
#include <limits>
#include <unordered_map>
#include <glm/glm.hpp>
#include <memory>

class GraphicsInterface;
class GraphicsProgram;
class MeshAsset;
class Material;
class Model;

struct PerMeshRenderInformation {
    std::vector<glm::uvec4> indices; // x = model object Id. y: material index to use. z and w reserved for texture id usage + bone transform index.
    uint32_t lod = 0;
    bool isAnimated = false;
    bool needsPose = false;//an instance is animated or hangs from a bone, the rest have nothing for World's pose pass to evaluate
    float depth = std::numeric_limits<float>::lowest(); //nearest instance, larger is nearer. Starting at 0 would tie everything past twice the near plane
};

class RenderList {
public:
    //one band per LOD level, drawn finest first. Inside a band materials and meshes go nearest first
    static constexpr uint32_t LOD_BAND_COUNT = 16;

private:
    typedef std::unordered_map<std::shared_ptr<MeshAsset>, PerMeshRenderInformation> MeshMap;
    typedef std::pair<const std::shared_ptr<MeshAsset>, PerMeshRenderInformation> MeshEntry;

    struct PerMaterialRenderInformation {
        MeshMap meshes;
        float depth = std::numeric_limits<float>::lowest();
        mutable std::vector<const MeshEntry*> meshOrder; //empty means it needs sorting again
    };
    typedef std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation> MaterialMap;
    typedef std::pair<const std::shared_ptr<const Material>, PerMaterialRenderInformation> MaterialEntry;

    struct Band {
        MaterialMap materials;
        mutable std::vector<const MaterialEntry*> materialOrder; //empty means it needs sorting again
    };

public:
    class RenderListIterator {
        const RenderList& renderList;
        uint32_t bandIndex = 0;
        size_t materialIndex = 0;
        size_t meshIndex = 0;
        bool end = false;

        void skipToNextMesh() {
            while (bandIndex < LOD_BAND_COUNT) {
                const std::vector<const MaterialEntry*>& materialOrder = renderList.bands[bandIndex].materialOrder;
                if (materialIndex < materialOrder.size()) {
                    if (meshIndex < materialOrder[materialIndex]->second.meshOrder.size()) {
                        return;
                    }
                    ++materialIndex;
                    meshIndex = 0;
                    continue;
                }
                ++bandIndex;
                materialIndex = 0;
                meshIndex = 0;
            }
            end = true;
        }

        const MaterialEntry* currentMaterial() const {
            return renderList.bands[bandIndex].materialOrder[materialIndex];
        }

    public:
        /**
         * Using this iterator is prone to iterator invalidation. Thats why we don't remove items while any of these are on the fly.
         * The orders are built by RenderList::getIterator, construct it from there.
         */
        explicit RenderListIterator(const RenderList& renderList) : renderList(renderList) {
            skipToNextMesh();
        }

        bool isEnd() const {
            return end;
        }

        RenderListIterator& operator++() {
            if (!end) {
                ++meshIndex;
                skipToNextMesh();
            }
            return *this;
        }

        const PerMeshRenderInformation& get() const {
            return currentMaterial()->second.meshOrder[meshIndex]->second;
        }

        const std::shared_ptr<const Material>& getMaterial() const {
            return currentMaterial()->first;
        }

        const std::shared_ptr<MeshAsset>& getMesh() const {
            return currentMaterial()->second.meshOrder[meshIndex]->first;
        }
    };
private:
    /**
     * This is a batch of renders we have to do. using this, we combine the model indice UBO writes to prevent
     * blocking after each model/mesh.
     *
     * If a model has more instances to render than the batch size, it will have multiple batches (multiple drawcalls)
     * Which is unavoidable because the batch size limit is set by driver.
     */
    struct PendingDraw {
        std::shared_ptr<MeshAsset> mesh;
        std::shared_ptr<const Material> material;
        uint32_t lod = 0;
        uint32_t indexOffset = 0;   //Passed to GPU as poor mans base index
        uint32_t instanceCount = 0;
        bool isAnimated = false;
    };

    mutable std::vector<glm::uvec4> batchIndices;   //scratch memory, class member so we don't allocate every frame.
    mutable std::vector<PendingDraw> pendingDraws;

    std::array<Band, LOD_BAND_COUNT> bands;
    //bands a mesh was ever put in since the last clear. Never cleared bit by bit, a stale bit only costs a lookup, a missing one would draw an object twice
    std::unordered_map<std::shared_ptr<MeshAsset>, uint16_t> meshBandMasks;

    void processRenderBatch(GraphicsInterface* graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated,
                    bool &lastAnimationState, std::shared_ptr<const Material> &lastMaterial) const;

    void removeFromBands(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, uint32_t modelId, uint16_t bandMask);

    void sortBands() const;

    static bool isMaterialNearer(const MaterialEntry* first, const MaterialEntry* second);

    static bool isMeshNearer(const MeshEntry* first, const MeshEntry* second);

public:
    RenderList() = default;
    //the orders point into the maps' nodes, a copy would point into the original. A move keeps the nodes
    RenderList(const RenderList&) = delete;
    RenderList& operator=(const RenderList&) = delete;
    RenderList(RenderList&&) = default;
    RenderList& operator=(RenderList&&) = default;

    void addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth, int32_t rigIdOverride = -1);
    void removeMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, uint32_t modelId);
    void removeModelFromAll(uint32_t modelId);
    RenderListIterator getIterator() const {
        sortBands();
        return RenderListIterator(*this);
    }


   void render(GraphicsInterface* graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram, bool forceNotAnimated = false) const;

    void cleanUpEmptyRenderLists();

    void clear() {
        for (Band& band : bands) {
            band.materials.clear();
            band.materialOrder.clear();
        }
        meshBandMasks.clear();
    }
};



#endif //RENDERLIST_H
