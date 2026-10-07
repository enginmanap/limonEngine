//
// Created by engin on 03.10.2026.
//

#include "ModelToWorldConverter.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <tinyxml2.h>

#include "AssetManager.h"
#include "ModelAsset.h"
#include "ModelAssetBuilder.h"
#include "../Transformation.h"
#include "../World.h"
#include "../XMLHelper.h"
#include "../GameObjects/Light.h"
#include "../Utils/GLMConverter.h"
#include "limonAPI/Graphics/GraphicsInterface.h"

ModelToWorldConverter::ModelToWorldConverter(std::shared_ptr<AssetManager> assetManager, const std::string &sourcePath,
                                             const std::string &outputDirectory, float scale)
        : assetManager(std::move(assetManager)), sourcePath(sourcePath), outputDirectory(outputDirectory), scale(scale) {
    std::filesystem::path source(sourcePath);
    sourceStem = source.stem().string();
    if (this->outputDirectory.empty()) {
        this->outputDirectory = (source.parent_path() / sourceStem).generic_string();
    }
    while (!this->outputDirectory.empty() && (this->outputDirectory.back() == '/' || this->outputDirectory.back() == '\\')) {
        this->outputDirectory.pop_back();
    }
}

bool ModelToWorldConverter::hasTriangles(const aiMesh *mesh) {
    for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        if (mesh->mFaces[faceIndex].mNumIndices == 3) {
            return true;
        }
    }
    return false;
}

std::string ModelToWorldConverter::sanitizeForFileName(const std::string &name) {
    std::string sanitized;
    for (char character : name) {
        bool allowed = (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                       (character >= '0' && character <= '9') || character == '_' || character == '-';
        sanitized += allowed ? character : '_';
        if (sanitized.size() >= 64) {
            break;
        }
    }
    return sanitized;
}

//the mesh index alone keeps it unique, the node name is only there for whoever browses the folder
std::string ModelToWorldConverter::buildPieceFilePath(const PieceKey &key, const std::string &nodeName) const {
    std::string fileName = sourceStem + "_" + std::to_string(key.meshIndex);
    std::string readableName = sanitizeForFileName(nodeName);
    if (!readableName.empty()) {
        fileName += "_" + readableName;
    }
    if (key.mirrored) {
        fileName += "_mirror";
    }
    return outputDirectory + "/" + fileName + ".limonmodel";
}

//false when the matrix has shear, which Transformation can't hold. A non uniform scale above a rotated child composes into it
bool ModelToWorldConverter::decomposeWithoutShear(const glm::mat4 &placement, glm::vec3 &translate, glm::quat &orientation, glm::vec3 &scale) {
    glm::vec3 skew;
    glm::vec4 perspective;
    if (!glm::decompose(placement, scale, orientation, translate, skew, perspective)) {
        return false;
    }
    glm::mat4 recomposed = glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation) *
                           glm::scale(glm::mat4(1.0f), scale);
    float largestElement = 0.0f;
    float largestError = 0.0f;
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            largestElement = std::max(largestElement, std::fabs(placement[column][row]));
            largestError = std::max(largestError, std::fabs(placement[column][row] - recomposed[column][row]));
        }
    }
    return largestError <= 1e-4f * std::max(largestElement, 1e-6f);
}

void ModelToWorldConverter::indexNodes(const aiNode *node, const aiNode *parent) {
    nodesByName[node->mName.C_Str()].push_back(node);
    parentNodes[node] = parent;
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        indexNodes(node->mChildren[i], node);
    }
}

void ModelToWorldConverter::collectMeshReferences(const aiScene *scene, const aiNode *node,
                                                  std::vector<std::pair<const aiNode *, uint32_t>> &meshReferences) const {
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        if (hasTriangles(scene->mMeshes[node->mMeshes[i]])) {
            meshReferences.push_back(std::make_pair(node, node->mMeshes[i]));
        }
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        collectMeshReferences(scene, node->mChildren[i], meshReferences);
    }
}

bool ModelToWorldConverter::isSelfOrDescendantOf(const aiNode *node, const std::set<const aiNode *> &ancestors) const {
    for (const aiNode *current = node; current != nullptr; current = parentNodes.at(current)) {
        if (ancestors.count(current) != 0) {
            return true;
        }
    }
    return false;
}


//the closest node with that name, walking out from the mesh. A global lookup would hand every car the first car's bones
const aiNode *ModelToWorldConverter::findBoneNode(const aiNode *meshNode, const std::string &boneName) const {
    std::map<std::string, std::vector<const aiNode *>>::const_iterator namedNodes = nodesByName.find(boneName);
    if (namedNodes == nodesByName.end()) {
        return nullptr;
    }
    if (namedNodes->second.size() == 1) {
        return namedNodes->second[0];
    }
    std::set<const aiNode *> meshAncestors;
    for (const aiNode *ancestor = meshNode; ancestor != nullptr; ancestor = parentNodes.at(ancestor)) {
        meshAncestors.insert(ancestor);
    }
    for (const aiNode *ancestor = meshNode; ancestor != nullptr; ancestor = parentNodes.at(ancestor)) {
        std::set<const aiNode *> searchRoot{ancestor};
        const aiNode *ancestorMatch = nullptr;
        for (const aiNode *candidate : namedNodes->second) {//depth first order, from indexNodes
            if (!isSelfOrDescendantOf(candidate, searchRoot)) {
                continue;
            }
            //Unreal names the actor like its root bone, and the actor is the mesh's parent, never what deforms it
            if (meshAncestors.count(candidate) == 0) {
                return candidate;
            }
            if (ancestorMatch == nullptr) {
                ancestorMatch = candidate;
            }
        }
        if (ancestorMatch != nullptr) {
            return ancestorMatch;
        }
    }
    return nullptr;
}

//exporters pad skins with zero weight entries for joints the mesh never uses, glTF lists every joint of the scene
bool ModelToWorldConverter::isBoneWeighted(const aiBone *bone) {
    for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
        if (bone->mWeights[weightIndex].mWeight > 0.0f) {
            return true;
        }
    }
    return false;
}

bool ModelToWorldConverter::collectBoneNodes(const aiMesh *mesh, const aiNode *meshNode, bool weightedOnly, std::set<const aiNode *> &boneNodes) const {
    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        if (weightedOnly && !isBoneWeighted(mesh->mBones[boneIndex])) {
            continue;
        }
        const aiNode *boneNode = findBoneNode(meshNode, mesh->mBones[boneIndex]->mName.C_Str());
        if (boneNode == nullptr) {
            return false;
        }
        boneNodes.insert(boneNode);
    }
    return true;
}

