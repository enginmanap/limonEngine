//
// Created by engin on 23.09.2026.
//

#include "LodGenerator.h"

#include "../../../libs/meshoptimizer/src/meshoptimizer.h"

static const float LOD_NORMAL_ATTRIBUTE_WEIGHT = 1.0f;//meshopt readme default, raising it keeps shading at the cost of triangles

LodGenerator::LodGenerator(const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals,
                           const std::vector<glm::vec2> &textureCoordinates, const uint16_t *indices, size_t indexCount)
        : vertices(vertices), indices(indices), indexCount(indexCount) {
    if (vertices.empty() || indices == nullptr || indexCount < 3) {
        return;
    }
    meshScale = meshopt_simplifyScale(&vertices[0].x, vertices.size(), sizeof(glm::vec3));
    hasTextureCoordinates = textureCoordinates.size() == vertices.size();
    buildAttributes(normals, textureCoordinates);
    buildUvSeamLocks(textureCoordinates);
    buildWeldedIndices();
}

void LodGenerator::buildAttributes(const std::vector<glm::vec3> &normals, const std::vector<glm::vec2> &textureCoordinates) {
    bool hasNormals = normals.size() == vertices.size();
    attributeCount = (hasNormals ? 3 : 0) + (hasTextureCoordinates ? 2 : 0);
    if (attributeCount == 0) {
        return;
    }
    attributes.resize(vertices.size() * attributeCount);
    for (size_t vertex = 0; vertex < vertices.size(); ++vertex) {
        size_t base = vertex * attributeCount;
        if (hasNormals) {
            attributes[base + 0] = normals[vertex].x;
            attributes[base + 1] = normals[vertex].y;
            attributes[base + 2] = normals[vertex].z;
        }
        if (hasTextureCoordinates) {
            size_t textureBase = base + (hasNormals ? 3 : 0);
            attributes[textureBase + 0] = textureCoordinates[vertex].x;
            attributes[textureBase + 1] = textureCoordinates[vertex].y;
        }
    }

    attributeWeights.assign(attributeCount, LOD_NORMAL_ATTRIBUTE_WEIGHT);
    if (hasTextureCoordinates) {
        double positionLength = 0;
        double textureLength = 0;
        for (size_t triangle = 0; triangle + 2 < indexCount; triangle = triangle + 3) {
            for (uint32_t corner = 0; corner < 3; ++corner) {
                uint16_t first = indices[triangle + corner];
                uint16_t second = indices[triangle + (corner + 1) % 3];
                positionLength += glm::length(vertices[first] - vertices[second]);
                textureLength += glm::length(textureCoordinates[first] - textureCoordinates[second]);
            }
        }
        //meshopt asks for the reciprocal UV density in its own rescaled units, so a stretched atlas doesn't outweigh geometry
        float uvWeight = (textureLength > 0 && meshScale > 0) ? (float) (positionLength / textureLength / meshScale) : 0.0f;
        size_t textureWeightBase = hasNormals ? 3 : 0;
        attributeWeights[textureWeightBase + 0] = uvWeight;
        attributeWeights[textureWeightBase + 1] = uvWeight;
    }
}

void LodGenerator::buildUvSeamLocks(const std::vector<glm::vec2> &textureCoordinates) {
    if (!hasTextureCoordinates) {
        return;//nothing to protect, permissive can collapse freely
    }
    std::vector<unsigned int> positionRemap(vertices.size());
    meshopt_generatePositionRemap(&positionRemap[0], &vertices[0].x, vertices.size(), sizeof(glm::vec3));
    std::vector<unsigned char> seamAtPosition(vertices.size(), 0);
    for (size_t vertex = 0; vertex < vertices.size(); ++vertex) {
        if (textureCoordinates[vertex] != textureCoordinates[positionRemap[vertex]]) {
            seamAtPosition[positionRemap[vertex]] = 1;
        }
    }
    vertexLock.assign(vertices.size(), 0);
    for (size_t vertex = 0; vertex < vertices.size(); ++vertex) {
        if (seamAtPosition[positionRemap[vertex]]) {
            //Protect only stops collapses across the seam, it still slides along it and eats windows. The scorer's uv check catches that
            vertexLock[vertex] = meshopt_SimplifyVertex_Protect;
        }
    }
}

void LodGenerator::buildWeldedIndices() {
    //redirecting indices alone does not weld: meshopt groups vertex copies over the whole buffer, so the shadow
    //index buffer is what makes the collapses legal, and SimplifySparse below is what makes it read them
    weldedIndices.resize(indexCount);
    meshopt_generateShadowIndexBuffer(&weldedIndices[0], indices, indexCount, &vertices[0].x,
                                      vertices.size(), sizeof(glm::vec3), sizeof(glm::vec3));
}

size_t LodGenerator::generate(GeneratorKind kind, float targetError, std::vector<uint16_t> &outIndices, float &outRelativeError) const {
    outRelativeError = 0.0f;
    outIndices.clear();
    if (indices == nullptr || indexCount < 3 || vertices.empty()) {
        return 0;
    }
    outIndices.resize(indexCount);

    size_t resultCount = 0;
    if (kind == GeneratorKind::SILHOUETTE_ONLY) {
        resultCount = meshopt_simplify(&outIndices[0], &weldedIndices[0], indexCount,
                                       &vertices[0].x, vertices.size(), sizeof(glm::vec3),
                                       0, targetError,
                                       meshopt_SimplifySparse | meshopt_SimplifyPrune,
                                       &outRelativeError);
    } else if (attributeCount > 0) {
        resultCount = meshopt_simplifyWithAttributes(&outIndices[0], indices, indexCount,
                                                     &vertices[0].x, vertices.size(), sizeof(glm::vec3),
                                                     &attributes[0], attributeCount * sizeof(float),
                                                     &attributeWeights[0], attributeCount,
                                                     vertexLock.empty() ? nullptr : &vertexLock[0],
                                                     0, targetError,
                                                     meshopt_SimplifyPermissive | meshopt_SimplifyPrune,
                                                     &outRelativeError);
    } else {
        //no normals and no UVs to protect, so there is nothing for the attribute aware call to do
        resultCount = meshopt_simplify(&outIndices[0], indices, indexCount,
                                       &vertices[0].x, vertices.size(), sizeof(glm::vec3),
                                       0, targetError, meshopt_SimplifyPrune, &outRelativeError);
    }
    outIndices.resize(resultCount);
    return resultCount;
}
