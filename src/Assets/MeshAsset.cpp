//
// Created by engin on 14.09.2016.
//

#include "MeshAsset.h"

#include <glm/gtc/type_ptr.hpp>
#include <Utils/GLMConverter.h>

#include "limonAPI/Graphics/GraphicsInterface.h"
#include "../../libs/meshoptimizer/src/meshoptimizer.h"
#include "Lod/LodGenerator.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include "snapdragon-oc/Source/app/FuzzyCulling/API/SDOCAPI.h"

MeshAsset::MeshAsset(const aiMesh *currentMesh, std::string name, std::shared_ptr<const BoneNode> meshSkeleton,
                     const glm::mat4 &parentTransform, const bool isPartOfAnimated, bool reverseWinding)
        : name(name), parentTransform(parentTransform), isPartOfAnimated(isPartOfAnimated), reverseWinding(reverseWinding) {
    if (!currentMesh->HasPositions()) {
        throw "No position found"; //Not going to process if mesh is empty
    }

    vertexCount = currentMesh->mNumVertices;
    if(!setTriangles(currentMesh)) {
        return;
    }

    //If model is animated, but mesh has no bones, it is most likely we need to attach to the nearest parent.
    //all eight corners, a mirroring or rotating parent transform would otherwise leave min above max
    glm::vec3 sourceMin = GLMConverter::AssimpToGLM(currentMesh->mAABB.mMin);
    glm::vec3 sourceMax = GLMConverter::AssimpToGLM(currentMesh->mAABB.mMax);
    this->minAABB = glm::vec4(std::numeric_limits<float>::max());
    this->maxAABB = glm::vec4(std::numeric_limits<float>::lowest());
    for (uint32_t corner = 0; corner < 8; ++corner) {
        glm::vec4 transformedCorner = parentTransform * glm::vec4((corner & 1) ? sourceMax.x : sourceMin.x,
                                                                  (corner & 2) ? sourceMax.y : sourceMin.y,
                                                                  (corner & 4) ? sourceMax.z : sourceMin.z, 1.0f);
        this->minAABB = glm::min(this->minAABB, transformedCorner);
        this->maxAABB = glm::max(this->maxAABB, transformedCorner);
    }
    //loadBoneInformation
    if (currentMesh->HasBones()) {
        this->bones = true;

        assert(meshSkeleton != nullptr);
        this->skeleton = meshSkeleton;
        //we will create a bone* vector so we can load the weight information to GPU. The same vector should be
        //copied and filled with animation information on Model class
        //an a name->bone* map, for animation.
        fillBoneMap(this->skeleton);

        boneIDs.resize(vertices.size());
        boneWeights.resize(vertices.size());
        for (unsigned int j = 0; j < currentMesh->mNumBones; ++j) {
            uint32_t boneID = boneIdMap[currentMesh->mBones[j]->mName.C_Str()];
            /*
             * Assimp has a bone array with weight lists for vertices,
             * we need a vertex array with weight list for bones.
             * This loop should generate that
             */
            for (uint32_t k = 0; k < currentMesh->mBones[j]->mNumWeights; ++k) {
                if(currentMesh->mBones[j]->mWeights[k].mWeight > 0.0f) {
                    addWeightToVertex(boneID, currentMesh->mBones[j]->mWeights[k].mVertexId,
                                      currentMesh->mBones[j]->mWeights[k].mWeight);
                }
            }


        }
        //std::cout << "Animation added for mesh" << std::endl;
    } else {
        if(isPartOfAnimated) {
            //what to do now? now we assign bone id of the node, and weight of 1.0

            this->bones = true;

            assert(meshSkeleton != nullptr);
            this->skeleton = meshSkeleton;
            //we will create a bone* vector so we can load the weight information to GPU. The same vector should be
            //copied and filled with animation information on Model class
            //an a name->bone* map, for animation.
            fillBoneMap(this->skeleton);

            boneIDs.resize(vertices.size());
            boneWeights.resize(vertices.size());
            uint32_t boneID = boneIdMap[name];
            /*
             * Assimp has a bone array with weight lists for vertices,
             * we need a vertex array with weight list for bones.
             * This loop should generate that
             */
            for (uint32_t k = 0; k < currentMesh->mNumVertices; ++k) {
                addWeightToVertex(boneID, k, 1.0f);
            }

            //std::cout << "Animation added for mesh" << std::endl;
        } else {
            this->bones = false;
        }
    }
    weldIdenticalVertices();//after bones, weights are indexed by assimp's vertex ids until here
}

