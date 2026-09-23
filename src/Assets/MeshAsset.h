//
// Created by engin on 14.09.2016.
//

#ifndef LIMONENGINE_MESHASSET_H
#define LIMONENGINE_MESHASSET_H

#include <vector>
#include <map>
#include <string>
#include <assimp/scene.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <glm/glm.hpp>

#include "../Material.h"
#include "BoneNode.h"
#include "../Utils/AlignedAllocator.hpp"
#ifdef CEREAL_SUPPORT
#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>
#include "../Utils/GLMCerealConverters.hpp"
#endif


class MeshAsset {
public:
    static constexpr uint32_t LOD_LEVEL_COUNT = 4;
private:
    //level 0 is the original mesh, the rest are driven by target error only, no triangle targets
    static constexpr float LOD_TARGET_ERRORS[LOD_LEVEL_COUNT] = {0.0f, 0.005f, 0.02f, 0.05f};

    uint32_t vao, ebo;
    uint32_t triangleCount[LOD_LEVEL_COUNT], offsets[LOD_LEVEL_COUNT], vertexCount;
    float lodError[LOD_LEVEL_COUNT] = {0.0f, 0.0f, 0.0f, 0.0f}; //meshopt error in model units, what LOD selection projects to pixels
    glm::vec4 minAABB, maxAABB;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::u16vec3> faces; //Possible reason for non portable data
    std::vector<glm::vec2> textureCoordinates;
    std::string name;

    std::map<uint32_t, std::vector<uint32_t>> boneAttachedMeshes;

    std::shared_ptr<const BoneNode> skeleton;
    std::map<std::string, uint32_t> boneIdMap;

    bool bones;

    //below 2 elements are used for passing id-weight pairs to GPU
    std::vector<glm::lowp_uvec4> boneIDs;
    std::vector<glm::vec4> boneWeights;

    glm::mat4 parentTransform;
    bool isPartOfAnimated;

    btTriangleMesh bulletMesh;
    btShapeHull *bulletHull;
    std::map<uint32_t, btConvexHullShape *> bulletHullMap;
    std::map<uint32_t, btTransform> bulletParentTransformMap;
    std::vector<btTriangleMesh *> shapeCopies;

    bool reverseWinding = false;

    //one per LOD, so the occluder level is a runtime choice. SDOC reads bakes with SIMD loads, hence the alignment
    std::vector<uint16_t, AlignedAllocator<uint16_t, 64>> bakedOccluders[LOD_LEVEL_COUNT];

    std::vector<uint32_t> bufferObjects;
    bool setTriangles(const aiMesh *currentMesh);
    void generateLods();
    void bakeOccluderLod(uint32_t lodLevel);
    void buildSimplifyAttributes(std::vector<float> &attributes, std::vector<float> &attributeWeights, float meshScale) const;
    void buildUvSeamLocks(std::vector<unsigned char> &vertexLock) const;
#ifdef CEREAL_SUPPORT
    void checkSerializationMagic(uint32_t magic) const;
#endif

    void normalizeTextureCoordinates(glm::vec2 &textureCoordinates) const;
#ifdef CEREAL_SUPPORT
    friend class cereal::access;
#endif
    MeshAsset(){}
public:
    MeshAsset(const aiMesh *currentMesh, std::string name, std::shared_ptr<const BoneNode> meshSkeleton,
              const glm::mat4 &parentTransform, const bool isPartOfAnimated, uint32_t bakeOccluderLodLevel, bool reverseWinding = false);
    void bakeAllOccluderLods();
    void buildBulletMesh();
    /**
     * This method sets GPU side of the deserialization, and uses AssetManager to access GPU with getGraphicsWrapper
     *
     * @param assetManager
     */
    void loadGPUPart(AssetManager *assetManager);

    // always returns LOD_LEVEL_COUNT elements
    const uint32_t *getTriangleCount() const {
        return triangleCount;
    }

    const uint32_t *getOffsets() const{
        return offsets;
    }

    // always returns LOD_LEVEL_COUNT elements, in model units, level 0 is always 0
    const float *getLodErrors() const {
        return lodError;
    }

    //empty for animated meshes and for anything the baker rejected, those still go through the raw index range
    const std::vector<uint16_t, AlignedAllocator<uint16_t, 64>> &getBakedOccluder(uint32_t requestedLodLevel) const {
        uint32_t lodLevel = requestedLodLevel < LOD_LEVEL_COUNT ? requestedLodLevel : LOD_LEVEL_COUNT - 1;
        while (lodLevel > 0 && bakedOccluders[lodLevel].empty()) {//a LOD can simplify to nothing, or the baker can refuse it
            lodLevel--;
        }
        return bakedOccluders[lodLevel];
    }

    uint32_t getSimplestLodLevel(uint32_t requestedLodLevel) const {
        uint32_t lodLevel = requestedLodLevel < LOD_LEVEL_COUNT ? requestedLodLevel : LOD_LEVEL_COUNT - 1;
        while (lodLevel > 0 && triangleCount[lodLevel] == 0) {//simplification can bottom out at zero triangles
            lodLevel--;
        }
        return lodLevel;
    }

    uint32_t getVao() const { return vao; }

    const std::vector<glm::vec3>& getVertices() {
        return vertices;
    }

    const std::vector<glm::u16vec3>& getFaces() {
        return faces;
    }

    uint32_t getEbo() const { return ebo; }

    btTriangleMesh *getBulletMesh(std::map<uint32_t, btConvexHullShape *> *hullMap,
                                  std::map<uint32_t, btTransform> *parentTransformMap);

    bool addWeightToVertex(uint32_t boneID, unsigned int vertex, float weight);

    bool hasBones() const;

    const glm::vec4& getAabbMin() const {
        return minAABB;
    }

    const glm::vec4& getAabbMax() const {
        return maxAABB;
    }

    ~MeshAsset() {
        for (unsigned int i = 0; i < shapeCopies.size(); ++i) {
            delete shapeCopies[i];
        }

        for (auto it = bulletHullMap.begin(); it != bulletHullMap.end(); ++it) {
            delete it->second;
        }
        //FIXME buffer objects are not freed!
    }

    void fillBoneMap(std::shared_ptr<const BoneNode> boneNode);

    std::string getName() {
        return name;
    }
#ifdef CEREAL_SUPPORT
    //bumped whenever the stored LOD data changes, an older file can not produce it, so we stop instead of reading garbage
    static constexpr uint32_t SERIALIZATION_MAGIC = 0x4C4D4633;

    template<class Archive>
    void serialize(Archive & archive){
        uint32_t magic = SERIALIZATION_MAGIC;
        archive(magic);
        checkSerializationMagic(magic);
        archive( vertices, normals, textureCoordinates, faces, vertexCount, triangleCount, offsets, lodError, bakedOccluders, skeleton, bones, boneIDs, boneWeights, boneAttachedMeshes, boneIdMap, name, isPartOfAnimated, parentTransform);
    }
#endif
};


#endif //LIMONENGINE_MESHASSET_H
