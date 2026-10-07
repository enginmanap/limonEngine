//
// Created by engin on 03.10.2026.
//

#ifndef LIMONENGINE_MODELTOWORLDCONVERTER_H
#define LIMONENGINE_MODELTOWORLDCONVERTER_H

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <assimp/scene.h>

class AssetManager;
class ModelAsset;

/**
 * Turns a model that is really a whole map into one limonmodel per unique mesh, plus a world that places every
 * node using them as its own object. Static models only.
 */
class ModelToWorldConverter {
    struct PieceKey {
        uint32_t meshIndex;
        bool mirrored;

        bool operator<(const PieceKey &other) const {
            if (meshIndex != other.meshIndex) {
                return meshIndex < other.meshIndex;
            }
            return mirrored < other.mirrored;
        }
    };

    struct Piece {
        std::string filePath;
        std::string firstNodeName;
        uint32_t sourceMeshIndex = 0;//the first copy's mesh, its offsets belong to the skeleton below
        const aiNode *skeletonRoot = nullptr;//only a skinned mesh has one
        std::set<const aiNode *> skeletonNodes;
        glm::mat4 skeletonPlacement = glm::mat4(1.0f);//skeleton root's parent, in the first copy's piece space
        glm::vec3 centerOffset = glm::vec3(0.0f);
    };

    //meshes that share an animation, directly or through another mesh, end up in one model
    struct AnimationGroup {
        std::set<std::pair<const aiNode *, uint32_t>> meshReferences;
        std::vector<uint32_t> animationIndices;
        const aiNode *root = nullptr;
        std::set<const aiNode *> treeNodes;
        std::vector<const aiNode *> orderedNodes;//rig order, copies are compared and their channels renamed position by position
        int32_t sharedGroup = -1;//a copy uses that group's file instead of writing its own
        std::string filePath;//empty when the group can't be converted
        glm::vec3 translate = glm::vec3(0.0f);//no centerOffset to put back, animated models carry it in the bones
        glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
    };

    struct Instance {
        PieceKey key;
        glm::vec3 translate;//of the node, the piece's centerOffset is not applied yet
        glm::quat orientation;
        glm::vec3 scale;
    };

    struct ClipCandidate {
        std::unique_ptr<aiAnimation> clip;
        std::string copyRootName;//its name prefix when copies disagree about one clip name
    };

    static constexpr uint32_t FIRST_OBJECT_ID = 2;//the world hands out IDs from here too
    static constexpr float CHANNEL_REST_EPSILON = 1e-4f;//exporters key every node, a key this close to rest doesn't animate

    std::shared_ptr<AssetManager> assetManager;
    std::string sourcePath;
    std::string outputDirectory;
    std::string sourceStem;
    float scale;
    glm::mat4 rootTransform = glm::mat4(1.0f);//the scale, as a parent of the whole model

    std::map<PieceKey, Piece> pieces;
    std::vector<Instance> instances;
    std::vector<std::string> rejectedNodes;
    glm::vec3 highestPoint = glm::vec3(0.0f);
    bool anyVertexSeen = false;
    std::mutex logMutex;

    std::map<std::string, std::vector<const aiNode *>> nodesByName;//copies of a prefab repeat names, every car calls its bones the same
    std::vector<uint32_t> canonicalMeshIndices;//a skinned mesh points at the first mesh with the same geometry and weights
    std::map<const aiNode *, const aiNode *> parentNodes;
    std::vector<std::set<const aiNode *>> movingNodesPerAnimation;
    std::set<std::pair<const aiNode *, uint32_t>> animatedMeshReferences;
    std::vector<AnimationGroup> animationGroups;
    std::vector<std::vector<std::unique_ptr<aiAnimation>>> groupClips;//per file writing group, every copy's clips renamed to its tree

    void indexNodes(const aiNode *node, const aiNode *parent);
    void collectMeshReferences(const aiScene *scene, const aiNode *node, std::vector<std::pair<const aiNode *, uint32_t>> &meshReferences) const;
    void findAnimationGroups(const aiScene *scene);
    bool prepareAnimationGroup(const aiScene *scene, uint32_t groupIndex, const glm::mat4 &rootTransform);
    bool isMeshMovedBy(const aiScene *scene, const aiNode *node, uint32_t meshIndex, const std::set<const aiNode *> &movingNodes) const;
    bool isSelfOrDescendantOf(const aiNode *node, const std::set<const aiNode *> &ancestors) const;
    bool collectBoneNodes(const aiMesh *mesh, const aiNode *meshNode, bool weightedOnly, std::set<const aiNode *> &boneNodes) const;
    static bool isBoneWeighted(const aiBone *bone);
    const aiNode *findBoneNode(const aiNode *meshNode, const std::string &boneName) const;
    void findSkinnedDuplicates(const aiScene *scene);

    static uint64_t hashSkinnedMesh(const aiMesh *mesh);
    static bool areSkinnedMeshesEqual(const aiMesh *first, const aiMesh *second);
    const aiNode *findLowestCommonAncestor(const std::set<const aiNode *> &nodes) const;
    void addPathToAncestor(const aiNode *node, const aiNode *ancestor, std::set<const aiNode *> &pathNodes) const;
    glm::mat4 buildGlobalTransform(const aiNode *node, const glm::mat4 &rootTransform) const;
    void orderTreeNodes(const aiNode *node, const std::set<const aiNode *> &treeNodes, std::vector<const aiNode *> &orderedNodes) const;
    bool areGroupsEquivalent(const aiScene *scene, const AnimationGroup &first, const AnimationGroup &second) const;
    bool isSkinReboundToRest(const aiMesh *mesh, const aiNode *meshNode) const;
    void findSharedGroups(const aiScene *scene);
    void buildGroupClips(const aiScene *scene);

    static aiNodeAnim *copyChannel(const aiNodeAnim *channel, const aiString &nodeName, const aiNode *rebaseRoot);
    static bool areChannelsEqual(const aiNodeAnim *first, const aiNodeAnim *second);
    static bool areClipsEqual(const aiAnimation *first, const aiAnimation *second);
    static bool areMatricesClose(const glm::mat4 &first, const glm::mat4 &second);

    static size_t findUnionRoot(std::vector<size_t> &unionParents, size_t index);
    static bool channelMoves(const aiNodeAnim *channel, const aiNode *node);
    static bool decomposeWithoutShear(const glm::mat4 &placement, glm::vec3 &translate, glm::quat &orientation, glm::vec3 &scale);

    void collectInstances(const aiScene *scene, const aiNode *node, const glm::mat4 &parentTransform, const std::string &parentPath);
    std::string buildPieceFilePath(const PieceKey &key, const std::string &nodeName) const;
    void buildPieces(const aiScene *scene, const std::vector<std::map<PieceKey, Piece>::iterator> &pieceQueue,
                     std::vector<std::unique_ptr<ModelAsset>> &builtPieces, std::atomic<size_t> &nextPiece,
                     std::atomic<size_t> &finishedPieceCount, std::atomic<bool> &anyPieceFailed);
    bool writeWorld(const std::string &worldPath) const;

    static bool hasTriangles(const aiMesh *mesh);
    static std::string sanitizeForFileName(const std::string &name);

public:
    //an empty outputDirectory means a folder named after the source, next to it. The scale only moves and sizes the
    //objects, the pieces stay in the source's units
    ModelToWorldConverter(std::shared_ptr<AssetManager> assetManager, const std::string &sourcePath, const std::string &outputDirectory,
                          float scale);

    bool convert();
};


#endif //LIMONENGINE_MODELTOWORLDCONVERTER_H