void MeshAsset::weldIdenticalVertices() {
    if (vertices.empty() || faces.empty()) {
        return;
    }
    std::vector<meshopt_Stream> streams;
    streams.push_back({&vertices[0], sizeof(glm::vec3), sizeof(glm::vec3)});
    if (normals.size() == vertices.size()) {
        streams.push_back({&normals[0], sizeof(glm::vec3), sizeof(glm::vec3)});
    }
    if (textureCoordinates.size() == vertices.size()) {
        streams.push_back({&textureCoordinates[0], sizeof(glm::vec2), sizeof(glm::vec2)});
    }
    //skinning is part of what a vertex is, two copies with different weights must stay apart
    if (boneIDs.size() == vertices.size()) {
        streams.push_back({&boneIDs[0], sizeof(glm::lowp_uvec4), sizeof(glm::lowp_uvec4)});
    }
    if (boneWeights.size() == vertices.size()) {
        streams.push_back({&boneWeights[0], sizeof(glm::vec4), sizeof(glm::vec4)});
    }

    std::vector<uint32_t> indices(faces.size() * 3);
    for (size_t face = 0; face < faces.size(); ++face) {
        indices[face * 3 + 0] = faces[face].x;
        indices[face * 3 + 1] = faces[face].y;
        indices[face * 3 + 2] = faces[face].z;
    }
    std::vector<uint32_t> remap(vertices.size());
    size_t weldedCount = meshopt_generateVertexRemapMulti(&remap[0], &indices[0], indices.size(), vertices.size(),
                                                          &streams[0], streams.size());
    if (weldedCount == vertices.size()) {
        return;
    }

    meshopt_remapVertexBuffer(&vertices[0], &vertices[0], vertices.size(), sizeof(glm::vec3), &remap[0]);
    if (normals.size() == vertices.size()) {
        meshopt_remapVertexBuffer(&normals[0], &normals[0], normals.size(), sizeof(glm::vec3), &remap[0]);
        normals.resize(weldedCount);
    }
    if (textureCoordinates.size() == vertices.size()) {
        meshopt_remapVertexBuffer(&textureCoordinates[0], &textureCoordinates[0], textureCoordinates.size(), sizeof(glm::vec2), &remap[0]);
        textureCoordinates.resize(weldedCount);
    }
    if (boneIDs.size() == vertices.size()) {
        meshopt_remapVertexBuffer(&boneIDs[0], &boneIDs[0], boneIDs.size(), sizeof(glm::lowp_uvec4), &remap[0]);
        boneIDs.resize(weldedCount);
    }
    if (boneWeights.size() == vertices.size()) {
        meshopt_remapVertexBuffer(&boneWeights[0], &boneWeights[0], boneWeights.size(), sizeof(glm::vec4), &remap[0]);
        boneWeights.resize(weldedCount);
    }
    vertices.resize(weldedCount);

    for (size_t face = 0; face < faces.size(); ++face) {
        faces[face] = glm::u16vec3(remap[faces[face].x], remap[faces[face].y], remap[faces[face].z]);
    }
    vertexCount = (uint32_t) weldedCount;
}


