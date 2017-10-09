//
// Created by engin on 31.08.2016.
//

#ifndef UBERGAME_MODELASSET_H
#define UBERGAME_MODELASSET_H


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <cstdint>
#include <map>

#include "../Utils/AssimpUtils.h"
#include "../Material.h"
#include "Asset.h"
#include "MeshAsset.h"
#include "../Utils/GLMConverter.h"
#include "BoneNode.h"


class ModelAsset : public Asset {



    std::string name;

    BoneNode *rootNode;
    int_fast32_t boneIDCounter, boneIDCounterPerMesh;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::map<std::string, Material *> materialMap;
    std::vector<btConvexShape *> shapeCopies;
    std::vector<MeshAsset *> meshes;
    std::map<std::string, glm::mat4> meshOffsetmap;

    aiAnimation** animations; //FIXME exposing this like that is not logical, it prevents deleting scene object
    const aiScene *scene;
    Assimp::Importer import;
    glm::mat4 globalInverseTransform;

    bool hasAnimation;

    Material *loadMaterials(const aiScene *scene, unsigned int materialIndex);

    BoneNode *loadNodeTree(aiNode *aiNode);

    void createMeshes(aiNode *aiNode, glm::mat4 parentTransform);//parent transform is not reference on purpose
    //if it was, then we would need a stack

    bool findNode(const BoneNode *nodeToMatch, const aiMesh *meshToCheckBone, int *index);


    void traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform, aiAnimation *animation, float timeInTicks,
                                 std::vector<glm::mat4> &transforms) const;

    const aiNodeAnim *findNodeAnimation(aiAnimation *pAnimation, std::string basic_string) const;


    aiQuaternion getRotationQuat(const float timeInTicks, const aiNodeAnim *nodeAnimation) const;

    aiVector3D getScalingVector(const float timeInTicks, const aiNodeAnim *nodeAnimation) const;

    aiVector3D getPositionVector(const float timeInTicks, const aiNodeAnim *nodeAnimation) const;

public:
    ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList);

    bool isAnimated() const;

    /*
     * FIXME: this generates transforms for first animation for now
     */
    void getTransform(long time, std::vector<glm::mat4>& transformMatrix) const; //this method takes vector to avoid copying it

    const glm::vec3 &getBoundingBoxMin() const { return boundingBoxMin; }

    const glm::vec3 &getBoundingBoxMax() const { return boundingBoxMax; }

    const glm::vec3 &getCenterOffset() const { return centerOffset; }


    /*
     * FIXME: the materials should be const too
     */
    const std::map<std::string, Material *> &getMaterialMap() const { return materialMap; };

    ~ModelAsset() {
        for (std::vector<MeshAsset *>::iterator iter = meshes.begin(); iter != meshes.end(); ++iter) {
            delete (*iter);
        }

        for (std::map<std::string, Material *>::iterator iter = materialMap.begin();
             iter != materialMap.end(); ++iter) {
            delete iter->second;
        }
    }

    std::vector<MeshAsset *> getMeshes() const {
        return meshes;
    }

};


#endif //UBERGAME_MODELASSET_H
