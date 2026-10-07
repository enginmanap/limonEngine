//
// Created by engin on 07.10.2026.
//

#include "ModelAssetBuilder.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>

#include "ModelAsset.h"
#include "../Utils/GLMConverter.h"

std::unique_ptr<ModelAsset> ModelAssetBuilder::buildPiece(AssetManager *assetManager, const aiScene *scene, uint32_t meshIndex, bool mirrorX,
                                                          const std::string &sourcePath, const std::string &piecePath, const std::string &meshName,
                                                          const aiNode *skeletonRoot, const std::set<const aiNode *> &skeletonNodes,
                                                          const glm::mat4 &skeletonPlacement) {
    std::unique_ptr<ModelAsset> asset(new ModelAsset(assetManager, piecePath));
    asset->sourcePath = sourcePath;
    asset->reverseWinding = mirrorX;
    asset->hasAnimation = false;
    if (skeletonRoot != nullptr) {
        asset->rootNode = asset->loadNodeTree(skeletonRoot, &skeletonNodes);//MeshAsset looks its bones up by name in here
        asset->rootNode->transformation = skeletonPlacement * asset->rootNode->transformation;
    } else {
        asset->rootNode = std::make_shared<BoneNode>();
        asset->rootNode->name = meshName;
        asset->rootNode->boneID = asset->boneIDCounter++;
        asset->rootNode->transformation = glm::mat4(1.0f);
    }

    const aiMesh *sourceMesh = scene->mMeshes[meshIndex];
    std::shared_ptr<Material> meshMaterial = asset->loadMaterials(scene, sourceMesh->mMaterialIndex);
    //no node transform on purpose, every node using this mesh places it through its own object transform
    glm::mat4 mirrorTransform = mirrorX ? glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f)) : glm::mat4(1.0f);
    std::shared_ptr<MeshAsset> mesh = std::make_shared<MeshAsset>(sourceMesh, meshName, asset->rootNode, mirrorTransform, false, asset->reverseWinding);
    asset->meshMaterialMap[mesh] = meshMaterial;
    if ((*mesh->getTriangleCount()) == 0) {
        std::cerr << "Mesh " << meshIndex << " of " << sourcePath << " has no triangles, piece " << piecePath << " is empty." << std::endl;
        exit(-1);
    }
    if (meshMaterial->hasOpacityMap()) {
        asset->transparentMaterialUsed = true;
    }
    asset->meshes.push_back(mesh);
    if (skeletonRoot != nullptr) {
        //the piece's own space is the mesh frame, a mirrored piece's vertices are mirrored before the offsets see them
        std::map<std::string, glm::mat4> restGlobals;
        collectRestGlobals(asset->rootNode, glm::mat4(1.0f), restGlobals);
        rebindToRestPose(*asset, sourceMesh, glm::mat4(1.0f), mirrorTransform, restGlobals);
    }

    asset->buildLodLevels(LodLadder::BuildMode::NORMAL);
    asset->computeBoundsFromVertices();
    asset->buildPhysicsMeshes();
    return asset;
}

std::unique_ptr<ModelAsset> ModelAssetBuilder::buildAnimationGroup(AssetManager *assetManager, const aiScene *scene, const aiNode *groupRoot,
                                                                   const std::set<const aiNode *> &groupNodes,
                                                                   const std::set<std::pair<const aiNode *, uint32_t>> &groupMeshes,
                                                                   const std::vector<aiAnimation *> &groupClips,
                                                                   const std::string &sourcePath, const std::string &groupPath) {
    std::unique_ptr<ModelAsset> asset(new ModelAsset(assetManager, groupPath));
    asset->sourcePath = sourcePath;
    asset->hasAnimation = true;
    //the root's rest transform is the object's, so the tree starts at identity and the gizmo sits on the model
    glm::mat4 rootRest = GLMConverter::AssimpToGLM(groupRoot->mTransformation);
    glm::mat4 belowRootRest = glm::inverse(rootRest);
    asset->rootNode = asset->loadNodeTree(groupRoot, &groupNodes);
    asset->rootNode->transformation = glm::mat4(1.0f);
    asset->createMeshes(scene, groupRoot, belowRootRest, &groupMeshes);
    if (asset->meshes.empty()) {
        std::cerr << "Animation group " << groupPath << " of " << sourcePath << " has no mesh with triangles." << std::endl;
        exit(-1);
    }
    std::map<std::string, glm::mat4> restGlobals;
    collectRestGlobals(asset->rootNode, glm::mat4(1.0f), restGlobals);
    rebindGroupSkins(*asset, scene, groupRoot, belowRootRest, groupMeshes, restGlobals);
    asset->buildLodLevels(LodLadder::BuildMode::NORMAL);

    std::vector<aiAnimation *> clips(groupClips.begin(), groupClips.end());//fillAnimationSet takes a mutable array
    asset->fillAnimationSet((unsigned int) clips.size(), clips.data());

    asset->boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
    asset->boundingBoxMax = glm::vec3(std::numeric_limits<float>::lowest());
    accumulateBindPoseBounds(*asset, scene, groupRoot, belowRootRest, groupMeshes);
    asset->centerOffset = (asset->boundingBoxMin + asset->boundingBoxMax) / 2.0f;
    asset->buildPhysicsMeshes();
    return asset;
}

