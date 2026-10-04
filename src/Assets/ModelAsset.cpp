//
// Created by engin on 31.08.2016.
//

#include <set>
#include <unordered_set>
#include <limits>

#include <assimp/config.h>

#include "ModelAsset.h"

#include <chrono>
#include <fstream>
#ifdef CEREAL_SUPPORT
#include <cereal/archives/binary.hpp>
#endif
#include "Lod/LodMetadata.h"
#include "../Utils/GLMUtils.h"
#include "Animations/AnimationAssimp.h"
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "Animations/AnimationAssimpSection.h"
#include "../../libs/ImGui/imgui.h"

std::string ModelAsset::stripFlipSuffix(const std::string &path, bool &outFlipX, bool &outFlipY, bool &outFlipZ) {
    outFlipX = outFlipY = outFlipZ = false;
    size_t pos = path.rfind("?flip");
    if (pos == std::string::npos) {
        return path;
    }
    std::string suffix = path.substr(pos + 5);
    for (char c : suffix) {
        if (c == 'X') outFlipX = true;
        else if (c == 'Y') outFlipY = true;
        else if (c == 'Z') outFlipZ = true;
    }
    return path.substr(0, pos);
}

ModelAsset::ModelAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList)
        : Asset(assetManager, assetID,
                fileList),
          boneIDCounter(0),
          boneIDCounterPerMesh(0) {
    if (fileList.empty()) {
        std::cerr << "Model load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = stripFlipSuffix(fileList[0], flipX, flipY, flipZ);
    sourcePath = name;
    reverseWinding = (int(flipX) + int(flipY) + int(flipZ)) % 2 == 1;
    if (fileList.size() > 1) {
        std::cerr << "multiple files are sent to Model constructor, extra elements ignored." << std::endl;
    }
}

ModelAsset::ModelAsset(AssetManager *assetManager, const aiScene *scene, uint32_t meshIndex, bool mirrorX,
                       const std::string &sourcePath, const std::string &piecePath, const std::string &meshName)
        : Asset(assetManager, 0, {piecePath}),
          boneIDCounter(0),
          boneIDCounterPerMesh(0) {
    name = piecePath;
    this->sourcePath = sourcePath;
    reverseWinding = mirrorX;
    hasAnimation = false;
    rootNode = std::make_shared<BoneNode>();
    rootNode->name = meshName;
    rootNode->boneID = boneIDCounter++;
    rootNode->transformation = glm::mat4(1.0f);

    const aiMesh *sourceMesh = scene->mMeshes[meshIndex];
    std::shared_ptr<Material> meshMaterial = loadMaterials(scene, sourceMesh->mMaterialIndex);
    //no node transform on purpose, every node using this mesh places it through its own object transform
    glm::mat4 mirrorTransform = mirrorX ? glm::scale(glm::mat4(1.0f), glm::vec3(-1.0f, 1.0f, 1.0f)) : glm::mat4(1.0f);
    std::shared_ptr<MeshAsset> mesh = std::make_shared<MeshAsset>(sourceMesh, meshName, rootNode, mirrorTransform, false, reverseWinding);
    meshMaterialMap[mesh] = meshMaterial;
    if ((*mesh->getTriangleCount()) == 0) {
        std::cerr << "Mesh " << meshIndex << " of " << sourcePath << " has no triangles, piece " << piecePath << " is empty." << std::endl;
        exit(-1);
    }
    if (meshMaterial->hasOpacityMap()) {
        transparentMaterialUsed = true;
    }
    meshes.push_back(mesh);

    buildLodLevels(LodLadder::BuildMode::NORMAL);
    computeBoundsFromVertices();
    buildPhysicsMeshes();
}

const aiScene *ModelAsset::importScene(Assimp::Importer &assimpImporter, const std::string &path) {
    assimpImporter.SetPropertyBool("AI_CONFIG_IMPORT_FBX_EMBEDDED_TEXTURES_LEGACY_NAMING", true);
    assimpImporter.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 65536);//faces are u16vec3, a bigger submesh wraps its indices and scrambles the mesh
    unsigned int flags = (aiProcess_GlobalScale|aiProcess_GenBoundingBoxes | aiProcess_FlipUVs | aiProcessPreset_TargetRealtime_MaxQuality);
#ifdef ASSIMP_VALIDATE_WORKAROUND
    flags = flags & ~aiProcess_FindInvalidData;
#endif
    const aiScene *scene = assimpImporter.ReadFile(path, flags);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << path << "::" << assimpImporter.GetErrorString() << std::endl;
        return nullptr;
    }
    return scene;
}

std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> ModelAsset::readEmbeddedTextures(const aiScene *scene) {
    std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> textures;
    for (size_t i = 0; i < scene->mNumTextures; ++i) {
        aiTexture* currentTexture = scene->mTextures[i];
        std::shared_ptr<AssetManager::EmbeddedTexture> eTexture = std::make_shared<AssetManager::EmbeddedTexture>();

        eTexture->height = currentTexture->mHeight;
        eTexture->width = currentTexture->mWidth;
        memcpy(&eTexture->format, &currentTexture->achFormatHint, sizeof(eTexture->format));
        if(eTexture->height != 0) {
            eTexture->texelData.resize(currentTexture->mHeight * currentTexture->mWidth);
            memcpy(eTexture->texelData.data(), currentTexture->pcData, currentTexture->mHeight * currentTexture->mWidth);
        } else {
            //compressed data
            eTexture->texelData.resize(currentTexture->mWidth);
            memcpy(eTexture->texelData.data(), currentTexture->pcData, currentTexture->mWidth);
        }
        textures.push_back(eTexture);
    }
    return textures;
}

void ModelAsset::computeBoundsFromVertices() {
    boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
    boundingBoxMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const std::shared_ptr<MeshAsset> &mesh : meshes) {
        for (const glm::vec3 &vertex : mesh->getVertices()) {
            boundingBoxMin = glm::min(boundingBoxMin, vertex);
            boundingBoxMax = glm::max(boundingBoxMax, vertex);
        }
    }
    centerOffset = (boundingBoxMin + boundingBoxMax) / 2.0f;
}

