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
    };

    struct AnimationSection {
        std::string baseAnimationName;
        std::string animationName;
        float startTime;
        float endTime;

        AnimationSection(const std::string &sourceAnimationName, const std::string &animationName,
                         float startTime, float endTime) : baseAnimationName(
                sourceAnimationName), animationName(animationName), startTime(startTime), endTime(endTime) {}
    };

    std::string name;
    std::unordered_map<std::string, AnimationInterface*> animations;
    BoneNode *rootNode;
    int_fast32_t boneIDCounter, boneIDCounterPerMesh;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::unordered_map<std::string, Material *> materialMap;
    std::vector<btConvexShape *> shapeCopies;
    std::vector<MeshAsset *> meshes;
    std::vector<AnimationSection> animationSections;

    std::unordered_map<std::string, MeshAsset *> simplifiedMeshes;
    std::unordered_map<std::string, BoneInformation> boneInformationMap;

    bool hasAnimation;
    bool customizationAfterSave = false;

    Material *loadMaterials(const aiScene *scene, unsigned int materialIndex);

    void createMeshes(const aiScene *scene, aiNode *aiNode, glm::mat4 parentTransform);//parent transform is not reference on purpose
    //if it was, then we would need a stack

    BoneNode *loadNodeTree(aiNode *aiNode);

    bool findNode(const std::string &nodeName, BoneNode** foundNode, BoneNode* searchRoot) const;

    void traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform, const AnimationInterface *animation,
                                 float timeInTicks,
                                 std::vector<glm::mat4> &transforms) const;

    const aiNodeAnim *findNodeAnimation(aiAnimation *pAnimation, std::string basic_string) const;

    void deserializeCustomizations();

public:
    ModelAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList);

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

    const glm::vec3 &getBoundingBoxMin() const { return boundingBoxMin; }

    const glm::vec3 &getBoundingBoxMax() const { return boundingBoxMax; }

    const glm::vec3 &getCenterOffset() const { return centerOffset; }

    /*
     * FIXME: the materials should be const too
     */
    const std::unordered_map<std::string, Material *> &getMaterialMap() const { return materialMap; };

    ~ModelAsset() {
        std::cout << "Model asset deleted: " << name << std::endl;
        for (std::vector<MeshAsset *>::iterator iter = meshes.begin(); iter != meshes.end(); ++iter) {
            delete (*iter);
        }

        for (auto iter = simplifiedMeshes.begin(); iter != simplifiedMeshes.end(); ++iter) {
            delete iter->second;
        }

        for (std::unordered_map<std::string, Material *>::iterator iter = materialMap.begin();
             iter != materialMap.end(); ++iter) {
            delete iter->second;
        }
        //FIXME GPU side is not freed
    }

    std::vector<MeshAsset *> getMeshes() const {
        return meshes;
    }

    /**
     * This method checks if there is a simplified mesh with same name, and there is, adds simplified one instead of original.
     * @return hybrid of original and simplified meshes
     */
    std::vector<MeshAsset *> getPhysicsMeshes() const {
        if(simplifiedMeshes.size() == 0) {
            return meshes;
        }

        std::vector<MeshAsset *> meshAssets = meshes;//shallow copy
        for (unsigned int i = 0; i < meshes.size(); ++i) {
            std::string meshName = "UCX_" + meshes[i]->getName();
            if(simplifiedMeshes.find(meshName) != simplifiedMeshes.end()) {
                meshAssets[i] = simplifiedMeshes.at(meshName);
            }
        }
        return meshAssets;
    }

    void fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations);

    const std::unordered_map<std::string, AnimationInterface*> &getAnimations() const {
        return animations;
    }

    void serializeCustomizations();

};


#endif //LIMONENGINE_MODELASSET_H