bool ModelToWorldConverter::isMeshMovedBy(const aiScene *scene, const aiNode *node, uint32_t meshIndex,
                                          const std::set<const aiNode *> &movingNodes) const {
    if (isSelfOrDescendantOf(node, movingNodes)) {
        return true;
    }
    const aiMesh *mesh = scene->mMeshes[meshIndex];
    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        if (!isBoneWeighted(mesh->mBones[boneIndex])) {
            continue;//moving it moves nothing of this mesh
        }
        const aiNode *boneNode = findBoneNode(node, mesh->mBones[boneIndex]->mName.C_Str());
        if (boneNode != nullptr && isSelfOrDescendantOf(boneNode, movingNodes)) {
            return true;
        }
    }
    return false;
}

//bone names and offsets are left out, offsets carry each copy's placement and names repeat anyway
uint64_t ModelToWorldConverter::hashSkinnedMesh(const aiMesh *mesh) {
    uint64_t hash = 1469598103934665603ull;
    std::vector<uint32_t> words;
    words.push_back(mesh->mMaterialIndex);
    words.push_back(mesh->mNumVertices);
    words.push_back(mesh->mNumFaces);
    words.push_back(mesh->mNumBones);
    for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
        uint32_t coordinateBits[3];
        memcpy(coordinateBits, &mesh->mVertices[vertexIndex], sizeof(coordinateBits));
        words.insert(words.end(), coordinateBits, coordinateBits + 3);
    }
    for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
        words.insert(words.end(), mesh->mFaces[faceIndex].mIndices, mesh->mFaces[faceIndex].mIndices + mesh->mFaces[faceIndex].mNumIndices);
    }
    for (uint32_t word : words) {
        hash = (hash ^ word) * 1099511628211ull;
    }
    return hash;
}

bool ModelToWorldConverter::areSkinnedMeshesEqual(const aiMesh *first, const aiMesh *second) {
    if (first->mMaterialIndex != second->mMaterialIndex || first->mNumVertices != second->mNumVertices ||
        first->mNumFaces != second->mNumFaces || first->mNumBones != second->mNumBones ||
        first->HasNormals() != second->HasNormals() || first->HasTextureCoords(0) != second->HasTextureCoords(0)) {
        return false;
    }
    if (memcmp(first->mVertices, second->mVertices, sizeof(aiVector3D) * first->mNumVertices) != 0) {
        return false;
    }
    if (first->HasNormals() && memcmp(first->mNormals, second->mNormals, sizeof(aiVector3D) * first->mNumVertices) != 0) {
        return false;
    }
    if (first->HasTextureCoords(0) &&
        memcmp(first->mTextureCoords[0], second->mTextureCoords[0], sizeof(aiVector3D) * first->mNumVertices) != 0) {
        return false;
    }
    for (uint32_t faceIndex = 0; faceIndex < first->mNumFaces; ++faceIndex) {
        const aiFace &firstFace = first->mFaces[faceIndex];
        const aiFace &secondFace = second->mFaces[faceIndex];
        if (firstFace.mNumIndices != secondFace.mNumIndices ||
            memcmp(firstFace.mIndices, secondFace.mIndices, sizeof(uint32_t) * firstFace.mNumIndices) != 0) {
            return false;
        }
    }
    for (uint32_t boneIndex = 0; boneIndex < first->mNumBones; ++boneIndex) {
        const aiBone *firstBone = first->mBones[boneIndex];
        const aiBone *secondBone = second->mBones[boneIndex];
        if (firstBone->mNumWeights != secondBone->mNumWeights ||
            memcmp(firstBone->mWeights, secondBone->mWeights, sizeof(aiVertexWeight) * firstBone->mNumWeights) != 0) {
            return false;
        }
    }
    return true;
}

//assimp only merges meshes that are equal bone names included, and every copy of a skinned prefab binds its own bones
void ModelToWorldConverter::findSkinnedDuplicates(const aiScene *scene) {
    canonicalMeshIndices.resize(scene->mNumMeshes);
    std::map<uint64_t, std::vector<uint32_t>> canonicalsByHash;
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        canonicalMeshIndices[meshIndex] = meshIndex;
        const aiMesh *mesh = scene->mMeshes[meshIndex];
        if (!mesh->HasBones()) {
            continue;
        }
        std::vector<uint32_t> &candidates = canonicalsByHash[hashSkinnedMesh(mesh)];
        for (uint32_t candidate : candidates) {
            if (areSkinnedMeshesEqual(scene->mMeshes[candidate], mesh)) {
                canonicalMeshIndices[meshIndex] = candidate;
                break;
            }
        }
        if (canonicalMeshIndices[meshIndex] == meshIndex) {
            candidates.push_back(meshIndex);
        }
    }
}

const aiNode *ModelToWorldConverter::findLowestCommonAncestor(const std::set<const aiNode *> &nodes) const {
    std::vector<const aiNode *> commonPath;//from the scene root down
    bool firstNode = true;
    for (const aiNode *node : nodes) {
        std::vector<const aiNode *> path;
        for (const aiNode *current = node; current != nullptr; current = parentNodes.at(current)) {
            path.insert(path.begin(), current);
        }
        if (firstNode) {
            commonPath = path;
            firstNode = false;
            continue;
        }
        size_t sharedLength = 0;
        while (sharedLength < commonPath.size() && sharedLength < path.size() && commonPath[sharedLength] == path[sharedLength]) {
            ++sharedLength;
        }
        commonPath.resize(sharedLength);
    }
    return commonPath.empty() ? nullptr : commonPath.back();
}

void ModelToWorldConverter::addPathToAncestor(const aiNode *node, const aiNode *ancestor, std::set<const aiNode *> &pathNodes) const {
    for (const aiNode *current = node; current != nullptr; current = parentNodes.at(current)) {
        pathNodes.insert(current);
        if (current == ancestor) {
            return;
        }
    }
}

//includes the node's own transform
glm::mat4 ModelToWorldConverter::buildGlobalTransform(const aiNode *node, const glm::mat4 &rootTransform) const {
    glm::mat4 globalTransform(1.0f);
    for (const aiNode *current = node; current != nullptr; current = parentNodes.at(current)) {
        globalTransform = GLMConverter::AssimpToGLM(current->mTransformation) * globalTransform;
    }
    return rootTransform * globalTransform;
}