void ModelAsset::loadCPUPart() {

    //std::cout << "ASSIMP::Loading::" << name << std::endl;
    Assimp::Importer assimpImporter;
    const aiScene *scene = importScene(assimpImporter, name);
    if (scene == nullptr) {
        exit(-1);
    }

    std::vector<std::shared_ptr<const AssetManager::EmbeddedTexture>> textures = readEmbeddedTextures(scene);
    if(textures.size() > 0 ) {
        assetManager->addEmbeddedTextures(this->sourcePath, textures);
    }

    this->hasAnimation = (scene->mNumAnimations != 0);

    //std::cout << "ASSIMP::success::" << name << std::endl;

    if (!scene->HasMeshes()) {
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    } else {
        //std::cout << "Model "<< this->name << " has " << scene->mNumMeshes << " mesh(es)." << std::endl;
    }

    this->rootNode = loadNodeTree(scene->mRootNode);

    glm::mat4 initialTransform(1.0f);
    if (flipX || flipY || flipZ) {
        if (this->hasAnimation) {
            std::cerr << "WARNING: flip requested for animated model " << name << " — flip is not supported for animated meshes, ignoring." << std::endl;
        } else {
            initialTransform = glm::scale(initialTransform, glm::vec3(flipX ? -1.0f : 1.0f, flipY ? -1.0f : 1.0f, flipZ ? -1.0f : 1.0f));
        }
    }
    createMeshes(scene, scene->mRootNode, initialTransform);
    buildLodLevels(LodLadder::BuildMode::NORMAL);
    if(this->hasAnimation) {
        fillAnimationSet(scene->mNumAnimations, scene->mAnimations);
    }

    if ((flipX || flipY || flipZ) && !this->hasAnimation) {
        computeBoundsFromVertices();
    } else {
        aiVector3D min, max;
        AssimpUtils::get_bounding_box(scene, &min, &max);
        boundingBoxMax = GLMConverter::AssimpToGLM(max);
        boundingBoxMin = GLMConverter::AssimpToGLM(min);
        centerOffset = glm::vec3((max.x + min.x) / 2, (max.y + min.y) / 2, (max.z + min.z) / 2);
    }
    //std::cout << "Model asset: " << name << "Assimp bounding box is " << GLMUtils::vectorToString(boundingBoxMin) << ", " <<  GLMUtils::vectorToString(boundingBoxMax) << std::endl;
    //Implicit call to import.FreeScene(), and removal of scene.

    //it is possible that there are mixamo animation files, check if they do, add them too if needed.
    // we get a list of file paths, so we don't hold the lock in asset tree for longer than needed
    std::vector<std::string> mixamoFilePaths = assetManager->getMixamoAnimationFilePaths(this->name);

    if (!mixamoFilePaths.empty()) {
        int32_t mixamoCount = 0;
        Assimp::Importer importer2;
        const aiScene *mixamoScene = nullptr;
        for (size_t i = 0; i < mixamoFilePaths.size(); ++i) {
            const std::string &mixamoFilePath = mixamoFilePaths[i];
            mixamoScene = importer2.ReadFile(mixamoFilePath, 0);
            if (!mixamoScene || !mixamoScene->mRootNode) {//it might have mixamoScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE, but that is expected
                std::cerr << "ERROR::ASSIMP::MIXAMO" << importer2.GetErrorString() << std::endl;
                //delete mixamoScene; don't delete, importer deletes
                continue;
            }
            if (mixamoScene->mNumAnimations != 0) {
                //Since we found an animation, lets first make sure we set this model as animated
                this->hasAnimation = true;
                //get the file name without extension
                size_t slashPosition = mixamoFilePath.find_last_of("/\\");
                std::string mixamoFileName = (slashPosition == std::string::npos) ? mixamoFilePath : mixamoFilePath.substr(slashPosition + 1);
                fillAnimationSet(mixamoScene->mNumAnimations, mixamoScene->mAnimations, mixamoFileName.substr(0, mixamoFileName.find_last_of(".")) + "|");
                mixamoCount++;
            } else {
                std::cout << "No animation in Mixamo file, it won't effect anyting." << std::endl;
            }
        }
        std::cout << mixamoCount << " mixamo animations found for model " << this->name << std::endl;
    }

    this->deserializeCustomizations();

    buildPhysicsMeshes();
}


//only the level the option asks for is baked at load, a limonmodel is expected to carry all of them
std::string ModelAsset::getFlipAxes() const {
    std::string flipAxes;
    if (flipX) flipAxes += 'X';
    if (flipY) flipAxes += 'Y';
    if (flipZ) flipAxes += 'Z';
    return flipAxes;
}

/**
 * Decides this model's LOD steps, then hands each mesh its plan. Measuring needs the assembled model, so it can
 * only run once every mesh exists, and its result is cached beside the asset because measuring is far more
 * expensive than regenerating from a known error.
 *
 * Runs on both load paths. A model that came from a binary arrives with its steps already filled, so this reads
 * its intent, finds nothing to change, and every mesh keeps the ranges it was deserialized with.
 */
void ModelAsset::buildLodLevels(LodLadder::BuildMode buildMode) {
    OptionsUtil::Options *options = assetManager == nullptr ? nullptr
                                                           : assetManager->getGraphicsWrapper()->getOptions();
    //a skinned mesh deforms, so a bind pose score says nothing about what it shows in motion
    lodLadder.bindAsset(name, getFlipAxes(), !hasAnimation, isConvertedAsset());
    lodLadder.readSettings(options);

    std::vector<LodLadder::MeshGeometry> geometry;
    buildLodGeometry(geometry);
    if (geometry.empty()) {
        return;
    }
    std::vector<LodLadder::LevelPlan> plan;
    lodLadder.build(geometry, buildMode, plan);

    bool anyMeshNeedsBuild = false;
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        if (!meshes[meshIndex]->hasLodsFor(plan)) {
            anyMeshNeedsBuild = true;
        }
    }
    if (anyMeshNeedsBuild) {
        //preparing the simplifier inputs is the expensive part, so it waits until something actually builds
        lodLadder.prepareGenerators(geometry);
        for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
            if (!meshes[meshIndex]->hasLodsFor(plan)) {
                meshes[meshIndex]->buildLods(lodLadder.getGenerator(meshIndex), plan);
            }
        }
    }

    uint32_t bakeOccluderLodLevel = 0;
    if (options != nullptr) {
        bakeOccluderLodLevel = (uint32_t) options->getOption<long>(HASH("occlusion_bakeLodLevel")).getOrDefault(0L);
    }
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        meshes[meshIndex]->bakeOccluderLod(meshes[meshIndex]->getSimplestLodLevel(bakeOccluderLodLevel));
    }
}

//the geometry the ladder measures and simplifies. One space for the whole model, since every mesh already
//carries its node transform, and a mesh scored alone measures something the viewer never sees
void ModelAsset::buildLodGeometry(std::vector<LodLadder::MeshGeometry> &outGeometry) const {
    outGeometry.clear();
    outGeometry.reserve(meshes.size());
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        const std::vector<glm::vec3> &meshVertices = meshes[meshIndex]->getVertices();
        const std::vector<glm::u16vec3> &meshFaces = meshes[meshIndex]->getFaces();
        size_t lod0IndexCount = meshes[meshIndex]->getTriangleCount()[0] * 3;
        if (meshVertices.empty() || meshFaces.empty() || lod0IndexCount == 0) {
            continue;
        }
        LodLadder::MeshGeometry entry;
        entry.vertices = &meshVertices;
        entry.normals = &meshes[meshIndex]->getNormals();
        entry.textureCoordinates = &meshes[meshIndex]->getTextureCoordinates();
        entry.indices = (const uint16_t *) &(meshFaces[0].x);
        entry.indexCount = lod0IndexCount;
        entry.aabbMin = glm::vec3(meshes[meshIndex]->getAabbMin());
        entry.aabbMax = glm::vec3(meshes[meshIndex]->getAabbMax());
        outGeometry.push_back(entry);
    }
}

bool ModelAsset::isConvertedAsset() const {
    return name.substr(name.find_last_of(".") + 1) == "limonmodel";
}

bool ModelAsset::saveChanges() {
    if (isConvertedAsset()) {
        if (!writeBinary(name)) {
            return false;
        }
        customizationAfterSave = false;
        lodLadder.clearUnsavedChanges();
        return true;
    }
    bool customizationsSaved = serializeCustomizations();
    bool lodIntentSaved = lodLadder.saveIntent();
    return customizationsSaved && lodIntentSaved;
}

