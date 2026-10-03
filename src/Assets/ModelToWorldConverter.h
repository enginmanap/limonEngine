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
        glm::vec3 centerOffset = glm::vec3(0.0f);
    };

    struct Instance {
        PieceKey key;
        glm::vec3 translate;//of the node, the piece's centerOffset is not applied yet
        glm::quat orientation;
        glm::vec3 scale;
    };

    static constexpr uint32_t FIRST_OBJECT_ID = 2;//the world hands out IDs from here too

    std::shared_ptr<AssetManager> assetManager;
    std::string sourcePath;
    std::string outputDirectory;
    std::string sourceStem;
    float scale;

    std::map<PieceKey, Piece> pieces;
    std::vector<Instance> instances;
    std::vector<std::string> rejectedNodes;
    glm::vec3 highestPoint = glm::vec3(0.0f);
    bool anyVertexSeen = false;
    std::mutex logMutex;

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
