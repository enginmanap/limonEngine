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
#include <unordered_map>
#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#include "../Utils/GLMCerealConverters.hpp"
#endif

#include "../Utils/AssimpUtils.h"
#include "../Material.h"
#include "Asset.h"
#include "MeshAsset.h"
#include "../Utils/GLMConverter.h"
#include "BoneNode.h"
#include "Animations/AnimationInterface.h"

class AnimationAssimp;

class ModelAsset : public Asset {

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
    std::unordered_map<std::string, std::shared_ptr<AnimationInterface>> animations;//shared for animation sections
    std::shared_ptr<BoneNode> rootNode = nullptr;//bones are shared with meshes
    int_fast32_t boneIDCounter, boneIDCounterPerMesh;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::unordered_map<std::string, std::shared_ptr<Material>> materialMap;//shared with model
    std::vector<btConvexShape *> shapeCopies;
    std::vector<std::shared_ptr<MeshAsset>> meshes;
    std::vector<AnimationSection> animationSections;

    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> simplifiedMeshes;//physics
    std::unordered_map<std::string, BoneInformation> boneInformationMap;

    bool hasAnimation;
    bool customizationAfterSave = false;

    std::unique_ptr<std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>>> temporaryEmbeddedTextures = nullptr;//this is set on cerealLoad

    std::shared_ptr<Material> loadMaterials(const aiScene *scene, unsigned int materialIndex);

    void createMeshes(const aiScene *scene, aiNode *aiNode, glm::mat4 parentTransform);//parent transform is not reference on purpose
    //if it was, then we would need a stack

    std::shared_ptr<BoneNode> loadNodeTree(aiNode *aiNode);

    bool findNode(const std::string &nodeName, std::shared_ptr<BoneNode>& foundNode, std::shared_ptr<BoneNode> searchRoot) const;

    void traverseAndSetTransform(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform, std::shared_ptr<const AnimationInterface> animation,
                                 float timeInTicks,
                                 std::vector<glm::mat4> &transforms) const;

    void traverseAndSetTransformBlended(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                        std::shared_ptr<const AnimationInterface> animationOld,
                                        float timeInTicksOld,
                                        std::shared_ptr<const AnimationInterface> animationNew,
                                        float timeInTicksNew,
                                        float blendFactor,
                                        std::vector<glm::mat4> &transforms) const;

    const aiNodeAnim *findNodeAnimation(aiAnimation *pAnimation, std::string basic_string) const;

    void deserializeCustomizations();

    int32_t buildEditorBoneTreeRecursive(std::shared_ptr<BoneNode> boneNode, int32_t selectedBoneNodeID);

#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    friend class AssetManager;
    /**
     * This is used by cereal for deserialize
     */
    ModelAsset() : Asset(nullptr, 0, std::vector<std::string>()) {};

public:
    ModelAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);

    void afterDeserialize(AssetManager *assetManager, std::vector<std::string> files);
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
    bool getTransform(long time, bool looped, std::string animationName, std::vector<glm::mat4> &transformMatrix) const; //this method takes vector to avoid copying it

    bool getTransformBlended(std::string animationName1, long time1, bool looped1,
                                         std::string animationName2, long time2, bool looped2,
                                         float blendFactor, std::vector<glm::mat4> &transformMatrixVector) const;

    const glm::vec3 &getBoundingBoxMin() const { return boundingBoxMin; }

    const glm::vec3 &getBoundingBoxMax() const { return boundingBoxMax; }

    const glm::vec3 &getCenterOffset() const { return centerOffset; }

    /*
     * FIXME: the materials should be const too
     */
    const std::unordered_map<std::string, std::shared_ptr<Material>> &getMaterialMap() const { return materialMap; };

    ~ModelAsset() {
        //FIXME GPU side is not freed
    }

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

    void fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations, const std::string &animationNamePrefix = "");

    const std::unordered_map<std::string, std::shared_ptr<AnimationInterface>> &getAnimations() const {
        return animations;
    }

    void serializeCustomizations();

    int32_t buildEditorBoneTree(int32_t selectedBoneNodeID);
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void save( Archive & ar ) const {
        std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> textures;
        size_t index = 0;
        std::shared_ptr<const AssetManager::EmbeddedTexture> embeddedTexture = assetManager->getEmbeddedTextures(name, index);
        while(embeddedTexture != nullptr) {
            textures.push_back(embeddedTexture);
            index++;
            embeddedTexture = assetManager->getEmbeddedTextures(name, index);
        }
        ar(assetID, boneIDCounter, boneIDCounterPerMesh, textures,                   hasAnimation, rootNode, boundingBoxMax, boundingBoxMin, centerOffset, boneInformationMap, simplifiedMeshes, meshes, animations, animationSections, customizationAfterSave, materialMap);
    }

    template<class Archive>
    void load( Archive & ar ) {
        temporaryEmbeddedTextures = std::make_unique<std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>>>();
        ar(assetID, boneIDCounter, boneIDCounterPerMesh, *temporaryEmbeddedTextures, hasAnimation, rootNode, boundingBoxMax, boundingBoxMin, centerOffset, boneInformationMap, simplifiedMeshes, meshes, animations, animationSections, customizationAfterSave, materialMap);
        //now update embedded textures to assetManager
    }
#endif

};


#endif //LIMONENGINE_MODELASSET_H
