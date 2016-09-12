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
#include "../Utils/GLMConverter.h"


class ModelAsset : public Asset {
    std::string name;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    glm::vec3 centerOffset;

    std::map<std::string, Material *> materialMap;
    std::vector<uint_fast32_t> bufferObjects;
    uint_fast32_t vao, ebo;
    uint_fast32_t triangleCount, vertexCount;

    btTriangleMesh *bulletMesh;
    std::vector<btVector3> bulletMeshVertices;
    std::vector<glm::mediump_uvec3> bulletMeshFaces;
    btConvexTriangleMeshShape *convexShape;
    btConvexShape *simplifiedConvexShape;
    uint_fast32_t bulletTriangleCount;
    std::vector<btConvexShape *> shapeCopies;


public:
    ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList);

    const glm::vec3 &getBoundingBoxMin() const { return boundingBoxMin; }

    const glm::vec3 &getBoundingBoxMax() const { return boundingBoxMax; }

    const glm::vec3 &getCenterOffset() const { return centerOffset; }

    btConvexShape *getCollisionMesh() {
        btTriangleMesh *copyMesh = new btTriangleMesh();
        for (int i = 0; i < bulletTriangleCount; ++i) {
            copyMesh->addTriangle(bulletMeshVertices.at(bulletMeshFaces.at(i).x),
                                  bulletMeshVertices.at(bulletMeshFaces.at(i).y),
                                  bulletMeshVertices.at(bulletMeshFaces.at(i).z));
        }

        btConvexShape *copyShape = new btConvexTriangleMeshShape(copyMesh);
        shapeCopies.push_back(copyShape);
        return copyShape;
    }

    uint_fast32_t getTriangleCount() const { return triangleCount; }

    uint_fast32_t getVertexCount() const { return vertexCount; }

    uint_fast32_t getVao() const { return vao; }

    uint_fast32_t getEbo() const { return ebo; }

    /*
     * FIXME: the materials should be const too
     */
    const std::map<std::string, Material *> &getMaterialMap() const { return materialMap; };

    ~ModelAsset() {
        delete bulletMesh;
        delete simplifiedConvexShape;

        for (int i = 0; i < shapeCopies.size(); ++i) {
            delete shapeCopies[i];
        }

        for (std::map<std::string, Material *>::iterator iter = materialMap.begin();
             iter != materialMap.end(); ++iter) {
            delete iter->second;
        }
    }
};


#endif //UBERGAME_MODELASSET_H