void MeshAsset::loadGPUPart(AssetManager *assetManager) {
    if (renderListId != NO_RENDER_LIST_ID) {
        std::cerr << "Mesh " << name << " loads its GPU part twice!" << std::endl;
        exit(-1);
    }
    renderListId = assetManager->acquireRenderListMeshId();
    /*** things should be set by serialize */
    //triangleCount
    //vertexCount
    // isPartOfAnimated
    //vertices
    //faces
    //normals
    //textureCoordinates
    //bones
    //skeleton
    // boneIDs
    // boneWeights
    // boneIDMap


    uint32_t vbo;
    assetManager->getGraphicsWrapper()->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    assetManager->getGraphicsWrapper()->bufferNormalData(normals, vao, vbo, 4);
    bufferObjects.push_back(vbo);

    if (!textureCoordinates.empty()) {
        assetManager->getGraphicsWrapper()->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3);
        bufferObjects.push_back(vbo);
    }

    if (this->bones) {
        assetManager->getGraphicsWrapper()->bufferExtraVertexData(boneIDs, vao, vbo, 5);
        bufferObjects.push_back(vbo);

        assetManager->getGraphicsWrapper()->bufferExtraVertexData(boneWeights, vao, vbo, 6);
        bufferObjects.push_back(vbo);
    }
}

bool MeshAsset::setTriangles(const aiMesh *currentMesh) {
    //In this part, the "if"s can be put in for, but then we will check them for each iteration. I am
    // not sure if that creates enough performance difference, it can be checked.
    if(isPartOfAnimated) {
        if (currentMesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
                glm::vec2 vectorsTextureCoordinates(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y);
                normalizeTextureCoordinates(vectorsTextureCoordinates);
                textureCoordinates.push_back(vectorsTextureCoordinates);

            }
        } else {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
            }
        }
    } else {

        if (currentMesh->HasTextureCoords(0)) {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]), 1.0f)));
                normals.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]), 0.0f)));
                glm::vec2 vectorsTextureCoordinates(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y);
                normalizeTextureCoordinates(vectorsTextureCoordinates);
                textureCoordinates.push_back(vectorsTextureCoordinates);

            }
        } else {
            for (unsigned int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]), 1.0f)));
                normals.push_back(glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]), 0.0f)));
            }
        }
    }
    if (!reverseWinding) { //if the asset is regular
        for (unsigned int j = 0; j < currentMesh->mNumFaces; ++j) {
            if(currentMesh->mFaces[j].mNumIndices == 3) {
                faces.push_back(glm::vec3(currentMesh->mFaces[j].mIndices[0],
                                          currentMesh->mFaces[j].mIndices[1],
                                          currentMesh->mFaces[j].mIndices[2]));
            }
        }
    } else { // if the asset is flipped, we flip vertice 2 and 1.
        for (unsigned int j = 0; j < currentMesh->mNumFaces; ++j) {
            if(currentMesh->mFaces[j].mNumIndices == 3) {
                faces.push_back(glm::vec3(currentMesh->mFaces[j].mIndices[0],
                                          currentMesh->mFaces[j].mIndices[2],
                                          currentMesh->mFaces[j].mIndices[1]));
            }
        }
    }
    triangleCount.assign(1, (uint32_t) faces.size());
    offsets.assign(1, 0);
    builtPlan.clear();
    bakedOccluders.assign(1, std::vector<uint16_t, AlignedAllocator<uint16_t, 64>>());
    if(faces.empty()) {
        return false;
    }
    return true;
}

//the ranges this mesh already holds came from a plan, and a model loaded from a binary arrives with both. Same
//plan means the meshopt work below would reproduce exactly what is there
bool MeshAsset::hasLodsFor(const std::vector<LodLadder::LevelPlan> &plan) const {
    return builtPlan == plan;
}

