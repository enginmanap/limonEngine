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

        //mirrored pieces carry the flip in their vertices, so what is left for the object is a proper rotation
        bool mirrored = glm::determinant(glm::mat3(nodeTransform)) < 0.0f;
        glm::mat4 placement = mirrored ? nodeTransform * glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f)) : nodeTransform;

        glm::vec3 scale, translate, skew;
        glm::quat orientation;
        glm::vec4 perspective;
        if (!glm::decompose(placement, scale, orientation, translate, skew, perspective)) {
            rejectedNodes.push_back(nodePath + ": transform can't be decomposed");
            continue;
        }
        //a non uniform scale above a rotated child composes into shear, which Transformation can't hold
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
        if (largestError > 1e-4f * std::max(largestElement, 1e-6f)) {
            rejectedNodes.push_back(nodePath + ": transform has shear, which is not supported");
            continue;
        }

        PieceKey key{meshIndex, mirrored};
        if (pieces.find(key) == pieces.end()) {
            Piece piece;
            piece.filePath = buildPieceFilePath(key, nodeName);
            piece.firstNodeName = nodeName;
            pieces[key] = piece;
        }
        Instance instance;
        instance.key = key;
        instance.translate = translate;
        instance.orientation = orientation;
        instance.scale = scale;
        instances.push_back(instance);

        for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
            glm::vec3 worldVertex = glm::vec3(nodeTransform * glm::vec4(GLMConverter::AssimpToGLM(mesh->mVertices[vertexIndex]), 1.0f));
            if (!anyVertexSeen || worldVertex.y > highestPoint.y) {
                highestPoint = worldVertex;
                anyVertexSeen = true;
            }
        }
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
    if (scene->mNumAnimations > 0) {
        std::cerr << sourcePath << " has animations, only static models can be converted." << std::endl;
        return false;
    }
    for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        if (scene->mMeshes[meshIndex]->HasBones()) {
            std::cerr << sourcePath << " has bones, only static models can be converted." << std::endl;
            return false;
        }
    }

    //a parent of the whole model, so positions scale from the map origin and the pieces stay aligned
    collectInstances(scene, scene->mRootNode, glm::scale(glm::mat4(1.0f), glm::vec3(scale)), "");
    if (!rejectedNodes.empty()) {
        std::cerr << "Conversion of " << sourcePath << " refused, " << rejectedNodes.size() << " nodes can't be converted:" << std::endl;
        for (const std::string &rejectedNode : rejectedNodes) {
            std::cerr << "    " << rejectedNode << std::endl;
        }
        return false;
    }
    if (instances.empty()) {
        std::cerr << sourcePath << " has no mesh to convert." << std::endl;
        return false;
    }
    //one more for the light. An object at or above NR_MAX_MODELS gets no transform and silently doesn't render
    uint64_t lastID = FIRST_OBJECT_ID + instances.size();
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

    std::cout << instances.size() << " objects using " << pieces.size() << " pieces" << std::endl;
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
        std::unique_ptr<ModelAsset> pieceAsset = std::make_unique<ModelAsset>(assetManager.get(), scene, pieceIterator->first.meshIndex,
                                                                              pieceIterator->first.mirrored, sourcePath,
                                                                              pieceIterator->second.filePath,
                                                                              pieceIterator->second.firstNodeName);
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