void ModelAssetBuilder::collectRestGlobals(const std::shared_ptr<BoneNode> &boneNode, const glm::mat4 &parentTransform,
                                           std::map<std::string, glm::mat4> &restGlobals) {
    glm::mat4 nodeTransform = parentTransform * boneNode->transformation;
    restGlobals[boneNode->name] = nodeTransform;
    for (const std::shared_ptr<BoneNode> &child : boneNode->children) {
        collectRestGlobals(child, nodeTransform, restGlobals);
    }
}

void ModelAssetBuilder::rebindGroupSkins(ModelAsset &asset, const aiScene *scene, const aiNode *node, glm::mat4 parentTransform,
                                         const std::set<std::pair<const aiNode *, uint32_t>> &includedMeshes,
                                         const std::map<std::string, glm::mat4> &restGlobals) {
    parentTransform = parentTransform * GLMConverter::AssimpToGLM(node->mTransformation);
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh->mNumBones != 0 && includedMeshes.count(std::make_pair(node, node->mMeshes[i])) != 0) {
            rebindToRestPose(asset, mesh, parentTransform, glm::mat4(1.0f), restGlobals);
        }
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        rebindGroupSkins(asset, scene, node->mChildren[i], parentTransform, includedMeshes, restGlobals);
    }
}

//skinning puts a vertex at rest * offset * v, the node tree at meshFrame * v. Unreal level exports bind with the mesh at the
//map origin, so every bone is off by the same rigid transform and gets its offset rebuilt from the rest pose
void ModelAssetBuilder::rebindToRestPose(ModelAsset &asset, const aiMesh *mesh, const glm::mat4 &meshFrame, const glm::mat4 &meshSpaceCorrection,
                                         const std::map<std::string, glm::mat4> &restGlobals) {
    glm::mat4 inverseMeshFrame = glm::inverse(meshFrame);
    std::vector<glm::mat4> bindMismatches;
    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        const aiBone *bone = mesh->mBones[boneIndex];
        std::map<std::string, glm::mat4>::const_iterator restGlobal = restGlobals.find(bone->mName.C_Str());
        bool weighted = false;
        for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights && !weighted; ++weightIndex) {
            weighted = bone->mWeights[weightIndex].mWeight > 0.0f;
        }
        if (!weighted || restGlobal == restGlobals.end()) {
            continue;//padding entries say nothing about the bind, and an animated tree leaves them out
        }
        glm::mat4 storedOffset = GLMConverter::AssimpToGLM(bone->mOffsetMatrix) * meshSpaceCorrection;
        bindMismatches.push_back(restGlobal->second * storedOffset * inverseMeshFrame);
    }
    float largestDisagreement = 0.0f;
    float largestMismatch = 0.0f;
    for (size_t boneIndex = 0; boneIndex < bindMismatches.size(); ++boneIndex) {
        for (int column = 0; column < 4; ++column) {
            for (int row = 0; row < 4; ++row) {
                largestDisagreement = std::max(largestDisagreement, std::fabs(bindMismatches[boneIndex][column][row] - bindMismatches[0][column][row]));
                largestMismatch = std::max(largestMismatch, std::fabs(bindMismatches[boneIndex][column][row] - (column == row ? 1.0f : 0.0f)));
            }
        }
    }
    bool rigidMismatch = largestDisagreement <= 1e-3f;
    if (!rigidMismatch) {
        std::cerr << "Skin of mesh " << mesh->mName.C_Str() << " in " << asset.name << " has a bind pose that differs from its rest pose, kept as is." << std::endl;
    } else if (largestMismatch > 1e-3f) {
        std::cout << "Skin of mesh " << mesh->mName.C_Str() << " in " << asset.name << " was bound away from its node, rebound to the rest pose." << std::endl;
    }
    for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        const std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        std::map<std::string, glm::mat4>::const_iterator restGlobal = restGlobals.find(boneName);
        if (restGlobal == restGlobals.end()) {
            continue;
        }
        ModelAsset::BoneInformation &boneInformation = asset.boneInformationMap[boneName];
        boneInformation.offset = rigidMismatch ? glm::inverse(restGlobal->second) * meshFrame
                                               : GLMConverter::AssimpToGLM(mesh->mBones[boneIndex]->mOffsetMatrix) * meshSpaceCorrection;
        boneInformation.parentOffset = meshFrame;
        boneInformation.globalMeshInverse = inverseMeshFrame;
    }
}

//animated vertices stay in mesh space, so the box comes from placing them with their node chain
void ModelAssetBuilder::accumulateBindPoseBounds(ModelAsset &asset, const aiScene *scene, const aiNode *node, glm::mat4 parentTransform,
                                                 const std::set<std::pair<const aiNode *, uint32_t>> &includedMeshes) {
    parentTransform = parentTransform * GLMConverter::AssimpToGLM(node->mTransformation);
    for (uint32_t i = 0; i < node->mNumMeshes; ++i) {
        if (includedMeshes.count(std::make_pair(node, node->mMeshes[i])) == 0) {
            continue;
        }
        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            glm::vec3 placedVertex = glm::vec3(parentTransform * glm::vec4(GLMConverter::AssimpToGLM(mesh->mVertices[vertexIndex]), 1.0f));
            asset.boundingBoxMin = glm::min(asset.boundingBoxMin, placedVertex);
            asset.boundingBoxMax = glm::max(asset.boundingBoxMax, placedVertex);
        }
    }
    for (uint32_t i = 0; i < node->mNumChildren; ++i) {
        accumulateBindPoseBounds(asset, scene, node->mChildren[i], parentTransform, includedMeshes);
    }
}