bool ModelAsset::writeBinary(const std::string &path) {
#ifdef CEREAL_SUPPORT
    bakeAllOccluderLods();//load only baked the level the option asked for, the file should carry all of them
    std::ofstream os(path, std::ios::binary);
    if (!os.is_open()) {
        std::cerr << "Could not open " << path << " for writing" << std::endl;
        return false;
    }
    {
        cereal::BinaryOutputArchive archive(os);
        archive(*this);
    }
    os.flush();
    if (!os.good()) {
        std::cerr << "Writing " << path << " failed" << std::endl;
        return false;
    }
    return true;
#else
    std::cerr << "Cereal support disabled, " << path << " not written" << std::endl;
    return false;
#endif
}

void ModelAsset::regenerateLods(LodLadder::BuildMode buildMode) {
    buildLodLevels(buildMode);
    for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex) {
        meshes[meshIndex]->reuploadGeometryBuffers(assetManager);
    }
}

//only the level the option asks for is baked at load, a limonmodel is expected to carry all of them
void ModelAsset::bakeAllOccluderLods() {
    for (auto mesh = meshes.begin(); mesh != meshes.end(); ++mesh) {
        (*mesh)->bakeAllOccluderLods();
    }
}
void ModelAsset::loadGPUPart() {
    // serialize should save these
    // assetID
    // boneIDCounter
    // boneIDCounterPerMesh
    // name
    // textures -> get from assetmanager
    // hasAnimation
    // rootNode
    // boundingBoxMax
    // boundingBoxMin
    // centerOffset
    // boneInformationMap
    // simplifiedMeshes
    // meshes
    // animations
    // animationSections
    // customizationAfterSave

    // AssetManager::EmbeddedTexture eTextures should be serializeable
    // Animations should be

    if(temporaryEmbeddedTextures != nullptr && temporaryEmbeddedTextures->size() > 0 ) {
        assetManager->addEmbeddedTextures(this->sourcePath, *temporaryEmbeddedTextures);
    }
    temporaryEmbeddedTextures.reset();

    for (auto material = materialMap.begin(); material != materialMap.end(); ++material) {
        material->second->afterLoad(assetManager);//Since this depends on the embedded textures, needs to be done as part of GPU load
        material->second->loadGPUSide(assetManager);
        assetManager->getGraphicsWrapper()->setMaterial(*material->second);
    }

    for (auto mesh = meshes.begin(); mesh != meshes.end(); ++mesh) {
        (*mesh)->loadGPUPart(assetManager);
    }
}


std::shared_ptr<Material> ModelAsset::loadMaterials(const aiScene *scene, unsigned int materialIndex) {
    // create material uniform buffer
    aiMaterial *currentMaterial = scene->mMaterials[materialIndex];
    aiString materialName;
    aiString texturePath;
    if (AI_SUCCESS != currentMaterial->Get(AI_MATKEY_NAME, materialName)) {
        std::cerr << "Material without a name is not handled." << std::endl;
        std::cerr << "Model " << this->name << std::endl;
        exit(-1);
        //return nullptr; should work too.
    }

    std::shared_ptr<Material> newMaterial;
    if (materialMap.find(materialName.C_Str()) == materialMap.end()) {//search for the name
        //if the material is not loaded before
        newMaterial = std::make_shared<Material>(assetManager, materialName.C_Str(), 0);//material index is not set, as it will be set by after serialize
        aiColor3D color(0.f, 0.f, 0.f);
        float transferFloat;

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
            newMaterial->setAmbientColor(GLMConverter::AssimpToGLM(color));
        } else {
            newMaterial->setAmbientColor(glm::vec3(0,0,0));
        }

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            newMaterial->setDiffuseColor(GLMConverter::AssimpToGLM(color));
        } else {
            newMaterial->setDiffuseColor(glm::vec3(0,0,0));
        }

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
            newMaterial->setSpecularColor(GLMConverter::AssimpToGLM(color));
        } else {
            newMaterial->setSpecularColor(glm::vec3(0,0,0));
        }

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_SHININESS, transferFloat)) {
            newMaterial->setSpecularExponent(transferFloat);
        } else {
            newMaterial->setSpecularExponent(0);
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_AMBIENT) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_AMBIENT, 0, &texturePath)) {
                if(texturePath.data[0] != '*') {
                    newMaterial->setAmbientTexture(texturePath.C_Str());
                    std::cout << "set ambient texture " << texturePath.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setAmbientTexture(texturePath.C_Str(), &this->sourcePath);
                    std::cout << "set (embedded) ambient texture " << texturePath.C_Str() << "|" << this->name<< std::endl;
                }

            } else {
                std::cerr << "The model contained ambient texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << texturePath.C_Str() << "]" << std::endl;
            }
        }
        if ((currentMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath)) {
                if(texturePath.data[0] != '*') {
                    newMaterial->setDiffuseTexture(texturePath.C_Str());
                } else {
                    //embeddedTexture handling
                    newMaterial->setDiffuseTexture(texturePath.C_Str(), &this->sourcePath);
                }
            } else {
                std::cerr << "The model contained diffuse texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << texturePath.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_SPECULAR, 0, &texturePath)) {
                if(texturePath.data[0] != '*') {
                    newMaterial->setSpecularTexture(texturePath.C_Str());
                    std::cout << "set specular texture " << texturePath.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setSpecularTexture(texturePath.C_Str(), &this->sourcePath);
                    std::cout << "set (embedded) setSpecularTexture texture " << texturePath.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained specular texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << texturePath.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_NORMALS, 0, &texturePath)) {
                if(texturePath.data[0] != '*') {
                    newMaterial->setNormalTexture(texturePath.C_Str());
                    std::cout << "set normal texture " << texturePath.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setNormalTexture(texturePath.C_Str(), &this->sourcePath);
                    std::cout << "set (embedded) setNormalTexture texture " << texturePath.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained normal texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << texturePath.C_Str() << "]" << std::endl;
            }
        }


        if ((currentMaterial->GetTextureCount(aiTextureType_OPACITY) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_OPACITY, 0, &texturePath)) {
                if(texturePath.data[0] != '*') {
                    newMaterial->setOpacityTexture(texturePath.C_Str());
                } else {
                    //embeddedTexture handling
                    newMaterial->setOpacityTexture(texturePath.C_Str(), &this->sourcePath);
                    std::cout << "set (embedded) setOpacityTexture texture " << texturePath.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained opacity texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << texturePath.C_Str() << "]" << std::endl;
            }
        }

        uint32_t maps = 0;

        if(newMaterial->hasNormalMap()) {
            maps +=16;
        }
        if(newMaterial->hasAmbientMap()) {
            maps +=8;
        }
        if(newMaterial->hasDiffuseMap()) {
            maps +=4;
        }
        if(newMaterial->hasSpecularMap()) {
            maps +=2;
        }
        if(newMaterial->hasOpacityMap()) {
            maps +=1;
        }

        newMaterial->setMaps(maps);
        //no originalHash, an asset's own material overrides nothing. It gets one only when a world overrides
        //it, set by MaterialRegistry::splitOffOverride, the only place that has the pre edit values
        newMaterial = assetManager->getMaterialRegistry().registerMaterial(newMaterial);
        //key by the name we asked for, not the dedup winner's name, or the next mesh asking for this name
        //misses the lookup above and registers it again. Several names on one Material is fine,
        //~ModelAsset dedups by pointer
        std::string requestedName = materialName.C_Str();
        materialMap[requestedName] = newMaterial;
    } else {
        newMaterial = materialMap[materialName.C_Str()];
    }
    return newMaterial;
}