// the levels themselves are decided per model, by measuring what each one can survive, see LodLadder
void MeshAsset::buildLods(const LodGenerator *generator, const std::vector<LodLadder::LevelPlan> &plan) {
    if (triangleCount.empty() || triangleCount[0] == 0 || vertices.empty() || generator == nullptr) {
        return;
    }
    //rebuilding: drop whatever ranges are appended after LOD0 and start again
    faces.resize(triangleCount[0]);
    triangleCount.resize(1);
    offsets.resize(1);
    builtPlan.clear();
    bakedOccluders.assign(1, std::vector<uint16_t, AlignedAllocator<uint16_t, 64>>());

    std::vector<uint16_t> lodIndices;
    for (size_t level = 0; level < plan.size() && triangleCount.size() < LOD_MAX_LEVEL_COUNT; ++level) {
        float ignoredRelativeError = 0.0f;
        LodGenerator::GeneratorKind kind = plan[level].silhouetteOnly ? LodGenerator::GeneratorKind::SILHOUETTE_ONLY
                                                                     : LodGenerator::GeneratorKind::STRUCTURE_PRESERVING;
        //every level simplifies LOD0, not the level before it, so an early level can not compound into a later one
        size_t resultIndexCount = generator->generate(kind, plan[level].targetError, lodIndices, ignoredRelativeError);
        uint32_t previousLevel = (uint32_t) triangleCount.size() - 1;
        offsets.push_back(offsets[previousLevel] + (triangleCount[previousLevel] * 3));
        triangleCount.push_back((uint32_t) (resultIndexCount / 3));
        builtPlan.push_back(plan[level]);
        bakedOccluders.push_back(std::vector<uint16_t, AlignedAllocator<uint16_t, 64>>());
        for (size_t index = 0; index + 2 < resultIndexCount; index = index + 3) {
            if (lodIndices[index] >= vertices.size() || lodIndices[index + 1] >= vertices.size() ||
                lodIndices[index + 2] >= vertices.size()) {
                //drawing a range that points outside the vertex buffer is what scattered geometry looks like
                std::cerr << "LOD level " << triangleCount.size() - 1 << " of " << name
                          << " produced an index outside the vertex buffer, dropping the level" << std::endl;
                //the triangles already appended for this level have to go with it, or the next level's offset,
                //which was computed before them, starts inside the leftovers
                faces.resize(offsets.back() / 3);
                triangleCount.back() = 0;
                break;
            }
            faces.push_back(glm::u16vec3(lodIndices[index + 0],
                                         lodIndices[index + 1],
                                         lodIndices[index + 2]));
        }
    }

    //every range has to sit inside faces, or a draw reads someone else's triangles
    uint32_t expectedIndexCount = offsets.back() + (triangleCount.back() * 3);
    if (expectedIndexCount != faces.size() * 3) {
        std::cerr << "LOD ranges of " << name << " end at index " << expectedIndexCount
                  << " but the index buffer holds " << faces.size() * 3 << std::endl;
    }
}

// occluders are rendered from this every frame, so we pay the bake once at load. A forced level option bakes only
// that level here, exporting a limonmodel bakes the rest. Animated meshes never occlude, their occluder would need the node
// transform and the pose, which we don't have at load
void MeshAsset::bakeOccluderLod(uint32_t lodLevel) {
    if (lodLevel >= triangleCount.size() || isPartOfAnimated || bones ||
        triangleCount[lodLevel] == 0 || !bakedOccluders[lodLevel].empty()) {
        return;
    }
    //every level indexes the whole vertex buffer, and a level that merges no quads keeps every vertex it was handed
    //and quantizes against all of them, so each level is baked from only the vertices it uses
    const uint16_t *levelIndices = (const uint16_t *) &(faces.data()->x) + offsets[lodLevel];
    const uint32_t levelIndexCount = triangleCount[lodLevel] * 3;
    std::vector<uint32_t> levelIndexOfVertex(vertices.size(), UINT32_MAX);
    std::vector<glm::vec3> levelVertices;
    std::vector<uint16_t> levelLocalIndices(levelIndexCount);
    for (uint32_t index = 0; index < levelIndexCount; ++index) {
        uint32_t &levelIndex = levelIndexOfVertex[levelIndices[index]];
        if (levelIndex == UINT32_MAX) {
            levelIndex = static_cast<uint32_t>(levelVertices.size());
            levelVertices.push_back(vertices[levelIndices[index]]);
        }
        levelLocalIndices[index] = static_cast<uint16_t>(levelIndex);
    }
    int bakedShortCount = 0;//SDOC counts uint16s here, SDOCAPI.h says ints
    unsigned short *bakedData = sdocMeshBake(&bakedShortCount,
                                             &levelVertices[0].x,
                                             levelLocalIndices.data(),
                                             static_cast<unsigned int>(levelVertices.size()),
                                             levelIndexCount,
                                             15.0f, true, true, 0);
    if (bakedData == nullptr || bakedShortCount <= 0) {
        return;//baker refused this level, a request for it falls back to the raw index range
    }
    bakedOccluders[lodLevel].assign(bakedData, bakedData + bakedShortCount);
    sdocMeshBake(reinterpret_cast<int *>(bakedData), nullptr, nullptr, 0, 0, 0, false, false, 0);//SDOC's documented free
}