//compared to the node's rest transform, not to the other keys: a constant key away from rest still moves the node
bool ModelToWorldConverter::channelMoves(const aiNodeAnim *channel, const aiNode *node) {
    aiVector3D restScale, restPosition;
    aiQuaternion restRotation;
    node->mTransformation.Decompose(restScale, restRotation, restPosition);
    float positionTolerance = CHANNEL_REST_EPSILON * std::max(1.0f, restPosition.Length());
    for (uint32_t keyIndex = 0; keyIndex < channel->mNumPositionKeys; ++keyIndex) {
        if ((channel->mPositionKeys[keyIndex].mValue - restPosition).Length() > positionTolerance) {
            return true;
        }
    }
    for (uint32_t keyIndex = 0; keyIndex < channel->mNumRotationKeys; ++keyIndex) {
        const aiQuaternion &rotation = channel->mRotationKeys[keyIndex].mValue;
        float alignment = std::fabs(rotation.x * restRotation.x + rotation.y * restRotation.y +
                                    rotation.z * restRotation.z + rotation.w * restRotation.w);//q and -q are the same rotation
        if (1.0f - alignment > CHANNEL_REST_EPSILON) {
            return true;
        }
    }
    float scaleTolerance = CHANNEL_REST_EPSILON * std::max(1.0f, restScale.Length());
    for (uint32_t keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex) {
        if ((channel->mScalingKeys[keyIndex].mValue - restScale).Length() > scaleTolerance) {
            return true;
        }
    }
    return false;
}

//same order ModelAsset::loadNodeTree hands out boneIDs, so a position here is the boneID the group asset will have
void ModelToWorldConverter::orderTreeNodes(const aiNode *node, const std::set<const aiNode *> &treeNodes,
                                           std::vector<const aiNode *> &orderedNodes) const {
    orderedNodes.push_back(node);
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        if (treeNodes.count(node->mChildren[i]) != 0) {
            orderTreeNodes(node->mChildren[i], treeNodes, orderedNodes);
        }
    }
}

bool ModelToWorldConverter::areMatricesClose(const glm::mat4 &first, const glm::mat4 &second) {
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float tolerance = 1e-4f * std::max(1.0f, std::max(std::fabs(first[column][row]), std::fabs(second[column][row])));
            if (std::fabs(first[column][row] - second[column][row]) > tolerance) {
                return false;
            }
        }
    }
    return true;
}

//the same test ModelAsset::rebindToRestPose makes: every weighted bone off its node by one shared rigid transform
bool ModelToWorldConverter::isSkinReboundToRest(const aiMesh *mesh, const aiNode *meshNode) const {
    glm::mat4 inverseMeshGlobal = glm::inverse(buildGlobalTransform(meshNode, glm::mat4(1.0f)));
    bool anyWeighted = false;
    glm::mat4 firstMismatch(1.0f);
    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        if (!isBoneWeighted(mesh->mBones[boneIndex])) {
            continue;
        }
        const aiNode *boneNode = findBoneNode(meshNode, mesh->mBones[boneIndex]->mName.C_Str());
        glm::mat4 mismatch = inverseMeshGlobal * buildGlobalTransform(boneNode, glm::mat4(1.0f)) *
                             GLMConverter::AssimpToGLM(mesh->mBones[boneIndex]->mOffsetMatrix);
        if (!anyWeighted) {
            firstMismatch = mismatch;
            anyWeighted = true;
        } else if (!areMatricesClose(firstMismatch, mismatch)) {
            return false;
        }
    }
    return true;
}