void ModelAsset::createMeshes(const aiScene *scene, aiNode *aiNode, glm::mat4 parentTransform) {
    parentTransform = parentTransform * GLMConverter::AssimpToGLM(aiNode->mTransformation);

    for (unsigned int i = 0; i < aiNode->mNumMeshes; ++i) {
        aiMesh *currentMesh;
        currentMesh = scene->mMeshes[aiNode->mMeshes[i]];
        for (unsigned int j = 0; j < currentMesh->mNumBones; ++j) {
            boneInformationMap[currentMesh->mBones[j]->mName.C_Str()].globalMeshInverse = glm::inverse(parentTransform);
            boneInformationMap[currentMesh->mBones[j]->mName.C_Str()].offset = GLMConverter::AssimpToGLM(currentMesh->mBones[j]->mOffsetMatrix);
            boneInformationMap[currentMesh->mBones[j]->mName.C_Str()].parentOffset = parentTransform;
        }
        if(currentMesh->mNumBones == 0 && hasAnimation) {
            //If animated, but a mesh without any bone exits, we should process that mesh specially
            boneInformationMap[aiNode->mName.C_Str()].offset = glm::mat4(1.0f);
            boneInformationMap[aiNode->mName.C_Str()].parentOffset = glm::mat4(1.0f);
            boneInformationMap[aiNode->mName.C_Str()].globalMeshInverse = glm::mat4(1.0f);
        }

        std::shared_ptr<Material> meshMaterial = loadMaterials(scene, currentMesh->mMaterialIndex);
        std::shared_ptr<MeshAsset> mesh;
        mesh = std::make_shared<MeshAsset>(currentMesh, aiNode->mName.C_Str(), rootNode,
                                           parentTransform, hasAnimation, reverseWinding);
        meshMaterialMap[mesh] = meshMaterial;
        if((*mesh->getTriangleCount()) == 0) {
            continue;
        }

        if(!strncmp(aiNode->mName.C_Str(), "UCX_", strlen("UCX_"))) {
            //if starts with "UCX_"
            simplifiedMeshes[mesh->getName()] = mesh;
            //std::cout << "simplified mesh " << currentMesh->mName.C_Str() << " for node " << aiNode->mName.C_Str() << std::endl;
        } else {
            if (meshMaterial->hasOpacityMap()) {
                this->transparentMaterialUsed = true;
                meshes.push_back(mesh);
            } else {
                meshes.insert(meshes.begin(), mesh);
            }
            //std::cout << "set mesh " << currentMesh->mName.C_Str() << " for node " << aiNode->mName.C_Str() << std::endl;
        }

    }

    for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
        createMeshes(scene, aiNode->mChildren[i], parentTransform);
    }
}

std::shared_ptr<BoneNode> ModelAsset::loadNodeTree(aiNode *aiNode) {
    auto currentNode = std::make_shared<BoneNode>();
    currentNode->name = aiNode->mName.C_Str();
    currentNode->boneID = boneIDCounter++;
    currentNode->transformation = GLMConverter::AssimpToGLM(aiNode->mTransformation);
    for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
        currentNode->children.push_back(loadNodeTree(aiNode->mChildren[i]));
    }
    return currentNode;
}

