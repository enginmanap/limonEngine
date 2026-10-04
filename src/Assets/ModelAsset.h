//
// Created by engin on 31.08.2016.
//

#ifndef LIMONENGINE_MODELASSET_H
#define LIMONENGINE_MODELASSET_H


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <cstdint>
#include <map>
#include <unordered_map>
#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#include "../Utils/GLMCerealConverters.hpp"
#endif

#include "../Utils/AssimpUtils.h"
#include "../Material.h"
#include "Asset.h"
#include "Lod/LodLadder.h"
#include "MeshAsset.h"
#include "../Utils/GLMConverter.h"
#include "BoneNode.h"
#include "Animations/AnimationInterface.h"

class AnimationAssimp;

class ModelAsset : public Asset {
private:
    void loadCPUPart() override;
    void loadGPUPart() override;

    struct BoneInformation {
        glm::mat4 offset;
        glm::mat4 parentOffset;
        glm::mat4 globalMeshInverse;

        template<class Archive>
        void serialize( Archive & ar ) {
            ar( offset, parentOffset, globalMeshInverse);
        }
    };

    struct AnimationSection {
        std::string baseAnimationName;
        std::string animationName;
        float startTime;
        float endTime;

        AnimationSection(const std::string &sourceAnimationName, const std::string &animationName,
                         float startTime, float endTime) : baseAnimationName(
                sourceAnimationName), animationName(animationName), startTime(startTime), endTime(endTime) {}
        //cereal uses
        AnimationSection() : startTime(0), endTime(0){}
        template<class Archive>
        void serialize( Archive & ar ) {
            ar( baseAnimationName, animationName, startTime, endTime);
        }
    };

    std::string name;
    std::string sourcePath;//what embedded textures are keyed by, a limonmodel keeps its source's so the stored materials still find them
    bool flipX = false, flipY = false, flipZ = false;
    bool reverseWinding = false;

    std::map<std::string, std::shared_ptr<AnimationInterface>> animations;//shared for animation sections
    std::shared_ptr<BoneNode> rootNode = nullptr;//bones are shared with meshes
    int_fast32_t boneIDCounter, boneIDCounterPerMesh;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::unordered_map<std::string, std::shared_ptr<Material>> materialMap;//shared with model
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::map<const std::shared_ptr<const MeshAsset>,std::shared_ptr<Material>> meshMaterialMap;
    std::vector<AnimationSection> animationSections;

    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> simplifiedMeshes;//physics
    std::unordered_map<std::string, BoneInformation> boneInformationMap;

    std::vector<btCompoundShape *> shapeCopies;
    btCompoundShape *compoundShapeForConvex; // used for non zero mass objects or animated objects
    std::map<uint32_t, btTransform> bulletTransformMap;
    std::map<uint32_t, btConvexHullShape *> bulletHullMap;
    btTransform baseTransform;
    std::map<uint32_t, uint32_t> boneIdCompoundChildMap;
    std::vector<btBvhTriangleMeshShape *>meshCollisionShapesForTriangle;
    std::vector<btCollisionShape *> reusableMeshes;
    bool hasAnimation;
    LodLadder lodLadder;//this model LOD steps: what was asked, what came out, where each one is used
    bool customizationAfterSave = false;

    bool transparentMaterialUsed = false;

    std::unique_ptr<std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>>> temporaryEmbeddedTextures = nullptr;//this is set on cerealLoad

    std::shared_ptr<Material> loadMaterials(const aiScene *scene, unsigned int materialIndex);

    void createMeshes(const aiScene *scene, aiNode *aiNode, glm::mat4 parentTransform);//parent transform is not reference on purpose
    //if it was, then we would need a stack

    std::shared_ptr<BoneNode> loadNodeTree(aiNode *aiNode);

    bool findNode(const std::string &nodeName, std::shared_ptr<BoneNode>& foundNode, std::shared_ptr<BoneNode> searchRoot) const;

    void traverseAndSetTransform(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform, std::shared_ptr<const AnimationInterface> animation,
                                 float timeInTicks,
                                 std::vector<glm::mat4> &transforms) const;

