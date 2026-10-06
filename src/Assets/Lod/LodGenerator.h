//
// Created by engin on 23.09.2026.
//

#ifndef LIMONENGINE_LODGENERATOR_H
#define LIMONENGINE_LODGENERATOR_H

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

//simplified index ranges for one mesh. The expensive inputs (attributes, UV seam locks, welded indices) are built
//once and reused, the search asks for the same mesh at a dozen target errors
class LodGenerator {
public:
    //SILHOUETTE_ONLY welds positions and drops normals and UVs, so only a depth camera may draw what it makes
    enum class GeneratorKind {
        STRUCTURE_PRESERVING,
        SILHOUETTE_ONLY
    };

    //bone data is null for a static mesh. boneWeightDeviation is the attribute weight of a weight change, 0 ignores it
    LodGenerator(const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals,
                 const std::vector<glm::vec2> &textureCoordinates, const uint16_t *indices, size_t indexCount,
                 const std::vector<glm::lowp_uvec4> *boneIDs, const std::vector<glm::vec4> *boneWeights,
                 float boneWeightDeviation);

    //outRelativeError is relative to the mesh extent, multiply by getMeshScale() for model units. Returns 0 when
    //the mesh can not be simplified at all
    size_t generate(GeneratorKind kind, float targetError, std::vector<uint16_t> &outIndices, float &outRelativeError) const;

    //longest side of the mesh AABB, which is what meshopt's relative error is relative to
    float getMeshScale() const {
        return meshScale;
    }

private:
    void buildAttributes(const std::vector<glm::vec3> &normals, const std::vector<glm::vec2> &textureCoordinates);
    void appendBoneAttributes(const std::vector<glm::lowp_uvec4> &boneIDs, const std::vector<glm::vec4> &boneWeights,
                              float boneWeightDeviation);
    void buildUvSeamLocks(const std::vector<glm::vec2> &textureCoordinates);
    void buildWeldedIndices();

    static const uint32_t BONE_ATTRIBUTE_DIMENSIONS = 4;

    const std::vector<glm::vec3> &vertices;
    const uint16_t *indices;
    size_t indexCount;
    float meshScale = 0.0f;

    bool hasTextureCoordinates = false;
    size_t attributeCount = 0;              //floats per vertex: normal xyz, uv when it exists, then the bone mix
    std::vector<float> attributes;
    std::vector<float> attributeWeights;
    std::vector<unsigned char> vertexLock;  //meshopt_SimplifyVertex_Protect where UVs differ at a shared position
    std::vector<uint16_t> weldedIndices;    //positions welded, for the silhouette only generator
};

#endif //LIMONENGINE_LODGENERATOR_H
