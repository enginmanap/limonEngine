//
// Created by engin on 14.09.2016.
//

#ifndef UBERGAME_MESHASSET_H
#define UBERGAME_MESHASSET_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <assimp/scene.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <GLM/glm.hpp>

#include "../Utils/GLMConverter.h"
#include "AssetManager.h"
#include "../Material.h"
#include "BoneNode.h"


class MeshAsset {
    uint_fast32_t vao, ebo;
    uint_fast32_t triangleCount, vertexCount;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;

    const BoneNode *skeleton;
    std::map<std::string, uint_fast32_t> boneIdMap;

    bool bones;

    //below 2 elements are used for passing id-weight pairs to GPU
    std::vector<glm::lowp_uvec4> boneIDs;
    std::vector<glm::vec4> boneWeights;

    const Material *material;


    btTriangleMesh *bulletMesh;
    /*
     uint_fast32_t bulletTriangleCount;
std::vector<glm::mediump_uvec3> bulletMeshFaces;

btConvexTriangleMeshShape *convexShape;
btCollisionShape *finalCollisionShape;
enum CollisionShapeTypes { BT_BVH_TRIANGLE_MESH, BT_CONVEX_TRIANGLE_MESH, BT_CONVEX_HULL};
CollisionShapeTypes shapeType;
*/
    std::vector<btTriangleMesh *> shapeCopies;

    std::vector<uint_fast32_t> bufferObjects;


public:
    MeshAsset(AssetManager *assetManager, const aiMesh *currentMesh, const Material *material,
              const BoneNode *meshSkeleton, const glm::mat4 &parentTransform);

    uint_fast32_t getTriangleCount() const { return triangleCount; }

    uint_fast32_t getVao() const { return vao; }

    uint_fast32_t getEbo() const { return ebo; }

    btTriangleMesh * getBulletMesh();

    bool addWeightToVertex(uint_fast32_t boneID, unsigned int vertex, float weight);

    const Material *getMaterial() const {
        return material;
    }

    bool hasBones() const;

    ~MeshAsset() {
        delete bulletMesh;

        for (int i = 0; i < shapeCopies.size(); ++i) {
            delete shapeCopies[i];
        }

    }

    void fillBoneMap(const BoneNode *boneNode);

    BoneNode *getSkeletonCopy() {

    }
};


#endif //UBERGAME_MESHASSET_H
