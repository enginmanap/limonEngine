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


struct AnimationNode {
    std::vector<glm::vec3> translates;
    std::vector<float>translateTimes;
    std::vector<glm::vec3> scales;
    std::vector<float>scaleTimes;
    std::vector<glm::quat> rotations;
    std::vector<float>rotationTimes;
};

struct AnimationSet {
    float ticksPerSecond;
    float duration;
    std::unordered_map<std::string, AnimationNode*> nodes;//FIXME these should be removed
};

class ModelAsset : public Asset {
    std::string name;
    std::unordered_map<std::string, AnimationSet*> animations;//FIXME these should be removed
    BoneNode *rootNode;
    int_fast32_t boneIDCounter, boneIDCounterPerMesh;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::unordered_map<std::string, Material *> materialMap;
    std::vector<btConvexShape *> shapeCopies;
    std::vector<MeshAsset *> meshes;
    std::unordered_map<std::string, MeshAsset *> simplifiedMeshes;
    std::unordered_map<std::string, glm::mat4> meshOffsetmap;
    glm::mat4 globalInverseTransform;

    bool hasAnimation;

    Material *loadMaterials(const aiScene *scene, unsigned int materialIndex);

    void createMeshes(const aiScene *scene, aiNode *aiNode, glm::mat4 parentTransform);//parent transform is not reference on purpose
    //if it was, then we would need a stack

    BoneNode *loadNodeTree(aiNode *aiNode);

    bool findNode(const std::string &nodeName, BoneNode** foundNode, BoneNode* searchRoot) const;

    void traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform, const AnimationSet *animation, float timeInTicks,
                                 std::vector<glm::mat4> &transforms) const;

    const aiNodeAnim *findNodeAnimation(aiAnimation *pAnimation, std::string basic_string) const;


    glm::quat getRotationQuat(const float timeInTicks, const AnimationNode *nodeAnimation) const;

    glm::vec3 getScalingVector(const float timeInTicks, const AnimationNode *nodeAnimation) const;

    glm::vec3 getPositionVector(const float timeInTicks, const AnimationNode *nodeAnimation) const;

public:
    ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList);

    bool isAnimated() const;

    void getTransform(long time, std::string animationName, std::vector<glm::mat4> &transformMatrix) const; //this method takes vector to avoid copying it

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

    std::unordered_map<float, glm::mat4> createTransformsForAllTimes(aiNodeAnim *animation);

    glm::mat4 calculateTransform(AnimationNode *animation, float time) const;

    const std::unordered_map<std::string, AnimationSet *> &getAnimations() const {
        return animations;
    }
};


#endif //LIMONENGINE_MODELASSET_H