    /**
    Used only by editor preview of bones/animations

    intentionally separate from the main transform calculation, as it is used in runtime

    Sets the matrices for joints, so editor can render them
    */
    void traverseAndSetJointTransform(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                      std::shared_ptr<const AnimationInterface> animation, float timeInTicks,
                                      std::vector<glm::mat4> &outJointTransforms) const;
    /**
    Used only by editor preview of bones/animations

    Same as above, but for no-animation/t-pose case
    */
    void traverseAndSetBindPoseJointTransform(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                              std::vector<glm::mat4> &outJointTransforms) const;

    void traverseAndSetTransformBlended(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                        std::shared_ptr<const AnimationInterface> animationOld,
                                        float timeInTicksOld,
                                        std::shared_ptr<const AnimationInterface> animationNew,
                                        float timeInTicksNew,
                                        float blendFactor,
                                        std::vector<glm::mat4> &transforms) const;

    const aiNodeAnim *findNodeAnimation(aiAnimation *pAnimation, std::string basic_string) const;

    void deserializeCustomizations();
    void computeBoundsFromVertices();
    void buildLodLevels(LodLadder::BuildMode buildMode);//measures or reuses, then hands each mesh its plan
    void buildLodGeometry(std::vector<LodLadder::MeshGeometry> &outGeometry) const;
    std::string getFlipAxes() const;

    int32_t buildEditorBoneTreeRecursive(std::shared_ptr<BoneNode> boneNode, int32_t selectedBoneNodeID, bool followSelection);

    void collectBoneHierarchyEdgesRecursive(const std::shared_ptr<BoneNode> &boneNode, std::vector<std::pair<uint32_t, uint32_t>> &edges) const;

    int32_t findBoneIDByNameRecursive(const std::shared_ptr<BoneNode> &boneNode, const std::string &boneName) const;

#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    friend class AssetManager;
    /**
     * This is used by cereal for deserialize
     */
    ModelAsset() : Asset(nullptr, 0, std::vector<std::string>()) {};

public:
    void bakeAllOccluderLods();//the limonmodel export calls this, load only bakes the level the option asks for

    //the steps, where they are used, and everything the editor edits about them
    const LodLadder &getLodLadder() const {
        return lodLadder;
    }

    //editing a step is two steps of its own: say what is wanted here, then regenerateLods to act on it
    LodLadder &getLodLadder() {
        return lodLadder;
    }

    //the old ranges are gone the moment this returns, so it may only run between frames, never while a render
    //list is holding a level index. The Editor drains a queue for it
    void regenerateLods(LodLadder::BuildMode buildMode);

    bool hasUnsavedChanges() const {
        return customizationAfterSave || lodLadder.hasUnsavedChanges();
    }

    //a source asset writes its sidecar, a converted one rewrites its limonmodel. Bakes every occluder level for
    //the latter, so only between frames. Unsaved flags are cleared only for what was actually written
    bool saveChanges();

    //bakes every occluder level first, so only between frames
    bool writeBinary(const std::string &path);

    //true for an asset loaded from a limonmodel, where the file itself is the store rather than a sidecar
    bool isConvertedAsset() const;
    static std::string stripFlipSuffix(const std::string &path, bool &outFlipX, bool &outFlipY, bool &outFlipZ);

    ModelAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);

    //one mesh of an imported scene, for the map converter. Never in the asset cache and never sent to the GPU, it
    //only exists to be written out. The caller must have added the scene's embedded textures under sourcePath
    ModelAsset(AssetManager *assetManager, const aiScene *scene, uint32_t meshIndex, bool mirrorX,
               const std::string &sourcePath, const std::string &piecePath, const std::string &meshName);

    //nullptr on failure, the scene lives as long as the importer
    static const aiScene *importScene(Assimp::Importer &assimpImporter, const std::string &path);
    static std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> readEmbeddedTextures(const aiScene *scene);
