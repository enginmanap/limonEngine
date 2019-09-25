//
// Created by engin on 14.09.2016.
//

#ifndef LIMONENGINE_MESHASSET_H
#define LIMONENGINE_MESHASSET_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <assimp/scene.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <glm/glm.hpp>

#include "../Utils/GLMConverter.h"
#include "AssetManager.h"
#include "../Material.h"
#include "BoneNode.h"
#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include "../Utils/GLMCerealConverters.hpp"
#endif



class MeshAsset {
    uint_fast32_t vao, ebo;
    uint_fast32_t triangleCount, vertexCount;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::mediump_uvec3> faces;
    std::vector<glm::vec2> textureCoordinates;
    std::string name;

    std::map<uint_fast32_t, std::vector<uint_fast32_t >> boneAttachedMeshes;

    std::shared_ptr<const BoneNode> skeleton;
    std::map<std::string, uint_fast32_t> boneIdMap;

    bool bones;

    //below 2 elements are used for passing id-weight pairs to GPU
    std::vector<glm::lowp_uvec4> boneIDs;
    std::vector<glm::vec4> boneWeights;

    std::shared_ptr<const Material> material;
    glm::mat4 parentTransform;
    bool isPartOfAnimated;

    std::vector<btTriangleMesh *> shapeCopies;

    std::vector<uint_fast32_t> bufferObjects;
    bool setTriangles(const aiMesh *currentMesh);

    void normalizeTextureCoordinates(glm::vec2 &textureCoordinates) const;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    MeshAsset(){}
public:
    MeshAsset(AssetManager *assetManager, const aiMesh *currentMesh, std::string name,
              std::shared_ptr<const Material> material, std::shared_ptr<const BoneNode> meshSkeleton,
              const glm::mat4 &parentTransform,
              const bool isPartOfAnimated);

    /**
     * This method sets GPU side of the deserialization, and uses AssetManager to access GPU with getGraphicsWrapper
     *
     * @param assetManager
     */
    void afterDeserialize(AssetManager *assetManager);

    uint_fast32_t getTriangleCount() const { return triangleCount; }

    uint_fast32_t getVao() const { return vao; }

    uint_fast32_t getEbo() const { return ebo; }

    btTriangleMesh *getBulletMesh(std::map<uint_fast32_t, btConvexHullShape *> *hullMap,
                                  std::map<uint_fast32_t, btTransform> *parentTransformMap);

    bool addWeightToVertex(uint_fast32_t boneID, unsigned int vertex, float weight);

    std::shared_ptr<const Material> getMaterial() const {
        return material;
    }

    bool hasBones() const;

    ~MeshAsset() {
        for (unsigned int i = 0; i < shapeCopies.size(); ++i) {
            delete shapeCopies[i];
        }

        //FIXME buffer objects are not freed!

    }

    void fillBoneMap(std::shared_ptr<const BoneNode> boneNode);

    std::string getName() {
        return name;
    }
#ifdef CEREAL_SUPPORT
    template<class Archive>
    void serialize(Archive & archive){
        archive( vertices, normals, textureCoordinates, faces, vertexCount, triangleCount, skeleton, bones, boneIDs, boneWeights, boneAttachedMeshes, boneIdMap, material, name, isPartOfAnimated, parentTransform);
    }
#endif
};


#endif //LIMONENGINE_MESHASSET_H