//copies of one prefab: same tree below the root, same meshes and skin at the same positions. Names and the root's own
//transform may differ, the root is the object's transform and channels are renamed by position
bool ModelToWorldConverter::areGroupsEquivalent(const aiScene *scene, const AnimationGroup &first, const AnimationGroup &second) const {
    if (first.orderedNodes.size() != second.orderedNodes.size() || first.meshReferences.size() != second.meshReferences.size()) {
        return false;
    }
    std::map<const aiNode *, size_t> firstPositions;
    std::map<const aiNode *, size_t> secondPositions;
    for (size_t position = 0; position < first.orderedNodes.size(); ++position) {
        firstPositions[first.orderedNodes[position]] = position;
        secondPositions[second.orderedNodes[position]] = position;
    }
    for (size_t position = 0; position < first.orderedNodes.size(); ++position) {
        const aiNode *firstNode = first.orderedNodes[position];
        const aiNode *secondNode = second.orderedNodes[position];
        //a parent at the same position with different child counts would be a different shape in the same order
        size_t firstChildCount = 0;
        size_t secondChildCount = 0;
        for (uint32_t i = 0; i < firstNode->mNumChildren; ++i) {
            firstChildCount += first.treeNodes.count(firstNode->mChildren[i]);
        }
        for (uint32_t i = 0; i < secondNode->mNumChildren; ++i) {
            secondChildCount += second.treeNodes.count(secondNode->mChildren[i]);
        }
        if (firstChildCount != secondChildCount) {
            return false;
        }
        if (position != 0 && !areMatricesClose(GLMConverter::AssimpToGLM(firstNode->mTransformation),
                                               GLMConverter::AssimpToGLM(secondNode->mTransformation))) {
            return false;
        }
        std::vector<uint32_t> firstMeshes;
        std::vector<uint32_t> secondMeshes;
        for (uint32_t i = 0; i < firstNode->mNumMeshes; ++i) {
            if (first.meshReferences.count(std::make_pair(firstNode, firstNode->mMeshes[i])) != 0) {
                firstMeshes.push_back(firstNode->mMeshes[i]);
            }
        }
        for (uint32_t i = 0; i < secondNode->mNumMeshes; ++i) {
            if (second.meshReferences.count(std::make_pair(secondNode, secondNode->mMeshes[i])) != 0) {
                secondMeshes.push_back(secondNode->mMeshes[i]);
            }
        }
        if (firstMeshes.size() != secondMeshes.size()) {
            return false;
        }
        for (size_t meshPosition = 0; meshPosition < firstMeshes.size(); ++meshPosition) {
            if (canonicalMeshIndices[firstMeshes[meshPosition]] != canonicalMeshIndices[secondMeshes[meshPosition]]) {
                return false;
            }
            //equal canonical indices mean equal geometry, weights and bone order, the bones still have to sit alike
            const aiMesh *firstMesh = scene->mMeshes[firstMeshes[meshPosition]];
            const aiMesh *secondMesh = scene->mMeshes[secondMeshes[meshPosition]];
            for (uint32_t boneIndex = 0; boneIndex < firstMesh->mNumBones; ++boneIndex) {
                if (!isBoneWeighted(firstMesh->mBones[boneIndex])) {
                    continue;
                }
                const aiNode *firstBone = findBoneNode(firstNode, firstMesh->mBones[boneIndex]->mName.C_Str());
                const aiNode *secondBone = findBoneNode(secondNode, secondMesh->mBones[boneIndex]->mName.C_Str());
                if (firstPositions.at(firstBone) != secondPositions.at(secondBone)) {
                    return false;
                }
            }
            if (firstMesh->mNumBones == 0) {
                continue;
            }
            bool firstRebound = isSkinReboundToRest(firstMesh, firstNode);
            if (firstRebound != isSkinReboundToRest(secondMesh, secondNode)) {
                return false;
            }
            if (!firstRebound) {
                //a real bind pose is kept as stored, so the stored offsets have to agree
                for (uint32_t boneIndex = 0; boneIndex < firstMesh->mNumBones; ++boneIndex) {
                    if (!areMatricesClose(GLMConverter::AssimpToGLM(firstMesh->mBones[boneIndex]->mOffsetMatrix),
                                          GLMConverter::AssimpToGLM(secondMesh->mBones[boneIndex]->mOffsetMatrix))) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void ModelToWorldConverter::findSharedGroups(const aiScene *scene) {
    for (uint32_t groupIndex = 0; groupIndex < animationGroups.size(); ++groupIndex) {
        AnimationGroup &group = animationGroups[groupIndex];
        if (group.filePath.empty()) {
            continue;
        }
        for (uint32_t candidateIndex = 0; candidateIndex < groupIndex; ++candidateIndex) {
            const AnimationGroup &candidate = animationGroups[candidateIndex];
            if (candidate.filePath.empty() || candidate.sharedGroup != -1) {
                continue;
            }
            if (areGroupsEquivalent(scene, candidate, group)) {
                group.sharedGroup = (int32_t) candidateIndex;
                group.filePath = candidate.filePath;
                break;
            }
        }
    }
}

//keys of the root are made relative to that copy's own rest, which its object transform already holds
aiNodeAnim *ModelToWorldConverter::copyChannel(const aiNodeAnim *channel, const aiString &nodeName, const aiNode *rebaseRoot) {
    aiVector3D restScale(1.0f, 1.0f, 1.0f), restPosition(0.0f, 0.0f, 0.0f);
    aiQuaternion restRotation;
    if (rebaseRoot != nullptr) {
        rebaseRoot->mTransformation.Decompose(restScale, restRotation, restPosition);
    }
    aiQuaternion inverseRestRotation = restRotation;
    inverseRestRotation.Conjugate();
    aiMatrix3x3 inverseRestRotationMatrix = inverseRestRotation.GetMatrix();
    float uniformScale = restScale.x;//prepareAnimationGroup refuses a keyed root with a non uniform one

    aiNodeAnim *copy = new aiNodeAnim();
    copy->mNodeName = nodeName;
    copy->mPreState = channel->mPreState;
    copy->mPostState = channel->mPostState;
    copy->mNumPositionKeys = channel->mNumPositionKeys;
    copy->mPositionKeys = new aiVectorKey[channel->mNumPositionKeys];
    for (uint32_t keyIndex = 0; keyIndex < channel->mNumPositionKeys; ++keyIndex) {
        copy->mPositionKeys[keyIndex] = channel->mPositionKeys[keyIndex];
        if (rebaseRoot != nullptr) {
            copy->mPositionKeys[keyIndex].mValue = (inverseRestRotationMatrix * (channel->mPositionKeys[keyIndex].mValue - restPosition)) / uniformScale;
        }
    }
    copy->mNumRotationKeys = channel->mNumRotationKeys;
    copy->mRotationKeys = new aiQuatKey[channel->mNumRotationKeys];
    for (uint32_t keyIndex = 0; keyIndex < channel->mNumRotationKeys; ++keyIndex) {
        copy->mRotationKeys[keyIndex] = channel->mRotationKeys[keyIndex];
        if (rebaseRoot != nullptr) {
            copy->mRotationKeys[keyIndex].mValue = inverseRestRotation * channel->mRotationKeys[keyIndex].mValue;
        }
    }
    copy->mNumScalingKeys = channel->mNumScalingKeys;
    copy->mScalingKeys = new aiVectorKey[channel->mNumScalingKeys];
    for (uint32_t keyIndex = 0; keyIndex < channel->mNumScalingKeys; ++keyIndex) {
        copy->mScalingKeys[keyIndex] = channel->mScalingKeys[keyIndex];
        if (rebaseRoot != nullptr) {
            copy->mScalingKeys[keyIndex].mValue = channel->mScalingKeys[keyIndex].mValue / uniformScale;
        }
    }
    return copy;
}

bool ModelToWorldConverter::areChannelsEqual(const aiNodeAnim *first, const aiNodeAnim *second) {
    if (first->mNumPositionKeys != second->mNumPositionKeys || first->mNumRotationKeys != second->mNumRotationKeys ||
        first->mNumScalingKeys != second->mNumScalingKeys) {
        return false;
    }
    const float tolerance = 1e-4f;
    for (uint32_t keyIndex = 0; keyIndex < first->mNumPositionKeys; ++keyIndex) {
        if (first->mPositionKeys[keyIndex].mTime != second->mPositionKeys[keyIndex].mTime ||
            (first->mPositionKeys[keyIndex].mValue - second->mPositionKeys[keyIndex].mValue).Length() >
            tolerance * std::max(1.0f, first->mPositionKeys[keyIndex].mValue.Length())) {
            return false;
        }
    }
    for (uint32_t keyIndex = 0; keyIndex < first->mNumRotationKeys; ++keyIndex) {
        const aiQuaternion &firstRotation = first->mRotationKeys[keyIndex].mValue;
        const aiQuaternion &secondRotation = second->mRotationKeys[keyIndex].mValue;
        float alignment = std::fabs(firstRotation.x * secondRotation.x + firstRotation.y * secondRotation.y +
                                    firstRotation.z * secondRotation.z + firstRotation.w * secondRotation.w);
        if (first->mRotationKeys[keyIndex].mTime != second->mRotationKeys[keyIndex].mTime || 1.0f - alignment > tolerance) {
            return false;
        }
    }
    for (uint32_t keyIndex = 0; keyIndex < first->mNumScalingKeys; ++keyIndex) {
        if (first->mScalingKeys[keyIndex].mTime != second->mScalingKeys[keyIndex].mTime ||
            (first->mScalingKeys[keyIndex].mValue - second->mScalingKeys[keyIndex].mValue).Length() >
            tolerance * std::max(1.0f, first->mScalingKeys[keyIndex].mValue.Length())) {
            return false;
        }
    }
    return true;
}

//both already renamed to the shared tree, so channels are matched by node name
bool ModelToWorldConverter::areClipsEqual(const aiAnimation *first, const aiAnimation *second) {
    if (first->mDuration != second->mDuration || first->mTicksPerSecond != second->mTicksPerSecond ||
        first->mNumChannels != second->mNumChannels) {
        return false;
    }
    for (uint32_t firstIndex = 0; firstIndex < first->mNumChannels; ++firstIndex) {
        bool matched = false;
        for (uint32_t secondIndex = 0; secondIndex < second->mNumChannels && !matched; ++secondIndex) {
            matched = first->mChannels[firstIndex]->mNodeName == second->mChannels[secondIndex]->mNodeName &&
                      areChannelsEqual(first->mChannels[firstIndex], second->mChannels[secondIndex]);
        }
        if (!matched) {
            return false;
        }
    }
    return true;
}

//every copy's clips renamed to the shared tree. One clip per distinct motion, under its source name unless copies
//disagree about that name, then every variant is filed as "<copy root>|<name>"
void ModelToWorldConverter::buildGroupClips(const aiScene *scene) {
    groupClips.clear();
    groupClips.resize(animationGroups.size());
    std::vector<std::vector<ClipCandidate>> distinctClips(animationGroups.size());
    for (uint32_t groupIndex = 0; groupIndex < animationGroups.size(); ++groupIndex) {
        const AnimationGroup &group = animationGroups[groupIndex];
        if (group.filePath.empty()) {
            continue;
        }
        uint32_t fileGroupIndex = group.sharedGroup == -1 ? groupIndex : (uint32_t) group.sharedGroup;
        const AnimationGroup &fileGroup = animationGroups[fileGroupIndex];
        std::map<std::string, size_t> positionsByName;//the first node of a name, the engine plays a channel by name too
        for (size_t position = group.orderedNodes.size(); position > 0; --position) {
            positionsByName[group.orderedNodes[position - 1]->mName.C_Str()] = position - 1;
        }
        for (uint32_t animationIndex : group.animationIndices) {
            const aiAnimation *source = scene->mAnimations[animationIndex];
            std::vector<aiNodeAnim *> channels;
            for (uint32_t channelIndex = 0; channelIndex < source->mNumChannels; ++channelIndex) {
                std::map<std::string, size_t>::const_iterator position = positionsByName.find(source->mChannels[channelIndex]->mNodeName.C_Str());
                if (position == positionsByName.end()) {
                    continue;//not this group's node, a clip only ever moves one group
                }
                channels.push_back(copyChannel(source->mChannels[channelIndex], fileGroup.orderedNodes[position->second]->mName,
                                               position->second == 0 ? group.root : nullptr));
            }
            std::unique_ptr<aiAnimation> clip = std::make_unique<aiAnimation>();
            clip->mName = source->mName;
            clip->mDuration = source->mDuration;
            clip->mTicksPerSecond = source->mTicksPerSecond;
            clip->mNumChannels = (uint32_t) channels.size();
            clip->mChannels = new aiNodeAnim *[channels.size()];
            for (size_t channelIndex = 0; channelIndex < channels.size(); ++channelIndex) {
                clip->mChannels[channelIndex] = channels[channelIndex];
            }
            bool alreadyStored = false;
            for (const ClipCandidate &stored : distinctClips[fileGroupIndex]) {
                if (stored.clip->mName == clip->mName && areClipsEqual(stored.clip.get(), clip.get())) {
                    alreadyStored = true;
                    break;
                }
            }
            if (!alreadyStored) {
                ClipCandidate candidate;
                candidate.clip = std::move(clip);
                candidate.copyRootName = group.root->mName.C_Str();
                distinctClips[fileGroupIndex].push_back(std::move(candidate));
            }
        }
    }

    for (uint32_t fileGroupIndex = 0; fileGroupIndex < distinctClips.size(); ++fileGroupIndex) {
        std::map<std::string, uint32_t> variantCounts;
        for (const ClipCandidate &candidate : distinctClips[fileGroupIndex]) {
            variantCounts[candidate.clip->mName.C_Str()]++;
        }
        for (ClipCandidate &candidate : distinctClips[fileGroupIndex]) {
            if (variantCounts[candidate.clip->mName.C_Str()] > 1) {
                candidate.clip->mName = aiString(candidate.copyRootName + "|" + candidate.clip->mName.C_Str());
            }
            groupClips[fileGroupIndex].push_back(std::move(candidate.clip));
        }
    }
}

size_t ModelToWorldConverter::findUnionRoot(std::vector<size_t> &unionParents, size_t index) {
    while (unionParents[index] != index) {
        unionParents[index] = unionParents[unionParents[index]];
        index = unionParents[index];
    }
    return index;
}

void ModelToWorldConverter::findAnimationGroups(const aiScene *scene) {
    movingNodesPerAnimation.assign(scene->mNumAnimations, std::set<const aiNode *>());
    for (uint32_t animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
        const aiAnimation *animation = scene->mAnimations[animationIndex];
        for (uint32_t channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex) {
            const aiNodeAnim *channel = animation->mChannels[channelIndex];
            //the engine plays a channel on every node of its name, so a repeated name moves all of them
            std::map<std::string, std::vector<const aiNode *>>::const_iterator targetNodes = nodesByName.find(channel->mNodeName.C_Str());
            if (targetNodes == nodesByName.end()) {
                continue;
            }
            for (const aiNode *targetNode : targetNodes->second) {
                if (channelMoves(channel, targetNode)) {
                    movingNodesPerAnimation[animationIndex].insert(targetNode);
                }
            }
        }
    }

    std::vector<std::pair<const aiNode *, uint32_t>> meshReferences;
    collectMeshReferences(scene, scene->mRootNode, meshReferences);
    std::vector<size_t> unionParents(meshReferences.size());
    for (size_t referenceIndex = 0; referenceIndex < meshReferences.size(); ++referenceIndex) {
        unionParents[referenceIndex] = referenceIndex;
    }
    std::vector<std::set<uint32_t>> animationsOfReference(meshReferences.size());
    for (uint32_t animationIndex = 0; animationIndex < scene->mNumAnimations; ++animationIndex) {
        if (movingNodesPerAnimation[animationIndex].empty()) {
            continue;//cameras, lights, or keys that never leave rest
        }
        bool anyMoved = false;
        size_t firstMovedReference = 0;
        for (size_t referenceIndex = 0; referenceIndex < meshReferences.size(); ++referenceIndex) {
            if (!isMeshMovedBy(scene, meshReferences[referenceIndex].first, meshReferences[referenceIndex].second,
                               movingNodesPerAnimation[animationIndex])) {
                continue;
            }
            animationsOfReference[referenceIndex].insert(animationIndex);
            if (!anyMoved) {
                anyMoved = true;
                firstMovedReference = referenceIndex;
            } else {
                unionParents[findUnionRoot(unionParents, referenceIndex)] = findUnionRoot(unionParents, firstMovedReference);
            }
        }
    }

    std::map<size_t, uint32_t> groupOfUnionRoot;
    for (size_t referenceIndex = 0; referenceIndex < meshReferences.size(); ++referenceIndex) {
        if (animationsOfReference[referenceIndex].empty()) {
            continue;
        }
        size_t unionRoot = findUnionRoot(unionParents, referenceIndex);
        if (groupOfUnionRoot.find(unionRoot) == groupOfUnionRoot.end()) {
            groupOfUnionRoot[unionRoot] = (uint32_t) animationGroups.size();
            animationGroups.emplace_back();
        }
        AnimationGroup &group = animationGroups[groupOfUnionRoot[unionRoot]];
        group.meshReferences.insert(meshReferences[referenceIndex]);
        for (uint32_t animationIndex : animationsOfReference[referenceIndex]) {
            if (std::find(group.animationIndices.begin(), group.animationIndices.end(), animationIndex) == group.animationIndices.end()) {
                group.animationIndices.push_back(animationIndex);
            }
        }
        animatedMeshReferences.insert(meshReferences[referenceIndex]);
    }
}

//false drops the group with an error, the rest of the map still converts
bool ModelToWorldConverter::prepareAnimationGroup(const aiScene *scene, uint32_t groupIndex, const glm::mat4 &rootTransform) {
    AnimationGroup &group = animationGroups[groupIndex];
    std::set<const aiNode *> neededNodes;
    for (const std::pair<const aiNode *, uint32_t> &meshReference : group.meshReferences) {
        neededNodes.insert(meshReference.first);
        if (!collectBoneNodes(scene->mMeshes[meshReference.second], meshReference.first, true, neededNodes)) {
            std::cerr << "Animation group " << groupIndex << " of " << sourcePath << " dropped, a bone of mesh "
                      << meshReference.second << " is not in the node tree." << std::endl;
            return false;
        }
    }
    //an animated ancestor has to be in the tree, or what it does is lost with the static chain above the root
    std::set<const aiNode *> groupMovingNodes;
    for (uint32_t animationIndex : group.animationIndices) {
        groupMovingNodes.insert(movingNodesPerAnimation[animationIndex].begin(), movingNodesPerAnimation[animationIndex].end());
    }
    std::set<const aiNode *> animatedAncestors;
    for (const aiNode *neededNode : neededNodes) {
        for (const aiNode *current = neededNode; current != nullptr; current = parentNodes.at(current)) {
            if (groupMovingNodes.count(current) != 0) {
                animatedAncestors.insert(current);
            }
        }
    }
    neededNodes.insert(animatedAncestors.begin(), animatedAncestors.end());

    group.root = findLowestCommonAncestor(neededNodes);
    for (const aiNode *neededNode : neededNodes) {
        addPathToAncestor(neededNode, group.root, group.treeNodes);
    }
    //every tree node gets an ID, but only the ones skinning reads must fit the rig: weighted bones, and the node of a
    //mesh without bones. The rest are only walked
    orderTreeNodes(group.root, group.treeNodes, group.orderedNodes);
    std::map<const aiNode *, uint32_t> rigIndices;
    for (uint32_t position = 0; position < group.orderedNodes.size(); ++position) {
        rigIndices[group.orderedNodes[position]] = position;
    }
    std::set<const aiNode *> skinningNodes;
    for (const std::pair<const aiNode *, uint32_t> &meshReference : group.meshReferences) {
        const aiMesh *mesh = scene->mMeshes[meshReference.second];
        if (mesh->mNumBones == 0) {
            skinningNodes.insert(meshReference.first);
            continue;
        }
        for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            if (isBoneWeighted(mesh->mBones[boneIndex])) {
                skinningNodes.insert(findBoneNode(meshReference.first, mesh->mBones[boneIndex]->mName.C_Str()));
            }
        }
    }
    uint32_t highestSkinningIndex = 0;
    for (const aiNode *skinningNode : skinningNodes) {
        highestSkinningIndex = std::max(highestSkinningIndex, rigIndices.at(skinningNode));
    }
    if (highestSkinningIndex >= NR_BONE) {
        std::cerr << "Animation group " << groupIndex << " of " << sourcePath << " rooted at " << group.root->mName.C_Str()
                  << " dropped, a weighted bone gets rig index " << highestSkinningIndex << " but a rig holds " << NR_BONE << "." << std::endl;
        return false;
    }
    //inside one rig a bone name means the last node of that name in rig order (MeshAsset::fillBoneMap overwrites), so
    //a shared name is only fatal when the skinning node isn't that last one
    std::map<std::string, const aiNode *> lastNodeByName;
    for (const aiNode *orderedNode : group.orderedNodes) {
        lastNodeByName[orderedNode->mName.C_Str()] = orderedNode;
    }
    for (const aiNode *skinningNode : skinningNodes) {
        if (lastNodeByName.at(skinningNode->mName.C_Str()) != skinningNode) {
            std::cerr << "Animation group " << groupIndex << " of " << sourcePath << " rooted at " << group.root->mName.C_Str()
                      << " dropped, a later node of its rig is also named " << skinningNode->mName.C_Str() << "." << std::endl;
            return false;
        }
    }

    //root keys get rebased with one scale factor, see copyChannel
    bool rootKeyed = false;
    for (uint32_t animationIndex : group.animationIndices) {
        const aiAnimation *animation = scene->mAnimations[animationIndex];
        for (uint32_t channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex) {
            if (animation->mChannels[channelIndex]->mNodeName == group.root->mName) {
                rootKeyed = true;
            }
        }
    }
    if (rootKeyed) {
        aiVector3D rootScale, rootPosition;
        aiQuaternion rootRotation;
        group.root->mTransformation.Decompose(rootScale, rootRotation, rootPosition);
        float largestScale = std::max(rootScale.x, std::max(rootScale.y, rootScale.z));
        float smallestScale = std::min(rootScale.x, std::min(rootScale.y, rootScale.z));
        if (largestScale - smallestScale > 1e-4f * largestScale) {
            std::cerr << "Animation group " << groupIndex << " of " << sourcePath << " rooted at " << group.root->mName.C_Str()
                      << " dropped, its root is animated and has a non uniform rest scale." << std::endl;
            return false;
        }
    }

    glm::mat4 placement = buildGlobalTransform(group.root, rootTransform);//the root's own rest included, the asset's tree starts at identity
    if (glm::determinant(glm::mat3(placement)) < 0.0f) {
        std::cerr << "Animation group " << groupIndex << " of " << sourcePath << " rooted at " << group.root->mName.C_Str()
                  << " dropped, it is mirrored and animated models can't be flipped." << std::endl;
        return false;
    }
    if (!decomposeWithoutShear(placement, group.translate, group.orientation, group.scale)) {
        std::cerr << "Animation group " << groupIndex << " of " << sourcePath << " rooted at " << group.root->mName.C_Str()
                  << " dropped, its placement has shear." << std::endl;
        return false;
    }
    std::string readableName = sanitizeForFileName(group.root->mName.C_Str());
    group.filePath = outputDirectory + "/" + sourceStem + "_animated" + std::to_string(groupIndex) +
                     (readableName.empty() ? "" : "_" + readableName) + ".limonmodel";
    return true;
}

void ModelToWorldConverter::collectInstances(const aiScene *scene, const aiNode *node, const glm::mat4 &parentTransform,
                                             const std::string &parentPath) {
    glm::mat4 nodeTransform = parentTransform * GLMConverter::AssimpToGLM(node->mTransformation);
    std::string nodeName = node->mName.C_Str();
    std::string nodePath = parentPath + "/" + nodeName;

    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        uint32_t meshIndex = node->mMeshes[i];
        const aiMesh *mesh = scene->mMeshes[meshIndex];
        if (!strncmp(nodeName.c_str(), "UCX_", strlen("UCX_"))) {
            rejectedNodes.push_back(nodePath + ": UCX_ physics proxies are not supported by the converter");
            continue;
        }
        if (!hasTriangles(mesh)) {
            continue;//the asset load skips these too
        }
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            glm::vec3 worldVertex = glm::vec3(nodeTransform * glm::vec4(GLMConverter::AssimpToGLM(mesh->mVertices[vertexIndex]), 1.0f));
            if (!anyVertexSeen || worldVertex.y > highestPoint.y) {
                highestPoint = worldVertex;
                anyVertexSeen = true;
            }
        }
        if (animatedMeshReferences.count(std::make_pair(node, meshIndex)) != 0) {
            continue;//its animation group places it
        }

        //mirrored pieces carry the flip in their vertices, so what is left for the object is a proper rotation
        bool mirrored = glm::determinant(glm::mat3(nodeTransform)) < 0.0f;
        glm::mat4 placement = mirrored ? nodeTransform * glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f)) : nodeTransform;

        glm::vec3 scale, translate;
        glm::quat orientation;
        if (!decomposeWithoutShear(placement, translate, orientation, scale)) {
            rejectedNodes.push_back(nodePath + ": transform has shear, which is not supported");
            continue;
        }

        PieceKey key{canonicalMeshIndices[meshIndex], mirrored};
        if (pieces.find(key) == pieces.end()) {
            Piece piece;
            piece.filePath = buildPieceFilePath(key, nodeName);
            piece.firstNodeName = nodeName;
            piece.sourceMeshIndex = meshIndex;
            if (mesh->HasBones()) {
                std::set<const aiNode *> boneNodes;
                if (!collectBoneNodes(mesh, node, false, boneNodes)) {//all of them, kept for retargeting
                    rejectedNodes.push_back(nodePath + ": a bone of mesh " + std::to_string(meshIndex) + " is not in the node tree");
                    continue;
                }
                piece.skeletonRoot = findLowestCommonAncestor(boneNodes);
                for (const aiNode *boneNode : boneNodes) {
                    addPathToAncestor(boneNode, piece.skeletonRoot, piece.skeletonNodes);
                }
                const aiNode *skeletonParent = parentNodes.at(piece.skeletonRoot);
                glm::mat4 skeletonParentGlobal = skeletonParent == nullptr ? rootTransform : buildGlobalTransform(skeletonParent, rootTransform);
                piece.skeletonPlacement = glm::inverse(placement) * skeletonParentGlobal;
            }
            pieces[key] = piece;
        }
        Instance instance;
        instance.key = key;
        instance.translate = translate;
        instance.orientation = orientation;
        instance.scale = scale;
        instances.push_back(instance);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        collectInstances(scene, node->mChildren[i], nodeTransform, nodePath);
    }
}