bool ModelAsset::findNode(const std::string &nodeName, std::shared_ptr<BoneNode>& foundNode, std::shared_ptr<BoneNode> searchRoot) const {
    if (nodeName == searchRoot->name) {
        foundNode = searchRoot;
        return true;
    } else {
        for (unsigned int i = 0; i < searchRoot->children.size(); ++i) {
            if (findNode(nodeName, foundNode, searchRoot->children[i])) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Blends two animations to generate transform matrix vector
 *
 * @param animationName1
 * @param time1
 * @param looped1
 * @param animationName2
 * @param time2
 * @param looped2
 * @param blendFactor
 * @param transformMatrixVector
 * @return returns true if both of the animations set their last frames. If any were looped, never returns true.
 */
bool ModelAsset::getTransformBlended(std::string animationNameOld, float timeOld, bool loopedOld,
                                     std::string animationNameNew, float timeNew, bool loopedNew,
                                                float blendFactor,
                                                std::vector<glm::mat4> &transformMatrix) const {

/*
    for(auto it = animations.begin(); it != animations.end(); it++) {
        std::cout << "Animations name: " << it->first << " size " << animations.size() <<std::endl;
    }
    */
    if((animationNameOld.empty() || animationNameOld == "") &&
      (animationNameNew.empty() || animationNameNew == "")) {
        //if no animation name is provided, we return bind pose by default
        for(std::unordered_map<std::string, BoneInformation>::const_iterator it = boneInformationMap.begin(); it != boneInformationMap.end(); it++){
            std::shared_ptr<BoneNode> node;
            std::string name = it->first;
            if(findNode(name, node, rootNode)) {
                transformMatrix[node->boneID] = boneInformationMap.at(node->name).parentOffset;
                //parent above means parent transform of the mesh node, not the parent of bone.
            }
        }
        //std::cout << "bind pose returned. for animation name [" << animationNameOld << "]"<< std::endl;
        return true;
    }

    std::shared_ptr<const AnimationInterface> currentAnimationOld = nullptr;
    float animationTimeOld = 0.0f;
    bool isFinishedOld = false;
    if(!(animationNameOld.empty() ||animationNameOld == "")) {
        if (animations.find(animationNameOld) != animations.end()) {
            currentAnimationOld = animations.at(animationNameOld);
        } else {
            //std::cerr << "Animation " << animationNameOld << " not found, playing first animation. " << std::endl;
            currentAnimationOld = animations.begin()->second;
        }

        float ticksPerSecond;
        if (currentAnimationOld->getTicksPerSecond() != 0) {
            ticksPerSecond = currentAnimationOld->getTicksPerSecond();
        } else {
            ticksPerSecond = TICK_PER_SECOND;
        }

        float requestedTime = (timeOld / 1000.0f) * ticksPerSecond;
        if (requestedTime < currentAnimationOld->getDuration()) {
            animationTimeOld = requestedTime;
        } else {
            if (loopedOld) {
                animationTimeOld = fmod(requestedTime, currentAnimationOld->getDuration());
            } else {
                animationTimeOld = currentAnimationOld->getDuration();
                isFinishedOld = true;
            }
        }
    }

    std::shared_ptr<const AnimationInterface> currentAnimationNew = nullptr;
    float animationTimeNew = 0.0f;
    bool isFinishedNew = false;
    if(!(animationNameNew.empty() ||animationNameNew == "")) {
        if (animations.find(animationNameNew) != animations.end()) {
            currentAnimationNew = animations.at(animationNameNew);
        } else {
            //std::cerr << "Animation " << animationNameNew << " not found, playing first animation. " << std::endl;
            currentAnimationNew = animations.begin()->second;
        }

        float ticksPerSecond;
        if (currentAnimationNew->getTicksPerSecond() != 0) {
            ticksPerSecond = currentAnimationNew->getTicksPerSecond();
        } else {
            ticksPerSecond = TICK_PER_SECOND;
        }

        float requestedTime = (timeNew / 1000.0f) * ticksPerSecond;
        if (requestedTime < currentAnimationNew->getDuration()) {
            animationTimeNew = requestedTime;
        } else {
            if (loopedNew) {
                animationTimeNew = fmod(requestedTime, currentAnimationNew->getDuration());
            } else {
                animationTimeNew = currentAnimationNew->getDuration();
                isFinishedNew = true;
            }
        }
    }

    //at this point, it is possible one of the animations doesn't exists, if both didn't we would have returned bind pose.
    //if one of them is not found, return single animation, and log the issue
    glm::mat4 parentTransform(1.0f);

    if(currentAnimationOld == nullptr) {
        std::cerr << "Animation blend fail, old animation "<< animationNameOld <<" not found" << std::endl;
        traverseAndSetTransform(rootNode, parentTransform, currentAnimationNew, animationTimeNew, transformMatrix);
    } else if(currentAnimationNew == nullptr) {
        std::cerr << "Animation blend fail, new animation "<< animationNameNew <<" not found" << std::endl;
        traverseAndSetTransform(rootNode, parentTransform, currentAnimationOld, animationTimeOld, transformMatrix);
    } else {
        traverseAndSetTransformBlended(rootNode, parentTransform, currentAnimationOld, animationTimeOld,
                                       currentAnimationNew, animationTimeNew, blendFactor, transformMatrix);
    }
    return isFinishedOld && isFinishedNew;
}


/**
 * This method is used to request a specific animations transform array for a specific time. If looped is false,
 * it will return if the given time was after or equals final frame. It interpolates by time automatically.
 *
 * @param time Requested animation time in miliseconds.
 * @param looped if animation should loop or not. Effects return.
 * @param animationName name of animation to seek.
 * @param transformMatrix transform matrix list for bones
 *
 * @return if last frame of animation is played for not looped animation. Always false for looped ones.
 */
bool ModelAsset::getTransform(float time, bool looped, std::string animationName, std::vector<glm::mat4> &transformMatrix) const {
/*
    for(auto it = animations.begin(); it != animations.end(); it++) {
        std::cout << "Animations name: " << it->first << " size " << animations.size() <<std::endl;
    }
    */
    if(animationName.empty() || animationName == "") {
        //this means return to bind pose
        //FIXME calculating bind pose for each frame is wrong, but I am assuming this part will be removed, and idle pose
        //will be used instead. If bind pose requirement arises, it should set once, and reused.
        for(std::unordered_map<std::string, BoneInformation>::const_iterator it = boneInformationMap.begin(); it != boneInformationMap.end(); it++){
            std::shared_ptr<BoneNode> node;
            std::string name = it->first;
            if(findNode(name, node, rootNode)) {
                transformMatrix[node->boneID] = boneInformationMap.at(node->name).parentOffset;
                //parent above means parent transform of the mesh node, not the parent of bone.
            }
        }
        //std::cout << "bind pose returned. for animation name [" << animationName << "]"<< std::endl;
        return true;
    }

    std::shared_ptr<const AnimationInterface> currentAnimation;
    if(animations.find(animationName) != animations.end()) {
        currentAnimation = animations.at(animationName);
    } else {
        //std::cerr << "Animation " << animationName << " not found, playing first animation. " << std::endl;
        currentAnimation = animations.begin()->second;
    }

    float animationTime;
    float ticksPerSecond;
    if (currentAnimation->getTicksPerSecond() != 0) {
        ticksPerSecond = currentAnimation->getTicksPerSecond();
    } else {
        ticksPerSecond = TICK_PER_SECOND;
    }

    bool result = false;
    float requestedTime = (time / 1000.0f) * ticksPerSecond;
    if(requestedTime < currentAnimation->getDuration()) {
        animationTime = requestedTime;
    } else {
        if (looped) {
            animationTime = fmod(requestedTime, currentAnimation->getDuration());
        } else {
            animationTime = currentAnimation->getDuration();
            result = true;
        }
    }

    glm::mat4 parentTransform(1.0f);
    traverseAndSetTransform(rootNode, parentTransform, currentAnimation, animationTime, transformMatrix);
    return result;
}

bool ModelAsset::isAnimationFinished(const std::string &animationName, float time, bool looped) const {
    if (animationName.empty()) {
        return true;//bind pose
    }
    std::shared_ptr<const AnimationInterface> currentAnimation;
    if (animations.find(animationName) != animations.end()) {
        currentAnimation = animations.at(animationName);
    } else {
        currentAnimation = animations.begin()->second;
    }
    float ticksPerSecond = currentAnimation->getTicksPerSecond() != 0 ? currentAnimation->getTicksPerSecond() : TICK_PER_SECOND;
    float requestedTime = (time / 1000.0f) * ticksPerSecond;
    return requestedTime >= currentAnimation->getDuration() && !looped;
}

void ModelAsset::getJointTransforms(float time, bool looped, const std::string &animationName, std::vector<glm::mat4> &outJointTransforms) const {
    if (animationName.empty()) {
        glm::mat4 parentTransform(1.0f);
        traverseAndSetBindPoseJointTransform(rootNode, parentTransform, outJointTransforms);
        return;
    }

    std::shared_ptr<const AnimationInterface> currentAnimation;
    if (animations.find(animationName) != animations.end()) {
        currentAnimation = animations.at(animationName);
    } else {
        currentAnimation = animations.begin()->second;
    }

    float ticksPerSecond = currentAnimation->getTicksPerSecond() != 0 ? currentAnimation->getTicksPerSecond() : TICK_PER_SECOND;

    float animationTime;
    float requestedTime = (time / 1000.0f) * ticksPerSecond;
    if (requestedTime < currentAnimation->getDuration()) {
        animationTime = requestedTime;
    } else if (looped) {
        animationTime = fmod(requestedTime, currentAnimation->getDuration());
    } else {
        animationTime = currentAnimation->getDuration();
    }

    glm::mat4 parentTransform(1.0f);
    traverseAndSetJointTransform(rootNode, parentTransform, currentAnimation, animationTime, outJointTransforms);
}

void ModelAsset::traverseAndSetTransformBlended(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                                std::shared_ptr<const AnimationInterface> animationOld,
                                                float timeInTicksOld,
                                                std::shared_ptr<const AnimationInterface> animationNew,
                                                float timeInTicksNew,
                                                float blendFactor,
                                                std::vector<glm::mat4> &transforms) const {

    glm::mat4 nodeTransform;
    Transformation tf1, tf2;
    bool status = animationOld->calculateTransform(boneNode->name, timeInTicksOld, tf1);
    bool status2 = animationNew->calculateTransform(boneNode->name, timeInTicksNew, tf2);

    if(!status && !status2) {
        nodeTransform = boneNode->transformation;//if both failed
    } else if(!status) {
        nodeTransform = tf2.getWorldTransform();//if only first failed(only by else)
    } else if(!status2) {
        nodeTransform = tf1.getWorldTransform();//if only second failed(only by else)
    } else {//if both succeed
        //now blend tf1 and tf2
        Transformation blended;

        glm::vec3 scaleDelta = tf2.getScale() - tf1.getScale();
        blended.setScale(tf1.getScale() + blendFactor * scaleDelta);

        glm::vec3 translateDelta = tf2.getTranslate() - tf1.getTranslate();
        blended.setTransformations(tf1.getTranslate() + blendFactor * translateDelta,
        glm::normalize(slerp(tf1.getOrientation(), tf2.getOrientation(), blendFactor)));

        nodeTransform = blended.getWorldTransform();
    }

    nodeTransform = parentTransform * nodeTransform;

    if(boneInformationMap.find(boneNode->name) != boneInformationMap.end()) {
        transforms[boneNode->boneID] =
                boneInformationMap.at(boneNode->name).globalMeshInverse * boneInformationMap.at(boneNode->name).parentOffset * nodeTransform * boneInformationMap.at(boneNode->name).offset;
        //parent above means parent transform of the mesh node, not the parent of bone.
    }

    //Call children even if parent does not have animation attached.
    for (unsigned int i = 0; i < boneNode->children.size(); ++i) {
        traverseAndSetTransformBlended(boneNode->children[i], nodeTransform, animationOld, timeInTicksOld, animationNew,
                timeInTicksNew, blendFactor, transforms);
    }
}

void ModelAsset::traverseAndSetTransform( std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                          std::shared_ptr<const AnimationInterface> animation,
                                    float timeInTicks,
                                    std::vector<glm::mat4> &transforms) const {

    glm::mat4 nodeTransform;
    Transformation tf;
    bool status = animation->calculateTransform(boneNode->name, timeInTicks, tf);
    nodeTransform = tf.getWorldTransform();

    if(!status) {
        nodeTransform = boneNode->transformation;
    }

    nodeTransform = parentTransform * nodeTransform;

    if(boneInformationMap.find(boneNode->name) != boneInformationMap.end()) {
        transforms[boneNode->boneID] =
                boneInformationMap.at(boneNode->name).globalMeshInverse * boneInformationMap.at(boneNode->name).parentOffset * nodeTransform * boneInformationMap.at(boneNode->name).offset;
        //parent above means parent transform of the mesh node, not the parent of bone.
    }

    //Call children even if parent does not have animation attached.
    for (unsigned int i = 0; i < boneNode->children.size(); ++i) {
        traverseAndSetTransform(boneNode->children[i], nodeTransform, animation, timeInTicks, transforms);
    }
}

void ModelAsset::traverseAndSetJointTransform(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                              std::shared_ptr<const AnimationInterface> animation, float timeInTicks,
                                              std::vector<glm::mat4> &outJointTransforms) const {
    if (boneNode == nullptr) {
        return;
    }
    Transformation tf;
    bool status = animation->calculateTransform(boneNode->name, timeInTicks, tf);
    glm::mat4 nodeTransform = status ? tf.getWorldTransform() : boneNode->transformation;
    nodeTransform = parentTransform * nodeTransform;

    if (boneNode->boneID < outJointTransforms.size()) {
        outJointTransforms[boneNode->boneID] = nodeTransform;
    }

    for (size_t i = 0; i < boneNode->children.size(); ++i) {
        traverseAndSetJointTransform(boneNode->children[i], nodeTransform, animation, timeInTicks, outJointTransforms);
    }
}

void ModelAsset::traverseAndSetBindPoseJointTransform(std::shared_ptr<const BoneNode> boneNode, const glm::mat4 &parentTransform,
                                                       std::vector<glm::mat4> &outJointTransforms) const {
    if (boneNode == nullptr) {
        return;
    }
    glm::mat4 nodeTransform = parentTransform * boneNode->transformation;
    if (boneNode->boneID < outJointTransforms.size()) {
        outJointTransforms[boneNode->boneID] = nodeTransform;
    }
    for (size_t i = 0; i < boneNode->children.size(); ++i) {
        traverseAndSetBindPoseJointTransform(boneNode->children[i], nodeTransform, outJointTransforms);
    }
}

bool ModelAsset::isAnimated() const {
    return hasAnimation;
}

void
ModelAsset::fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations, const std::string &animationNamePrefix) {
    aiAnimation* currentAnimation;
    for (unsigned int i = 0; i < numAnimation; ++i) {
        currentAnimation = pAnimations[i];
        std::string animationName = animationNamePrefix;
        //std::cout << "add animation with name " << animationNamePrefix << animationName << std::endl;

        std::shared_ptr<AnimationAssimp> animationObject = std::make_shared<AnimationAssimp>(currentAnimation);
        animations[animationName] = animationObject;
        //Appearently Mixamo changed some logic, and some models that used to work no longer works.
        // Simple work around is to add both.
        animationName = animationNamePrefix + currentAnimation->mName.C_Str();
        animations[animationName] = animationObject;
    }
    //validate
}

bool ModelAsset::addAnimationAsSubSequence(const std::string &baseAnimationName, const std::string newAnimationName,
                                           float startTime, float endTime) {
    if(this->animations.find(baseAnimationName) == this->animations.end()) {
        //base animation not found
        return false;
    }
    std::shared_ptr<AnimationInterface> animationAssimp = this->animations[baseAnimationName];
    std::shared_ptr<AnimationAssimpSection> animation = std::make_shared<AnimationAssimpSection>(animationAssimp, startTime, endTime);

    this->animations[newAnimationName] = animation;

    this->animationSections.push_back(AnimationSection(baseAnimationName, newAnimationName, startTime, endTime));
    std::cout << "animation created and added to sections" << std::endl;
    this->customizationAfterSave = true;
    return true;
}

bool ModelAsset::serializeCustomizations() {
    if(!customizationAfterSave) {
        return true;//nothing changed since the last save
    }
    //the LOD calibration and the overrides live in the same file, writing a fresh document would drop both
    const std::lock_guard<std::mutex> lock(LodMetadata::getFileMutex());
    std::string customizationPath = name + ".limon";
    tinyxml2::XMLDocument customizationDocument;
    tinyxml2::XMLElement * rootNode = nullptr;
    if (customizationDocument.LoadFile(customizationPath.c_str()) == tinyxml2::XML_SUCCESS) {
        rootNode = customizationDocument.FirstChildElement("ModelCustomizations");
    }
    if (rootNode == nullptr) {
        customizationDocument.Clear();
        rootNode = customizationDocument.NewElement("ModelCustomizations");
        customizationDocument.InsertFirstChild(rootNode);
    }
    tinyxml2::XMLElement * previousAnimationsNode = rootNode->FirstChildElement("Animation");
    if (previousAnimationsNode != nullptr) {
        rootNode->DeleteChild(previousAnimationsNode);
    }

    tinyxml2::XMLElement * animationsSectionsNode = customizationDocument.NewElement("Animation");
    rootNode->InsertEndChild(animationsSectionsNode);
    for(auto it=this->animationSections.begin(); it != this->animationSections.end(); ++it) {
        tinyxml2::XMLElement *sectionElement = customizationDocument.NewElement("Section");
        animationsSectionsNode->InsertEndChild(sectionElement);

        tinyxml2::XMLElement *currentElement = customizationDocument.NewElement("BaseName");
        currentElement->SetText(it->baseAnimationName.c_str());
        sectionElement->InsertEndChild(currentElement);

        currentElement = customizationDocument.NewElement("Name");
        currentElement->SetText(it->animationName.c_str());
        sectionElement->InsertEndChild(currentElement);

        currentElement = customizationDocument.NewElement("StartTime");
        currentElement->SetText(std::to_string(it->startTime).c_str());
        sectionElement->InsertEndChild(currentElement);

        currentElement = customizationDocument.NewElement("EndTime");
        currentElement->SetText(std::to_string(it->endTime).c_str());
        sectionElement->InsertEndChild(currentElement);
    }

    tinyxml2::XMLError eResult = customizationDocument.SaveFile(customizationPath.c_str());
    if(eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "ERROR saving model customization: " << eResult << std::endl;
        return false;
    }
    customizationAfterSave = false;
    return true;
}

void ModelAsset::deserializeCustomizations() {
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile((name + ".limon").c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        if(eResult == tinyxml2::XML_ERROR_FILE_NOT_FOUND) {
            //if no customization, this happens.
            return;
        } else {
            std::cerr << "Error loading XML " << (name + ".limon") << ": " << xmlDoc.ErrorName() << ". Customizations not loaded." << std::endl;
            return;
        }
    }

    tinyxml2::XMLNode * rootNode = xmlDoc.FirstChild();
    if (rootNode == nullptr) {
        std::cerr << "customization xml " << (name + ".limon") << " is not a valid XML." << std::endl;
        return;
    }

    tinyxml2::XMLElement* animationsNode =  rootNode->FirstChildElement("Animation");
    if (animationsNode == nullptr) {
        return;//a metadata file that only carries LOD calibration is normal, there is nothing to read here
    }
    tinyxml2::XMLElement* sectionNode =  animationsNode->FirstChildElement("Section");

    while(sectionNode != nullptr) {
        tinyxml2::XMLElement* baseNameNode = sectionNode->FirstChildElement("BaseName");
        if(baseNameNode == nullptr) {
            std::cerr << "Animation section without a base animation name can't be read. Animation loading not possible, skipping" << std::endl;
        } else {
            std::string baseName = baseNameNode->GetText();

            tinyxml2::XMLElement *animationNameNode = sectionNode->FirstChildElement("Name");
            if (animationNameNode == nullptr) {
                std::cerr << "Animation section name can't be read. Animation loading not possible, skipping"  << std::endl;
            } else {
                std::string animationSectionName = animationNameNode->GetText();
                tinyxml2::XMLElement *startTimeNode = sectionNode->FirstChildElement("StartTime");
                if (startTimeNode == nullptr) {
                    std::cerr << "Animation section start time can't be read. Animation loading not possible, skipping"  << std::endl;
                } else {
                    float startTime = std::stof(startTimeNode->GetText());

                    tinyxml2::XMLElement *endTimeNode = sectionNode->FirstChildElement("EndTime");
                    if (endTimeNode == nullptr) {
                        std::cerr << "Animation section end timecan't be read. Animation loading not possible, skipping"  << std::endl;
                    } else {
                        float endTime = std::stof(endTimeNode->GetText());
                        this->addAnimationAsSubSequence(baseName, animationSectionName, startTime, endTime);
                    }
                }
            }
        }
        sectionNode = sectionNode->NextSiblingElement("Section");
    } // end of while (Section)




}

int32_t ModelAsset::buildEditorBoneTree(int32_t selectedBoneNodeID, bool followSelection) {
   if(rootNode != nullptr) {
       return buildEditorBoneTreeRecursive(rootNode, selectedBoneNodeID, followSelection);
   }
   return -1;//not found
}

//Used only to decide whether an ancestor branch must be force-opened to reveal selectedBoneID (see followSelection
//below) -- true for the node itself too, so every ancestor level up to and including the selected node's parent
//gets expanded.
static bool subtreeContainsBone(const std::shared_ptr<BoneNode> &boneNode, int32_t targetBoneID) {
    if (boneNode == nullptr) {
        return false;
    }
    if (static_cast<int32_t>(boneNode->boneID) == targetBoneID) {
        return true;
    }
    for (const auto &child : boneNode->children) {
        if (subtreeContainsBone(child, targetBoneID)) {
            return true;
        }
    }
    return false;
}

int32_t ModelAsset::buildEditorBoneTreeRecursive(std::shared_ptr<BoneNode> boneNode, int32_t selectedBoneNodeID, bool followSelection) {
    int32_t result = -1;
    if(boneNode == nullptr) {
        return result;
    }
    bool isSelected = (selectedBoneNodeID == (int32_t)boneNode->boneID);
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

    if(boneNode->children.size() == 0) {
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx((boneNode->name + "##BoneTree" + std::to_string(boneNode->boneID)).c_str(), node_flags);
        if (followSelection && isSelected) {
            ImGui::SetScrollHereY();
        }
        if (ImGui::IsItemClicked()) {
            result = boneNode->boneID;
            return result;
        }
    } else {
        if (followSelection && subtreeContainsBone(boneNode, selectedBoneNodeID)) {
            //Force this branch open so navigating here from a preview-image click reveals the target node, even
            //if the user had this branch collapsed. Only forced for the one frame followSelection is true.
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }
        bool nodeOpen = ImGui::TreeNodeEx((boneNode->name + "##BoneTree" + std::to_string(boneNode->boneID)).c_str(), node_flags);
        if (followSelection && isSelected) {
            ImGui::SetScrollHereY();
        }
        if(nodeOpen) {
            if (ImGui::IsItemClicked()) {
                result = boneNode->boneID;
            }
            for (size_t i = 0; i < boneNode->children.size(); ++i) {
                result = std::max(buildEditorBoneTreeRecursive(boneNode->children[i], selectedBoneNodeID, followSelection), result);
            }
            ImGui::TreePop();
        } else {
            if (ImGui::IsItemClicked()) {
                result = boneNode->boneID;
            }
        }

    }
    return result;
}

void ModelAsset::collectBoneHierarchyEdgesRecursive(const std::shared_ptr<BoneNode> &boneNode, std::vector<std::pair<uint32_t, uint32_t>> &edges) const {
    if (boneNode == nullptr) {
        return;
    }
    //Every node's joint transform (see traverseAndSetJointTransform) is computed unconditionally, regardless of
    //boneInformationMap membership -- so every node in the hierarchy has a meaningful joint position, including
    //structural nodes (e.g. a spine/chest connector) with no direct skin weights. No filtering needed here.
    for (size_t i = 0; i < boneNode->children.size(); ++i) {
        edges.emplace_back(boneNode->children[i]->boneID, boneNode->boneID);
        collectBoneHierarchyEdgesRecursive(boneNode->children[i], edges);
    }
}

int32_t ModelAsset::findBoneIDByNameRecursive(const std::shared_ptr<BoneNode> &boneNode, const std::string &boneName) const {
    if (boneNode == nullptr) {
        return -1;
    }
    if (boneNode->name == boneName) {
        return static_cast<int32_t>(boneNode->boneID);
    }
    for (size_t i = 0; i < boneNode->children.size(); ++i) {
        int32_t foundBoneID = findBoneIDByNameRecursive(boneNode->children[i], boneName);
        if (foundBoneID != -1) {
            return foundBoneID;
        }
    }
    return -1;
}

void ModelAsset::buildPhysicsMeshes() {
    baseTransform.setIdentity();
    baseTransform.setOrigin(GLMConverter::GLMToBlt(-1.0f * centerOffset));

    std::vector<std::shared_ptr<MeshAsset>> physicalMeshes = getPhysicsMeshes();
        //we can end up here if mass is 0, but if mass is not 0, we will not be using this one but the other one, so we need both
        // For animated, we can't get the triangles, so we just get the convex hull
    // This shape can be used for animated objects, or dynamic physical objects(non zero mass).
    // If it is a static object (mass 0), we will use the convex tringle mesh shale
    compoundShapeForConvex = new btCompoundShape();

    for(auto iter = physicalMeshes.begin(); iter != physicalMeshes.end(); ++iter) {

        btTriangleMesh *rawCollisionMesh = (*iter)->getBulletMesh(&bulletHullMap, &bulletTransformMap);
        if (rawCollisionMesh != nullptr) {
            if (!this->isAnimated() ) {
                //btTriangleIndexVertexArray *indexArray = new btTriangleIndexVertexArray(rawCollisionMesh->getIndexedMeshArray()[0];
                btBvhTriangleMeshShape* bvhTriangleMeshShape = new btBvhTriangleMeshShape(rawCollisionMesh, true, true);
                meshCollisionShapesForTriangle.emplace_back(bvhTriangleMeshShape);

                //btConvexTriangleMeshShape only points at the striding mesh, and setLocalScaling writes the scale into
                //that mesh, so every shape sharing it (including the bvh one above) ends up with the last writer's
                //scale. Both branches below build a shape that owns its own points instead.
                btCollisionShape *meshCollisionShape;
                if (rawCollisionMesh->getNumTriangles() > 24) {
                    btConvexTriangleMeshShape *convexTriangleMeshShape = new btConvexTriangleMeshShape(rawCollisionMesh);
                    btShapeHull *hull = new btShapeHull(convexTriangleMeshShape);
                    hull->buildHull(convexTriangleMeshShape->getMargin());
                    meshCollisionShape = new btConvexHullShape(reinterpret_cast<const btScalar *>(hull->getVertexPointer()),
                                                               hull->numVertices());
                    delete hull;
                    delete convexTriangleMeshShape;
                } else {
                    //few enough points for the support function to walk them all, so we skip building a hull
                    const std::vector<glm::vec3> &meshVertices = (*iter)->getVertices();
                    meshCollisionShape = new btConvexHullShape(reinterpret_cast<const btScalar *>(meshVertices.data()),
                                                               (int) meshVertices.size(), sizeof(glm::vec3));
                }
                //a convex shape can not be bigger than the points it was built from, so if it is, the wrong points went in
                btVector3 shapeMin, shapeMax;
                meshCollisionShape->getAabb(btTransform::getIdentity(), shapeMin, shapeMax);
                //measured off the points themselves. getAabbMin/Max box the transformed source box, which a rotating
                //node transform makes larger than the mesh
                const std::vector<glm::vec3> &collisionPoints = (*iter)->getVertices();
                glm::vec3 meshMin(std::numeric_limits<float>::max());
                glm::vec3 meshMax(-std::numeric_limits<float>::max());
                for (size_t pointIndex = 0; pointIndex < collisionPoints.size(); ++pointIndex) {
                    meshMin = glm::min(meshMin, collisionPoints[pointIndex]);
                    meshMax = glm::max(meshMax, collisionPoints[pointIndex]);
                }
                btVector3 shapeSize = shapeMax - shapeMin;
                glm::vec3 meshSize = collisionPoints.empty() ? glm::vec3(0.0f) : meshMax - meshMin;
                float allowed = 1.2f;//margins and the hull's own padding make an exact match unreasonable
                if (shapeSize.x() > meshSize.x * allowed + 0.2f || shapeSize.y() > meshSize.y * allowed + 0.2f ||
                    shapeSize.z() > meshSize.z * allowed + 0.2f) {
                    std::cerr << "collision shape of " << name << " mesh " << (*iter)->getName()
                              << " is " << shapeSize.x() << "x" << shapeSize.y() << "x" << shapeSize.z()
                              << " while the mesh is " << meshSize.x << "x" << meshSize.y << "x" << meshSize.z
                              << ", triangles " << rawCollisionMesh->getNumTriangles() << std::endl;
                }
                compoundShapeForConvex->addChildShape(baseTransform, meshCollisionShape);
                reusableMeshes.emplace_back(meshCollisionShape);
            }
        }
    }

    if (this->isAnimated()) {
        std::map<uint32_t, btConvexHullShape *>::iterator it;
        for (unsigned int i = 0;i < 128; i++) {//FIXME 128 is the number of bones supported. It should be an option or an constant
            if (bulletTransformMap.find(i) != bulletTransformMap.end() && bulletHullMap.find(i) != bulletHullMap.end()) {
                boneIdCompoundChildMap[i] = compoundShapeForConvex->getNumChildShapes();//get numchild actually increase with each new child add below
                compoundShapeForConvex->addChildShape(bulletTransformMap[i], bulletHullMap[i]);//this add the mesh to collision shape, in order
            }
        }
    }
}

btCompoundShape * ModelAsset::getCompoundShapeForMass(uint32_t mass, std::map<uint32_t, uint32_t> &boneIdCompoundChildMap, std::vector<btCollisionShape *>& childrenShapes) {

    btCompoundShape *copyMesh =new btCompoundShape();
    if (!this->isAnimated() && mass==0) {
        for(size_t i= 0; i < meshCollisionShapesForTriangle.size(); ++i) {
            //the wrapper is per instance, the bvh it points to is shared and stays with the asset
            btScaledBvhTriangleMeshShape *scaledChild = new btScaledBvhTriangleMeshShape(
                    reinterpret_cast<btBvhTriangleMeshShape *>(meshCollisionShapesForTriangle[i]), btVector3(1, 1, 1));
            childrenShapes.emplace_back(scaledChild);
            copyMesh->addChildShape(baseTransform, scaledChild);
        }
    } else {

        for(int i= 0; i < compoundShapeForConvex->getNumChildShapes(); ++i) {
            btCollisionShape *newChild;
            //no convex triangle mesh shape here on purpose, copies of one would all share a single striding mesh
            if(compoundShapeForConvex->getChildShape(i)->getShapeType() == CONVEX_HULL_SHAPE_PROXYTYPE) {
                newChild = new btConvexHullShape(*(static_cast<btConvexHullShape *>(compoundShapeForConvex->getChildShape(i))));
            } else {
                newChild = new btBoxShape(btVector3(1,1,1));
                std::cerr << "Unknown type of shape found" << std::endl;
            }
            childrenShapes.emplace_back(newChild);
            copyMesh->addChildShape(baseTransform, newChild);
        }
    }
    if (this->isAnimated()) {
        for(auto it = this->boneIdCompoundChildMap.begin(); it != this->boneIdCompoundChildMap.end(); ++it) {
            boneIdCompoundChildMap[it->first] = it->second;
        }
    }

    return copyMesh;
}

ModelAsset::~ModelAsset() {
    //materialMap can have multiple names pointing at the same deduplicated Material (see registerMaterial()),
    // so unregister each distinct Material once, not once per name that happens to reference it.
    //Keyed by registrationID, the same identity the registry files them under. Safe because assignment is
    //deleted, so no two live materials can share one.
    std::unordered_set<uint32_t> unregisteredMaterials;
    for (auto materialIt: materialMap) {
        if (unregisteredMaterials.insert(materialIt.second->getRegistrationID()).second) {
            assetManager->getMaterialRegistry().unregisterMaterial(materialIt.second);
        }
    }

    for (unsigned int i = 0; i < shapeCopies.size(); ++i) {
        delete shapeCopies[i];
    }

    delete compoundShapeForConvex;

    for (btCollisionShape* shape:reusableMeshes) {
        delete shape;
    }

    for (btBvhTriangleMeshShape* shape:meshCollisionShapesForTriangle) {
        delete shape;
    }
    //FIXME GPU side is not freed
}