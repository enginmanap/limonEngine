//
// Created by engin on 14.09.2016.
//

#ifndef UBERGAME_MESHASSET_H
#define UBERGAME_MESHASSET_H

#include <vector>
#include <iostream>
#include <assimp/scene.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>

#include "../glm/glm.hpp"
#include "../Utils/GLMConverter.h"
#include "AssetManager.h"
#include "../Material.h"


class MeshAsset {
    uint_fast32_t vao, ebo;
    uint_fast32_t triangleCount, vertexCount;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;

    bool bones;

    std::vector<glm::lowp_uvec4> boneIDs;
    std::vector<glm::vec4> boneWeights;

    const Material *material;

    uint_fast32_t bulletTriangleCount;
    btTriangleMesh *bulletMesh;
    std::vector<glm::mediump_uvec3> bulletMeshFaces;
    btShapeHull *hull;
    btConvexTriangleMeshShape *convexShape;
    btConvexShape *simplifiedConvexShape;
    std::vector<btConvexShape *> shapeCopies;

    std::vector<uint_fast32_t> bufferObjects;


public:
    MeshAsset(AssetManager *assetManager, const aiMesh *currentMesh, const Material *material,
              const std::map<std::string, uint_fast32_t> &boneIDMap);

    uint_fast32_t getTriangleCount() const { return triangleCount; }

    uint_fast32_t getVao() const { return vao; }

    uint_fast32_t getEbo() const { return ebo; }

    btConvexShape *getCollisionShape() {
        btTriangleMesh *copyMesh = new btTriangleMesh();
        if (hull == NULL) {
            for (int i = 0; i < bulletTriangleCount; ++i) {
                copyMesh->addTriangle(GLMConverter::GLMToBlt(vertices.at(bulletMeshFaces[i].x)),
                                      GLMConverter::GLMToBlt(vertices.at(bulletMeshFaces[i].y)),
                                      GLMConverter::GLMToBlt(vertices.at(bulletMeshFaces[i].z)));
            }
        } else {
            for (int i = 0; i < bulletTriangleCount; ++i) {
                copyMesh->addTriangle(hull->getVertexPointer()[bulletMeshFaces[i].x],
                                      hull->getVertexPointer()[bulletMeshFaces[i].y],
                                      hull->getVertexPointer()[bulletMeshFaces[i].z]);
            }
        }
        btConvexShape *copyShape = new btConvexTriangleMeshShape(copyMesh);
        shapeCopies.push_back(copyShape);
        return copyShape;
    }

    bool addWeightToVertex(uint_fast32_t boneID, unsigned int vertex, float weight);

    const Material *getMaterial() const {
        return material;
    }

    bool hasBones() const;

    ~MeshAsset() {
        delete bulletMesh;
        delete simplifiedConvexShape;

        for (int i = 0; i < shapeCopies.size(); ++i) {
            delete shapeCopies[i];
        }

    }
};


#endif //UBERGAME_MESHASSET_H