bool ModelToWorldConverter::convert() {
    std::cout << "Converting " << sourcePath << " into " << outputDirectory << " with scale " << scale << std::endl;
    if (!(scale > 0.0f)) {
        std::cerr << "Scale " << scale << " is not greater than 0, a negative one would mirror the whole map." << std::endl;
        return false;
    }
    Assimp::Importer importer;
    const aiScene *scene = ModelAsset::importScene(importer, sourcePath);
    if (scene == nullptr) {
        return false;
    }
    indexNodes(scene->mRootNode, nullptr);
    findSkinnedDuplicates(scene);
    findAnimationGroups(scene);

    //a parent of the whole model, so positions scale from the map origin and the pieces stay aligned
    rootTransform = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    uint32_t usableGroupCount = 0;
    for (uint32_t groupIndex = 0; groupIndex < animationGroups.size(); ++groupIndex) {
        if (prepareAnimationGroup(scene, groupIndex, rootTransform)) {
            ++usableGroupCount;
        }
    }
    findSharedGroups(scene);
    buildGroupClips(scene);
    collectInstances(scene, scene->mRootNode, rootTransform, "");
    if (!rejectedNodes.empty()) {
        std::cerr << "Conversion of " << sourcePath << " refused, " << rejectedNodes.size() << " nodes can't be converted:" << std::endl;
        for (const std::string &rejectedNode : rejectedNodes) {
            std::cerr << "    " << rejectedNode << std::endl;
        }
        return false;
    }
    if (instances.empty() && usableGroupCount == 0) {
        std::cerr << sourcePath << " has no mesh to convert." << std::endl;
        return false;
    }
    //one more for the light. An object at or above NR_MAX_MODELS gets no transform and silently doesn't render
    uint64_t lastID = FIRST_OBJECT_ID + instances.size() + usableGroupCount;
    if (lastID >= NR_MAX_MODELS) {
        std::cerr << sourcePath << " would need object IDs up to " << lastID << ", but NR_MAX_MODELS is " << NR_MAX_MODELS << "." << std::endl;
        return false;
    }

    std::error_code directoryError;
    std::filesystem::create_directories(outputDirectory, directoryError);
    if (directoryError) {
        std::cerr << "Can't create " << outputDirectory << ": " << directoryError.message() << std::endl;
        return false;
    }

    std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> embeddedTextures = ModelAsset::readEmbeddedTextures(scene);
    if (!embeddedTextures.empty()) {
        assetManager->addEmbeddedTextures(sourcePath, embeddedTextures);
    }

    uint32_t groupFileCount = 0;
    for (const AnimationGroup &group : animationGroups) {
        if (!group.filePath.empty() && group.sharedGroup == -1) {
            ++groupFileCount;
        }
    }
    std::cout << instances.size() << " static objects using " << pieces.size() << " pieces, " << usableGroupCount << " of "
              << animationGroups.size() << " animation groups using " << groupFileCount << " files" << std::endl;
    std::vector<std::map<PieceKey, Piece>::iterator> pieceQueue;
    for (std::map<PieceKey, Piece>::iterator pieceIterator = pieces.begin(); pieceIterator != pieces.end(); ++pieceIterator) {
        pieceQueue.push_back(pieceIterator);
    }
    //kept until the end, the pieces share a few textures and freeing the last user reloads them for the next piece
    std::vector<std::unique_ptr<ModelAsset>> writtenPieces(pieceQueue.size());
    std::atomic<size_t> nextPiece(0);
    std::atomic<size_t> finishedPieceCount(0);
    std::atomic<bool> anyPieceFailed(false);
    uint32_t workerCount = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> workers;
    for (uint32_t workerIndex = 0; workerIndex < workerCount; ++workerIndex) {
        workers.emplace_back(&ModelToWorldConverter::buildPieces, this, scene, std::cref(pieceQueue), std::ref(writtenPieces),
                             std::ref(nextPiece), std::ref(finishedPieceCount), std::ref(anyPieceFailed));
    }
    for (std::thread &worker : workers) {
        worker.join();
    }

    //few of these, so no workers
    for (uint32_t groupIndex = 0; groupIndex < animationGroups.size() && !anyPieceFailed; ++groupIndex) {
        const AnimationGroup &group = animationGroups[groupIndex];
        if (group.filePath.empty() || group.sharedGroup != -1) {
            continue;
        }
        std::vector<aiAnimation *> clips;
        for (const std::unique_ptr<aiAnimation> &clip : groupClips[groupIndex]) {
            clips.push_back(clip.get());
        }
        std::cout << "animation group " << groupIndex << ": " << group.meshReferences.size() << " meshes, " << group.treeNodes.size()
                  << " nodes, " << clips.size() << " animations: " << group.filePath << std::endl;
        std::unique_ptr<ModelAsset> groupAsset = ModelAssetBuilder::buildAnimationGroup(assetManager.get(), scene, group.root,
                                                                                        group.treeNodes, group.meshReferences, clips,
                                                                                        sourcePath, group.filePath);
        if (!groupAsset->writeBinary(group.filePath)) {
            anyPieceFailed = true;
        }
        writtenPieces.push_back(std::move(groupAsset));
    }

    //textures need the GL context, and freeing a half loaded one is reported as a bug. Only the first call per texture uploads
    for (const std::unique_ptr<ModelAsset> &writtenPiece : writtenPieces) {
        if (writtenPiece == nullptr) {
            continue;
        }
        for (const std::pair<const std::string, std::shared_ptr<Material>> &material : writtenPiece->getMaterialMap()) {
            material.second->loadGPUSide(assetManager.get());
        }
    }
    if (anyPieceFailed) {
        return false;
    }

    std::string worldPath = outputDirectory + "/" + sourceStem + ".xml";
    if (!writeWorld(worldPath)) {
        return false;
    }
    std::cout << "Conversion done, world written to " << worldPath << std::endl;
    return true;
}