//a limonmodel carries every level, so the occluder LOD option still works on a map built from converted models
void MeshAsset::bakeAllOccluderLods() {
    for (uint32_t level = 0; level < triangleCount.size(); ++level) {
        bakeOccluderLod(level);
    }
}

void MeshAsset::normalizeTextureCoordinates(glm::vec2 &textureCoordinates) const {
    float fractionPart = textureCoordinates.x;
    if(fabs(textureCoordinates.x) > 1) {
        double integerPart;
        fractionPart = modf (textureCoordinates.x , &integerPart);
    }
    if(textureCoordinates.x < 0 ) {
        fractionPart = fractionPart + 1;
    }
    textureCoordinates.x = fractionPart;

    fractionPart = textureCoordinates.y;
    if(fabs(textureCoordinates.y) > 1) {
        double integerPart;
        fractionPart = modf (textureCoordinates.y , &integerPart);
    }
    if(textureCoordinates.y < 0 ) {
        fractionPart = fractionPart + 1;
    }
    textureCoordinates.y = fractionPart;
}

bool MeshAsset::addWeightToVertex(uint32_t boneID, unsigned int vertex, float weight) {
    //the weights are suppose to be ordered,
    for (int i = 0; i < 4; ++i) { //we allow only 4 bones per vertex
        if (boneWeights[vertex][i] < weight) {
            //shift to open slot
            if(boneWeights[vertex][i] > 0.0f) {
                for (int j = 3; j > i; --j) {
                    boneWeights[vertex][j] = boneWeights[vertex][j - 1];
                    boneIDs[vertex][j] = boneIDs[vertex][j - 1];
                }
            }
            boneWeights[vertex][i] = weight;
            boneIDs[vertex][i] = boneID;
            return true;
        }
    }
    return false;
}

bool MeshAsset::hasBones() const {
    return bones;
}

bool MeshAsset::hasNonFiniteVertex() const {
    for (const glm::vec3 &vertex : vertices) {
        if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y) || !std::isfinite(vertex.z)) {
            return true;
        }
    }
    return false;
}

void MeshAsset::fillBoneMap(std::shared_ptr<const BoneNode> boneNode) {
    if (boneNode == nullptr) {
        return;
    }
    boneIdMap[boneNode->name] = boneNode->boneID;
    for (unsigned int i = 0; i < boneNode->children.size(); ++i) {
        fillBoneMap(boneNode->children[i]);
    }
}
void MeshAsset::releaseRenderListId(AssetManager *assetManager) {
    if (renderListId == NO_RENDER_LIST_ID) {
        return;//never reached the GPU, the packer loads skip it
    }
    assetManager->releaseRenderListMeshId(renderListId);
    renderListId = NO_RENDER_LIST_ID;
}

void MeshAsset::reuploadGeometryBuffers(AssetManager *assetManager) {
    if (bufferObjects.empty()) {
        return;//never reached the GPU, the next loadGPUPart will upload whatever is current
    }
    //bufferObjects[0] is the position buffer loadGPUPart pushed first. Only faces changed, but updateVertexData
    //is the only call the backend offers, so the positions are re-sent with them
    assetManager->getGraphicsWrapper()->updateVertexData(vertices, faces, bufferObjects[0], ebo);
}
