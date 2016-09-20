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

#include "../Utils/AssimpUtils.h"
#include "../Material.h"
#include "Asset.h"
#include "MeshAsset.h"
#include "../Utils/GLMConverter.h"


class ModelAsset : public Asset {
    std::string name;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::map<std::string, Material *> materialMap;
    std::vector<btConvexShape *> shapeCopies;
    std::vector<MeshAsset *> meshes;

    Material *loadMaterials(const aiScene *scene, unsigned int materialIndex);


public:
    ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList);

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