//each worker writes only its own slot of builtPieces and its own map entry, so nothing here needs a lock
void ModelToWorldConverter::buildPieces(const aiScene *scene, const std::vector<std::map<PieceKey, Piece>::iterator> &pieceQueue,
                                        std::vector<std::unique_ptr<ModelAsset>> &builtPieces, std::atomic<size_t> &nextPiece,
                                        std::atomic<size_t> &finishedPieceCount, std::atomic<bool> &anyPieceFailed) {
    while (!anyPieceFailed) {
        size_t pieceIndex = nextPiece++;
        if (pieceIndex >= pieceQueue.size()) {
            return;
        }
        std::map<PieceKey, Piece>::iterator pieceIterator = pieceQueue[pieceIndex];
        std::unique_ptr<ModelAsset> pieceAsset = ModelAssetBuilder::buildPiece(assetManager.get(), scene, pieceIterator->second.sourceMeshIndex,
                                                                               pieceIterator->first.mirrored, sourcePath,
                                                                               pieceIterator->second.filePath,
                                                                               pieceIterator->second.firstNodeName,
                                                                               pieceIterator->second.skeletonRoot,
                                                                               pieceIterator->second.skeletonNodes,
                                                                               pieceIterator->second.skeletonPlacement);
        if (!pieceAsset->writeBinary(pieceIterator->second.filePath)) {
            anyPieceFailed = true;
        }
        pieceIterator->second.centerOffset = pieceAsset->getCenterOffset();
        builtPieces[pieceIndex] = std::move(pieceAsset);
        size_t finishedCount = ++finishedPieceCount;
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << "piece " << finishedCount << "/" << pieceQueue.size() << ": " << pieceIterator->second.filePath << std::endl;
    }
}

