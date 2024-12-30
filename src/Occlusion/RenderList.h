//
// Created by Engin Manap on 21/12/2024.
//

#ifndef RENDERLIST_H
#define RENDERLIST_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>
#include <glm/glm.hpp>
#include <memory>

class GraphicsInterface;
class GraphicsProgram;
class MeshAsset;
class Material;
class Model;

struct PerMeshRenderInformation {
    std::vector<glm::uvec4> indices; // x = model object Id. y: material index to use. z and w reserved for texture id usage + bone transform index.
    uint32_t lod = 0;  //Max level of detail. Since we use instanced rendering, using single LOD for all meshes is faster than multiple draw calls (at least in my testing)
    bool isAnimated = false;
    float depth = 0; //max depth for this mesh set
};

class RenderList {
    struct PerMaterialRenderInformation {
        class perMaterialIterator {
            const PerMaterialRenderInformation& perMaterialRenderInformation;
            std::multimap<float, std::shared_ptr<MeshAsset>>::const_reverse_iterator it;
            bool end;
        public:
            explicit perMaterialIterator(const PerMaterialRenderInformation& perMaterialRenderInformation) : perMaterialRenderInformation(perMaterialRenderInformation), end(false) {
                if (perMaterialRenderInformation.meshRenderPriorityMap.empty()) {
                    for (auto &materialEntry : perMaterialRenderInformation.maxDepthPerMesh) {
                        perMaterialRenderInformation.meshRenderPriorityMap.insert(std::make_pair(materialEntry.second, materialEntry.first));
                    }
                }
                it = perMaterialRenderInformation.meshRenderPriorityMap.rbegin();
                if (it == perMaterialRenderInformation.meshRenderPriorityMap.rend()) {
                    end = true;
                }
            }

            bool isEnd() const {
                return end;
            }

            perMaterialIterator& operator++() {
                if (!end) {
                    ++it;
                    end = (it == perMaterialRenderInformation.meshRenderPriorityMap.rend());
                }
                return *this;
            }
            const PerMeshRenderInformation& get() const {
                return perMaterialRenderInformation.meshesToRender.at(it->second);
            }
            const std::shared_ptr<MeshAsset>& getMesh() const {
                return it->second;
            }
        };
        std::unordered_map<std::shared_ptr<MeshAsset>,PerMeshRenderInformation> meshesToRender;
        std::unordered_map<std::shared_ptr<MeshAsset>, float> maxDepthPerMesh; //this map is created same time as meshes to render, but it is a pre transform container, as ordering by value not possible(or logical) in maps.
        mutable std::multimap<float, std::shared_ptr<MeshAsset>> meshRenderPriorityMap; //this map is created after first two values are created. It is just a sorted container to use sorting of materials
        std::unordered_map<std::shared_ptr<MeshAsset>, PerMeshRenderInformation>::iterator getOrCreateMeshEntry(const std::shared_ptr<MeshAsset> &meshAsset) {
            std::unordered_map<std::shared_ptr<MeshAsset>, PerMeshRenderInformation>::iterator it = meshesToRender.find(meshAsset);
            if (it == meshesToRender.end()) {
                meshesToRender[meshAsset] = PerMeshRenderInformation();
                it = meshesToRender.find(meshAsset);
                maxDepthPerMesh[meshAsset] = 0.0f;
            }
            return it;
        }
    };

public:
    class RenderListIterator {
        const RenderList& renderList;
        std::multimap<float, std::shared_ptr<const Material>>::reverse_iterator materialPriorityIt;
        mutable PerMaterialRenderInformation::perMaterialIterator* perMaterialIterator = nullptr;
        bool end;
        bool materialChanged = true;

    public:
        /**
         * Provides an iterator that automatically iterates through materials and meshes, using depth information as ordering.
         * Using this iterator is prone to iterator invalidation. Thats why we don't remove items while any of these are on the fly.
         */
        explicit RenderListIterator(const RenderList& renderList) : renderList(renderList), materialPriorityIt(renderList.materialRenderPriorityMap.rbegin())
                                                              , end(false) {
            if (materialPriorityIt == renderList.materialRenderPriorityMap.rend()) {
                end = true;
            } else {
                perMaterialIterator = new PerMaterialRenderInformation::perMaterialIterator(renderList.perMaterialMeshMap.at(materialPriorityIt->second));
            }
        }
        ~RenderListIterator() {
            delete perMaterialIterator;
        }

        bool isEnd() const {
            return end;
        }

        RenderListIterator& operator++() {
            if (end) {
                return *this;
            }
            if (!perMaterialIterator->isEnd()) {
                ++(*perMaterialIterator);
                materialChanged = false;
            }
            if (perMaterialIterator->isEnd()) {
                //so this material is done. move to next material
                ++materialPriorityIt;
                end = (materialPriorityIt == renderList.materialRenderPriorityMap.rend());
                if (end) {
                    return *this;
                }
                delete perMaterialIterator;
                perMaterialIterator = new PerMaterialRenderInformation::perMaterialIterator(renderList.perMaterialMeshMap.at(materialPriorityIt->second));
                materialChanged = true;
            }
            return *this;
        }
        const PerMeshRenderInformation& get() const {
            return perMaterialIterator->get();
        }

        const std::shared_ptr<const Material>& getMaterial() const {
            return materialPriorityIt->second;
        }

        const std::shared_ptr<MeshAsset>& getMesh() const {
            return perMaterialIterator->getMesh();
        }

        bool isMaterialChanged() const {
            return materialChanged;
        }
    };
private:
    std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation> perMaterialMeshMap; //Each mesh has its own render information
    std::unordered_map<std::shared_ptr<const Material>, float> maxDepthPerMaterial; //this map is created same time as meshes to render, but it is a pre transform container, as ordering by value not possible(or logical) in maps.
    mutable std::multimap<float, std::shared_ptr<const Material>> materialRenderPriorityMap; //this map is created after first two values are created. It is just a sorted container to use sorting of materials

    std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation>::iterator getOrCreateMaterialEntry(const std::shared_ptr<const Material> &material) {
        std::unordered_map<std::shared_ptr<const Material>, PerMaterialRenderInformation>::iterator it = perMaterialMeshMap.find(material);
        if (it == perMaterialMeshMap.end()) {
            perMaterialMeshMap[material] = PerMaterialRenderInformation();
            it = perMaterialMeshMap.find(material);
            maxDepthPerMaterial[material] = 0.0f;
        }
        return it;
    }
public:
    void addMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, const Model *model, uint32_t lod, float maxDepth);
    void removeMeshMaterial(const std::shared_ptr<const Material> &material, const std::shared_ptr<MeshAsset> &meshAsset, uint32_t modelId);
    void removeModelFromAll(uint32_t modelId);
    RenderListIterator getIterator() const {
        if (materialRenderPriorityMap.empty()) {
            //first fill the map
            for (auto &materialEntry : maxDepthPerMaterial) {
                materialRenderPriorityMap.insert(std::make_pair(materialEntry.second, materialEntry.first));
            }
        }
        return RenderListIterator(*this);
    }


   void render(GraphicsInterface* graphicsWrapper, const std::shared_ptr<GraphicsProgram> &renderProgram) const;

    void cleanUpEmptyRenderLists();

    void clear() {
        this->materialRenderPriorityMap.clear();
        this->perMaterialMeshMap.clear();
        this->maxDepthPerMaterial.clear();
    }
};



#endif //RENDERLIST_H
