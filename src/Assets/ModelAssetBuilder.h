//
// Created by engin on 07.10.2026.
//

#ifndef LIMONENGINE_MODELASSETBUILDER_H
#define LIMONENGINE_MODELASSETBUILDER_H

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include "BoneNode.h"

class AssetManager;
class ModelAsset;

/**
 * Builds ModelAssets from parts of an imported scene for the map converter. Never in the asset cache and never sent
 * to the GPU, they only exist to be written out. The caller must have added the scene's embedded textures under sourcePath.
 */
class ModelAssetBuilder {
    static void collectRestGlobals(const std::shared_ptr<BoneNode> &boneNode, const glm::mat4 &parentTransform,
                                   std::map<std::string, glm::mat4> &restGlobals);
    static void rebindGroupSkins(ModelAsset &asset, const aiScene *scene, const aiNode *node, glm::mat4 parentTransform,
                                 const std::set<std::pair<const aiNode *, uint32_t>> &includedMeshes,
                                 const std::map<std::string, glm::mat4> &restGlobals);
    static void rebindToRestPose(ModelAsset &asset, const aiMesh *mesh, const glm::mat4 &meshFrame, const glm::mat4 &meshSpaceCorrection,
                                 const std::map<std::string, glm::mat4> &restGlobals);
    static void accumulateBindPoseBounds(ModelAsset &asset, const aiScene *scene, const aiNode *node, glm::mat4 parentTransform,
                                         const std::set<std::pair<const aiNode *, uint32_t>> &includedMeshes);

public:
    //skeletonRoot is null for a mesh without bones. A skinned one keeps its bones although nothing animates them, with
    //skeletonPlacement putting the skeleton root's parent into the piece's own space
    static std::unique_ptr<ModelAsset> buildPiece(AssetManager *assetManager, const aiScene *scene, uint32_t meshIndex, bool mirrorX,
                                                  const std::string &sourcePath, const std::string &piecePath, const std::string &meshName,
                                                  const aiNode *skeletonRoot, const std::set<const aiNode *> &skeletonNodes,
                                                  const glm::mat4 &skeletonPlacement);

    //the meshes a set of animations moves. groupRoot's global is the object's transform, so the clips must already name
    //this tree's nodes and key the root relative to its rest
    static std::unique_ptr<ModelAsset> buildAnimationGroup(AssetManager *assetManager, const aiScene *scene, const aiNode *groupRoot,
                                                           const std::set<const aiNode *> &groupNodes,
                                                           const std::set<std::pair<const aiNode *, uint32_t>> &groupMeshes,
                                                           const std::vector<aiAnimation *> &groupClips,
                                                           const std::string &sourcePath, const std::string &groupPath);
};


#endif //LIMONENGINE_MODELASSETBUILDER_H