#ifdef CEREAL_SUPPORT
    ModelAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList, cereal::BinaryInputArchive& binaryArchive) :
            Asset(assetManager, assetID, fileList, binaryArchive) {
        binaryArchive(*this);
        this->assetManager = assetManager;
        name = fileList[0];//the stored name is the source's, keeping it would make this look like a source asset
        customizationAfterSave = false;//older files stored it set, but the sections came from this file
        //the same path a source asset takes. The steps arrived with the file, so there is nothing to measure and
        //every mesh keeps the ranges it was deserialized with
        buildLodLevels(LodLadder::BuildMode::NORMAL);
    }
#endif
    bool addAnimationAsSubSequence(const std::string &baseAnimationName, const std::string newAnimationName,
                                   float startTime, float endTime);

    bool isAnimated() const;

    /**
     * This method is used to request a specific animations transform array for a specific time. If looped is false,
     * it will return if the given time was after or equals final frame. It interpolates by time automatically.
     *
     * @param time Requested animation time in miliseconds.
     * @param looped if animation should loop or not. Effects return.
     * @param animationName name of animation to seek.
     * @param transformMatrix transform matrix list for bones
     *
     * @return if last frame of animation is played for not looped animation. Always true for looped ones.
     */
    bool getTransform(float time, bool looped, std::string animationName, std::vector<glm::mat4> &transformMatrix) const; //this method takes vector to avoid copying it

    // only returns if animation finished, without calculating animation itself. Used to skip getTransform which does calculate all bone transforms.
    // Why do we wanna skip it? because object might be out of view, so we might wanna only activate physics on object, if it is still animating
    bool isAnimationFinished(const std::string &animationName, float time, bool looped) const;

    bool getTransformBlended(std::string animationName1, float time1, bool looped1,
                                         std::string animationName2, float time2, bool looped2,
                                         float blendFactor, std::vector<glm::mat4> &transformMatrixVector) const;

    /**
     * Editor Only
     *
     * Computes each bone's own current joint transform (parent-chain accumulated, not the skinning matrix
     * getTransform produces -- see traverseAndSetJointTransform). Used by the bone-exposure
     * preview to visualize joint positions, entirely separate from the gameplay animation path.
     *
     * @param time Requested animation time in milliseconds. Ignored if animationName is empty (bind pose).
     * @param looped if animation should loop or not.
     * @param animationName name of animation to seek, or empty for bind pose.
     * @param outJointTransforms transform list for bones, pre-sized by the caller.
     */
    void getJointTransforms(float time, bool looped, const std::string &animationName, std::vector<glm::mat4> &outJointTransforms) const;

    const glm::vec3 &getBoundingBoxMin() const { return boundingBoxMin; }

    const glm::vec3 &getBoundingBoxMax() const { return boundingBoxMax; }

    const glm::vec3 &getCenterOffset() const { return centerOffset; }

    /*
     * FIXME: the materials should be const too
     */
    const std::unordered_map<std::string, std::shared_ptr<Material>> &getMaterialMap() const { return materialMap; };

    std::shared_ptr<Material> getMeshMaterial(const std::shared_ptr<MeshAsset> &mesh) const {
        auto iter = meshMaterialMap.find(mesh);
        if (iter == meshMaterialMap.end()) {
            return nullptr;
        }
        return meshMaterialMap.at(mesh);
    }

    ~ModelAsset() override;

    std::vector<std::shared_ptr<MeshAsset>> getMeshes() const {
        return meshes;
    }

    /**
     * This method checks if there is a simplified mesh with same name, and there is, adds simplified one instead of original.
     * @return hybrid of original and simplified meshes
     */
    std::vector<std::shared_ptr<MeshAsset>> getPhysicsMeshes() const {
        if(simplifiedMeshes.size() == 0) {
            return meshes;
        }

        std::vector<std::shared_ptr<MeshAsset>> meshAssets = meshes;//shallow copy
        for (unsigned int i = 0; i < meshes.size(); ++i) {
            std::string meshName = "UCX_" + meshes[i]->getName();
            if(simplifiedMeshes.find(meshName) != simplifiedMeshes.end()) {
                meshAssets[i] = simplifiedMeshes.at(meshName);
            }
        }
        return meshAssets;
    }
    btCompoundShape * getCompoundShapeForMass(uint32_t mass, std::map<uint32_t, uint32_t> &boneIdCompoundChildMap, std::vector<btCollisionShape *>& childrenShapes);

    void fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations, const std::string &animationNamePrefix = "");

    const std::map<std::string, std::shared_ptr<AnimationInterface>> &getAnimations() const {
        return animations;
    }

    bool serializeCustomizations();

    int32_t buildEditorBoneTree(int32_t selectedBoneNodeID, bool followSelection);

    /**
    This might be cached, but since it is used for rendering, and we already limit bone count to 128, there is
    virtually no chance this will become a measurable time spent.
    */
    //-1 if this model has no bone with that name, the names are the ones the editor lists
    int32_t getBoneIDByName(const std::string &boneName) const {
        return findBoneIDByNameRecursive(rootNode, boneName);
    }

    std::vector<std::pair<uint32_t, uint32_t>> getBoneHierarchyEdges() const {
        std::vector<std::pair<uint32_t, uint32_t>> edges;
        collectBoneHierarchyEdgesRecursive(rootNode, edges);
        return edges;
    }
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void save( Archive & ar ) const {
        std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> textures;
        size_t index = 0;
        std::shared_ptr<const AssetManager::EmbeddedTexture> embeddedTexture = assetManager->getEmbeddedTextures(sourcePath, index);
        while(embeddedTexture != nullptr) {
            textures.push_back(embeddedTexture);
            index++;
            embeddedTexture = assetManager->getEmbeddedTextures(sourcePath, index);
        }
        ar(sourcePath, boneIDCounter, boneIDCounterPerMesh, textures,                   hasAnimation, rootNode, boundingBoxMax, boundingBoxMin, centerOffset, boneInformationMap, simplifiedMeshes, meshes, animations, animationSections, customizationAfterSave, materialMap, meshMaterialMap, transparentMaterialUsed, lodLadder);
    }

    template<class Archive>
    void load( Archive & ar ) {
        std::map<std::shared_ptr<MeshAsset>,std::shared_ptr<Material>> tempMeshMaterialMap;
        temporaryEmbeddedTextures = std::make_unique<std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>>>();
        ar(sourcePath, boneIDCounter, boneIDCounterPerMesh, *temporaryEmbeddedTextures, hasAnimation, rootNode, boundingBoxMax, boundingBoxMin, centerOffset, boneInformationMap, simplifiedMeshes, meshes, animations, animationSections, customizationAfterSave, materialMap, tempMeshMaterialMap, transparentMaterialUsed, lodLadder);
        //now update embedded textures to assetManager
        for (size_t i = 0; i < meshes.size(); ++i) {
            meshes[i]->buildBulletMesh();
        }
        for (auto& simpleMesh:simplifiedMeshes) {
            simpleMesh.second->buildBulletMesh();
        }

        for (const auto& tempMeshMaterialPair:tempMeshMaterialMap) {
            meshMaterialMap[tempMeshMaterialPair.first] = tempMeshMaterialPair.second;
        }

        for (auto& materialPair:materialMap) {

            std::vector<std::map<const std::shared_ptr<const MeshAsset>,std::shared_ptr<Material>>::iterator> meshMaterialToUpdateItList;
            for (std::map<const std::shared_ptr<const MeshAsset>,std::shared_ptr<Material>>::iterator it = meshMaterialMap.begin(); it != meshMaterialMap.end(); ++it) {
                if(it->second == materialPair.second) {
                    meshMaterialToUpdateItList.emplace_back(it);
                }
            }
            //asset's own materials, not overrides, so register like the fresh assimp path does. As
            //overrides they would each install a forwarding rule and replace their own base everywhere
            materialPair.second = assetManager->getMaterialRegistry().registerMaterial(materialPair.second);
            for (auto& meshMaterialToUpdateIt: meshMaterialToUpdateItList) {
                meshMaterialMap[meshMaterialToUpdateIt->first] = materialPair.second;
            }
        }
        buildPhysicsMeshes();
    }
#endif

    void buildPhysicsMeshes();
};


#endif //LIMONENGINE_MODELASSET_H