bool ModelToWorldConverter::writeWorld(const std::string &worldPath) const {
    tinyxml2::XMLDocument document;
    tinyxml2::XMLNode *worldNode = document.NewElement("World");
    document.InsertFirstChild(worldNode);
    XMLHelper::writeElement(document, worldNode, "Name", worldPath);
    XMLHelper::writeElement(document, worldNode, "SaveVersion", 2);

    World::PlayerInfo startingPlayer("Editor");
    startingPlayer.position = highestPoint + glm::vec3(0.0f, 2.0f, 0.0f);
    startingPlayer.serialize(document, worldNode);

    tinyxml2::XMLElement *objectsNode = document.NewElement("Objects");
    uint32_t objectID = FIRST_OBJECT_ID;
    for (const Instance &instance : instances) {
        const Piece &piece = pieces.at(instance.key);
        tinyxml2::XMLElement *objectNode = document.NewElement("Object");
        objectsNode->InsertEndChild(objectNode);
        XMLHelper::writeElement(document, objectNode, "File", piece.filePath);
        XMLHelper::writeElement(document, objectNode, "Disconnected", "False");
        XMLHelper::writeElement(document, objectNode, "Mass", 0.0f);
        XMLHelper::writeElement(document, objectNode, "ID", objectID);
        //the model renders around its own centerOffset, so the saved position has to put that back
        Transformation transformation;
        transformation.setScale(instance.scale);
        transformation.setOrientation(instance.orientation);
        transformation.setTranslate(instance.translate + instance.orientation * (instance.scale * piece.centerOffset));
        transformation.serialize(document, objectNode);
        ++objectID;
    }
    for (const AnimationGroup &group : animationGroups) {
        if (group.filePath.empty()) {
            continue;
        }
        tinyxml2::XMLElement *objectNode = document.NewElement("Object");
        objectsNode->InsertEndChild(objectNode);
        XMLHelper::writeElement(document, objectNode, "File", group.filePath);
        XMLHelper::writeElement(document, objectNode, "Disconnected", "False");
        XMLHelper::writeElement(document, objectNode, "Mass", 0.0f);
        XMLHelper::writeElement(document, objectNode, "ID", objectID);
        Transformation transformation;
        transformation.setScale(group.scale);
        transformation.setOrientation(group.orientation);
        transformation.setTranslate(group.translate);
        transformation.serialize(document, objectNode);
        ++objectID;
    }
    worldNode->InsertEndChild(objectsNode);

    //same sun as World001
    tinyxml2::XMLElement *lightsNode = document.NewElement("Lights");
    Light sun(assetManager->getGraphicsWrapper(), objectID, Light::LightTypes::DIRECTIONAL,
              glm::vec3(0.40824795f, -0.81649691f, 0.40824795f), glm::vec3(0.7f, 0.7f, 0.7f));
    sun.serialize(document, lightsNode);
    worldNode->InsertEndChild(lightsNode);

    tinyxml2::XMLError saveResult = document.SaveFile(worldPath.c_str());
    if (saveResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Writing " << worldPath << " failed with " << saveResult << std::endl;
        return false;
    }
    return true;
}
