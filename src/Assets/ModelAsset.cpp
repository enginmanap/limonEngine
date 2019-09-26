//
// Created by engin on 31.08.2016.
//

#include <set>

#include "ModelAsset.h"
#include "../glm/gtx/matrix_decompose.hpp"
#include "../Utils/GLMUtils.h"
#include "Animations/AnimationAssimp.h"
#include "API/GraphicsInterface.h"
#include "Animations/AnimationAssimpSection.h"
#include "../../libs/ImGui/imgui.h"

ModelAsset::ModelAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &fileList)
        : Asset(assetManager, assetID,
                fileList),
          boneIDCounter(0),
          boneIDCounterPerMesh(0) {
    if (fileList.empty()) {
        std::cerr << "Model load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = fileList[0];
    if (fileList.size() > 1) {
        std::cerr << "multiple files are sent to Model constructor, extra elements ignored." << std::endl;
    }
    //std::cout << "ASSIMP::Loading::" << name << std::endl;
    const aiScene *scene;
    Assimp::Importer import;
    unsigned int flags = (aiProcess_FlipUVs | aiProcessPreset_TargetRealtime_MaxQuality);
#ifdef ASSIMP_VALIDATE_WORKAROUND
    flags = flags & ~aiProcess_FindInvalidData;
#endif
    scene = import.ReadFile(name, flags);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        exit(-1);
    }

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
    if(textures.size() > 0 ) {
        assetManager->addEmbeddedTextures(this->name, textures);
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

    createMeshes(scene, scene->mRootNode, glm::mat4(1.0f));
    if(this->hasAnimation) {
        fillAnimationSet(scene->mNumAnimations, scene->mAnimations);
    }
    aiVector3D min, max;
    AssimpUtils::get_bounding_box(scene, &min, &max);
    boundingBoxMax = GLMConverter::AssimpToGLM(max);
    boundingBoxMin = GLMConverter::AssimpToGLM(min);
    centerOffset = glm::vec3((max.x + min.x) / 2, (max.y + min.y) / 2, (max.z + min.z) / 2);
    //std::cout << "Model asset: " << name << "Assimp bounding box is " << GLMUtils::vectorToString(boundingBoxMin) << ", " <<  GLMUtils::vectorToString(boundingBoxMax) << std::endl;
    //Implicit call to import.FreeScene(), and removal of scene.

    //it is possible that there are mixamo animation files, check if they do, add them too if needed
    const AssetManager::AvailableAssetsNode* availableAssetsTree = assetManager->getAvailableAssetsTree();

    //first find node of the object itself
    const AssetManager::AvailableAssetsNode* thisAssetsNode = availableAssetsTree->findNode(this->name);

    if(thisAssetsNode == nullptr) {
        std::cerr << "The asset " << this->name << " is not in asset tree, skipping Mixamo search" << std::endl;
    } else {
        AssetManager::AvailableAssetsNode* parentNode = thisAssetsNode->parent;
        for (auto child = parentNode->children.begin(); child != parentNode->children.end(); ++child) {
            if((*child)->name == "Mixamo") {
                std::cout << "searching mixamo animations at path: " << (*child)->fullPath << std::endl;
                Assimp::Importer importer2;
                const aiScene *mixamoScene = nullptr;
                for (auto mixamoFile = (*child)->children.begin(); mixamoFile != (*child)->children.end(); ++mixamoFile) {
                    if((*mixamoFile)->assetType == AssetManager::AssetTypes::Asset_type_MODEL) {
                        mixamoScene = importer2.ReadFile((*mixamoFile)->fullPath, 0);
                        if (!mixamoScene || !mixamoScene->mRootNode) {//it might have mixamoScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE, but that is expected
                            std::cerr << "ERROR::ASSIMP::MIXAMO" << importer2.GetErrorString() << std::endl;
                            //delete mixamoScene; don't delete, importer deletes
                            continue;
                        }
                        if (mixamoScene->mNumAnimations != 0) {
                            //get the file name
                            fillAnimationSet(mixamoScene->mNumAnimations, mixamoScene->mAnimations, (*mixamoFile)->name.substr(0, (*mixamoFile)->name.find_last_of("."))+ "|");
                        } else {
                            std::cout << "No animation in Mixamo file, it won't effect anyting." << std::endl;
                        }
                    }

                }
            }
        }
    }

    this->deserializeCustomizations();
}


void ModelAsset::afterDeserialize(AssetManager *assetManager, std::vector<std::string> files) {
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

    this->assetManager = assetManager;
    if (files.empty()) {
        std::cerr << "Model load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    this->name = files[0];

    if(temporaryEmbeddedTextures->size() > 0 ) {
        assetManager->addEmbeddedTextures(this->name, *temporaryEmbeddedTextures);
    }
    temporaryEmbeddedTextures.reset();

    for (auto material = materialMap.begin(); material != materialMap.end(); ++material) {
        material->second->afterDeserialize(assetManager, name);
        assetManager->getGraphicsWrapper()->setMaterial(material->second);
    }

    for (auto mesh = meshes.begin(); mesh != meshes.end(); ++mesh) {
        (*mesh)->afterDeserialize(assetManager);
    }
}


std::shared_ptr<Material>ModelAsset::loadMaterials(const aiScene *scene, unsigned int materialIndex) {
    // create material uniform buffer
    aiMaterial *currentMaterial = scene->mMaterials[materialIndex];
    aiString property;    //contains filename of texture
    if (AI_SUCCESS != currentMaterial->Get(AI_MATKEY_NAME, property)) {
        std::cerr << "Material without a name is not handled." << std::endl;
        std::cerr << "Model " << this->name << std::endl;
        exit(-1);
        //return nullptr; should work too.
    }

    std::shared_ptr<Material> newMaterial;
    if (materialMap.find(property.C_Str()) == materialMap.end()) {//search for the name
        //if the material is not loaded before
        newMaterial = std::make_shared<Material>(assetManager, property.C_Str(), assetManager->getGraphicsWrapper()->getNextMaterialIndex());
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
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_AMBIENT, 0, &property)) {
                if(property.data[0] != '*') {
                    newMaterial->setAmbientTexture(property.C_Str());
                    std::cout << "set ambient texture " << property.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setAmbientTexture(property.C_Str(), &this->name);
                    std::cout << "set (embedded) ambient texture " << property.C_Str() << "|" << this->name<< std::endl;
                }

            } else {
                std::cerr << "The model contained ambient texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }
        if ((currentMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &property)) {
                if(property.data[0] != '*') {
                    newMaterial->setDiffuseTexture(property.C_Str());
                    //std::cout << "set diffuse texture " << property.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setDiffuseTexture(property.C_Str(), &this->name);
                    //std::cout << "set (embedded) setDiffuseTexture texture " << property.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained diffuse texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_SPECULAR, 0, &property)) {
                if(property.data[0] != '*') {
                    newMaterial->setSpecularTexture(property.C_Str());
                    std::cout << "set specular texture " << property.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setSpecularTexture(property.C_Str(), &this->name);
                    std::cout << "set (embedded) setSpecularTexture texture " << property.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained specular texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_NORMALS) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_NORMALS, 0, &property)) {
                if(property.data[0] != '*') {
                    newMaterial->setNormalTexture(property.C_Str());
                    std::cout << "set normal texture " << property.C_Str() << std::endl;
                } else {
                    //embeddedTexture handling
                    newMaterial->setNormalTexture(property.C_Str(), &this->name);
                    std::cout << "set (embedded) setNormalTexture texture " << property.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained normal texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }


        if ((currentMaterial->GetTextureCount(aiTextureType_OPACITY) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_OPACITY, 0, &property)) {
                if(property.data[0] != '*') {
                    newMaterial->setOpacityTexture(property.C_Str());
                } else {
                    //embeddedTexture handling
                    newMaterial->setOpacityTexture(property.C_Str(), &this->name);
                    std::cout << "set (embedded) setOpacityTexture texture " << property.C_Str() << "|" << this->name<< std::endl;
                }
            } else {
                std::cerr << "The model contained opacity texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
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

        assetManager->getGraphicsWrapper()->setMaterial(newMaterial);

        materialMap[newMaterial->getName()] = newMaterial;
    } else {
        newMaterial = materialMap[property.C_Str()];
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

        std::shared_ptr<Material>meshMaterial = loadMaterials(scene, currentMesh->mMaterialIndex);
        std::shared_ptr<MeshAsset> mesh;
        try {
            mesh = std::make_shared<MeshAsset>(assetManager, currentMesh, aiNode->mName.C_Str(), meshMaterial, rootNode,
                                            parentTransform, hasAnimation);
        } catch(...) {
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
bool ModelAsset::getTransformBlended(std::string animationNameOld, long timeOld, bool loopedOld,
                                     std::string animationNameNew, long timeNew, bool loopedNew,
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
        std::cout << "bind pose returned. for animation name [" << animationNameOld << "]"<< std::endl;
        return true;
    }

    std::shared_ptr<const AnimationInterface> currentAnimationOld = nullptr;
    float animationTimeOld = 0.0f;
    bool isFinishedOld = false;
    if(!(animationNameOld.empty() ||animationNameOld == "")) {
        if (animations.find(animationNameOld) != animations.end()) {
            currentAnimationOld = animations.at(animationNameOld);
        } else {
            std::cerr << "Animation " << animationNameOld << " not found, playing first animation. " << std::endl;
            currentAnimationOld = animations.begin()->second;
        }

        float ticksPerSecond;
        if (currentAnimationOld->getTicksPerSecond() != 0) {
            ticksPerSecond = currentAnimationOld->getTicksPerSecond();
        } else {
            ticksPerSecond = 60.0f;
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
            std::cerr << "Animation " << animationNameNew << " not found, playing first animation. " << std::endl;
            currentAnimationNew = animations.begin()->second;
        }

        float ticksPerSecond;
        if (currentAnimationNew->getTicksPerSecond() != 0) {
            ticksPerSecond = currentAnimationNew->getTicksPerSecond();
        } else {
            ticksPerSecond = 60.0f;
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
bool ModelAsset::getTransform(long time, bool looped, std::string animationName, std::vector<glm::mat4> &transformMatrix) const {
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
        std::cout << "bind pose returned. for animation name [" << animationName << "]"<< std::endl;
        return true;
    }

    std::shared_ptr<const AnimationInterface> currentAnimation;
    if(animations.find(animationName) != animations.end()) {
        currentAnimation = animations.at(animationName);
    } else {
        std::cerr << "Animation " << animationName << " not found, playing first animation. " << std::endl;
        currentAnimation = animations.begin()->second;
    }

    float animationTime;
    float ticksPerSecond;
    if (currentAnimation->getTicksPerSecond() != 0) {
        ticksPerSecond = currentAnimation->getTicksPerSecond();
    } else {
        ticksPerSecond = 60.0f;
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
        blended.setTranslate(tf1.getTranslate() + blendFactor * translateDelta);

        blended.setOrientation(glm::normalize(slerp(tf1.getOrientation(), tf2.getOrientation(), blendFactor)));

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

bool ModelAsset::isAnimated() const {
    return hasAnimation;
}

void
ModelAsset::fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations, const std::string &animationNamePrefix) {
    aiAnimation* currentAnimation;
    for (unsigned int i = 0; i < numAnimation; ++i) {
        currentAnimation = pAnimations[i];
        std::string animationName = animationNamePrefix + currentAnimation->mName.C_Str();
        std::cout << "add animation with name " << animationNamePrefix << animationName << std::endl;

        std::shared_ptr<AnimationAssimp> animationObject = std::make_shared<AnimationAssimp>(currentAnimation);
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

void ModelAsset::serializeCustomizations() {
    if(!customizationAfterSave) {
        //Since assets are shared, serialize will be called multiple times. This flag is just a block for that.
        return;
    }
    tinyxml2::XMLDocument customizationDocument;
    tinyxml2::XMLNode * rootNode = customizationDocument.NewElement("ModelCustomizations");
    customizationDocument.InsertFirstChild(rootNode);

    tinyxml2::XMLElement * animationsSectionsNode = customizationDocument.NewElement("Animation");
    rootNode->InsertEndChild(animationsSectionsNode);
    for(auto it=this->animationSections.begin(); it != this->animationSections.end(); it++) {
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

    tinyxml2::XMLError eResult = customizationDocument.SaveFile((name + ".limon").c_str());
    if(eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "ERROR saving model customization: " << eResult << std::endl;
    } else {
        customizationAfterSave = false;
    }
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
        std::cerr << "Customizations must have a Animation field." << std::endl;
        return;
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

int32_t ModelAsset::buildEditorBoneTree(int32_t selectedBoneNodeID) {
   if(rootNode != nullptr) {
       return buildEditorBoneTreeRecursive(rootNode, selectedBoneNodeID);
   }
   return -1;//not found
}

int32_t ModelAsset::buildEditorBoneTreeRecursive(std::shared_ptr<BoneNode> boneNode, int32_t selectedBoneNodeID) {
    int32_t result = -1;
    if(boneNode == nullptr) {
        return result;
    }
    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ((selectedBoneNodeID == (int32_t)boneNode->boneID) ? ImGuiTreeNodeFlags_Selected : 0);

    if(boneNode->children.size() == 0) {
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx((boneNode->name + "##BoneTree" + std::to_string(boneNode->boneID)).c_str(), node_flags);
        if (ImGui::IsItemClicked()) {
            result = boneNode->boneID;
            return result;
        }
    } else {
        if(ImGui::TreeNodeEx((boneNode->name + "##BoneTree" + std::to_string(boneNode->boneID)).c_str(), node_flags)) {
            if (ImGui::IsItemClicked()) {
                result = boneNode->boneID;
            }
            for (size_t i = 0; i < boneNode->children.size(); ++i) {
                result = std::max(buildEditorBoneTreeRecursive(boneNode->children[i], selectedBoneNodeID), result);
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

bool ModelAsset::isTransparent() const {
    if(isAnimated()) {
        return false;
    }
    return transparentMaterialUsed;
}
