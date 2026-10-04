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
#include "Lod/LodLadder.h"
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
    //a ceiling for anything that needs a bound, the level count itself comes from the options and is per model
    static constexpr uint32_t LOD_MAX_LEVEL_COUNT = 16;
private:
    uint32_t vao, ebo;
    uint32_t vertexCount;
    //index 0 is the original mesh and always exists, the rest are index ranges appended into faces
    std::vector<uint32_t> triangleCount;
    std::vector<uint32_t> offsets;
    //what the ranges above were built from. A model loaded from a binary arrives with both, so matching it is
    //how a converted model skips simplifying everything again at load
    std::vector<LodLadder::LevelPlan> builtPlan;
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
    std::vector<std::vector<uint16_t, AlignedAllocator<uint16_t, 64>>> bakedOccluders;

    std::vector<uint32_t> bufferObjects;
    bool setTriangles(const aiMesh *currentMesh);
    //assimp splits on channels we never read, like a second uv set. Those copies leave seam vertices with extra
    //wedges, which meshopt locks, so the same model simplifies differently depending on what its file carried
    void weldIdenticalVertices();
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
              const glm::mat4 &parentTransform, const bool isPartOfAnimated, bool reverseWinding = false);

    /**
     * Builds the index ranges for the levels ModelAsset calibrated. Separate from the constructor because the
     * calibration that decides these needs every mesh of the model to exist first.
     */
    void buildLods(const LodGenerator *generator, const std::vector<LodLadder::LevelPlan> &plan);

    //true when the ranges this mesh holds were built from exactly this plan, so there is nothing to do
    bool hasLodsFor(const std::vector<LodLadder::LevelPlan> &plan) const;
    void bakeOccluderLod(uint32_t lodLevel);
    void bakeAllOccluderLods();
    void buildBulletMesh();
    /**
     * This method sets GPU side of the deserialization, and uses AssetManager to access GPU with getGraphicsWrapper
     *
     * @param assetManager
     */
    void loadGPUPart(AssetManager *assetManager);
    //after buildLods rebuilt the ranges, the GPU still holds the old ones until this runs. Positions go along
    //because the backend has no index only update, not because they changed
    void reuploadGeometryBuffers(AssetManager *assetManager);

    // one entry per LOD, level 0 is the original mesh
    const uint32_t *getTriangleCount() const {
        return triangleCount.data();
    }

    const uint32_t *getOffsets() const{
        return offsets.data();
    }

    //empty for animated meshes and for anything the baker rejected, those still go through the raw index range
    const std::vector<uint16_t, AlignedAllocator<uint16_t, 64>> &getBakedOccluder(uint32_t requestedLodLevel) const {
        uint32_t lodLevel = requestedLodLevel < bakedOccluders.size() ? requestedLodLevel : (uint32_t) bakedOccluders.size() - 1;
        while (lodLevel > 0 && bakedOccluders[lodLevel].empty()) {//a LOD can simplify to nothing, or the baker can refuse it
            lodLevel--;
        }
        return bakedOccluders[lodLevel];
    }

    uint32_t getSimplestLodLevel(uint32_t requestedLodLevel) const {
        if (triangleCount.empty()) {
            return 0;
        }
        uint32_t lodLevel = requestedLodLevel < triangleCount.size() ? requestedLodLevel : (uint32_t) triangleCount.size() - 1;
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

    const std::vector<glm::vec3>& getNormals() const {
        return normals;
    }

    const std::vector<glm::vec2>& getTextureCoordinates() const {
        return textureCoordinates;
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
    static constexpr uint32_t SERIALIZATION_MAGIC = 0x4C4D463C;

    template<class Archive>
    void serialize(Archive & archive){
        uint32_t magic = SERIALIZATION_MAGIC;
        archive(magic);
        checkSerializationMagic(magic);
        archive( vertices, normals, textureCoordinates, faces, vertexCount, triangleCount, offsets, builtPlan, bakedOccluders, skeleton, bones, boneIDs, boneWeights, boneAttachedMeshes, boneIdMap, name, isPartOfAnimated, parentTransform, minAABB, maxAABB);
    }
#endif
};


#endif //LIMONENGINE_MESHASSET_H
