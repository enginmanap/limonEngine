//
// Created by engin on 31.08.2016.
//

#include <set>
#include "ModelAsset.h"
#include "../glm/gtx/matrix_decompose.hpp"
#include "../Utils/GLMUtils.h"
#include "Animations/AnimationAssimp.h"
#include "../GLHelper.h"

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
    std::cout << "ASSIMP::Loading::" << name << std::endl;
    const aiScene *scene;
    Assimp::Importer import;
    scene = import.ReadFile(name, aiProcess_FlipUVs
                                  | aiProcessPreset_TargetRealtime_MaxQuality);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        exit(-1);
    }

    aiMatrix4x4 m_GlobalInverseTransform = scene->mRootNode->mTransformation;
    m_GlobalInverseTransform.Inverse();

    globalInverseTransform = GLMConverter::AssimpToGLM(m_GlobalInverseTransform);

    this->hasAnimation = (scene->mNumAnimations != 0);

    std::cout << "ASSIMP::success::" << name << std::endl;

    if (!scene->HasMeshes()) {
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    } else {
        std::cout << "Model "<< this->name << " has " << scene->mNumMeshes << " mesh(es)." << std::endl;
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
    std::cout << "Model asset: " << name << "Assimp bounding box is " << GLMUtils::vectorToString(boundingBoxMin) << ", " <<  GLMUtils::vectorToString(boundingBoxMax) << std::endl;
    //Implicit call to import.FreeScene(), and removal of scene.
}


Material *ModelAsset::loadMaterials(const aiScene *scene, unsigned int materialIndex) {
    // create material uniform buffer
    aiMaterial *currentMaterial = scene->mMaterials[materialIndex];
    aiString property;    //contains filename of texture
    if (AI_SUCCESS != currentMaterial->Get(AI_MATKEY_NAME, property)) {
        std::cerr << "Material without a name is not handled." << std::endl;
        std::cerr << "Model " << this->name << std::endl;
        exit(-1);
        //return nullptr; should work too.
    }

    Material *newMaterial;
    if (materialMap.find(property.C_Str()) == materialMap.end()) {//search for the name
        //if the material is not loaded before
        newMaterial = new Material(assetManager, property.C_Str(), assetManager->getGlHelper()->getNextMaterialIndex());
        aiColor3D color(0.f, 0.f, 0.f);
        float transferFloat;

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
            newMaterial->setAmbientColor(GLMConverter::AssimpToGLM(color));
        }

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
            newMaterial->setDiffuseColor(GLMConverter::AssimpToGLM(color));
        }

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
            newMaterial->setSpecularColor(GLMConverter::AssimpToGLM(color));
        }

        if (AI_SUCCESS == currentMaterial->Get(AI_MATKEY_SHININESS, transferFloat)) {
            newMaterial->setSpecularExponent(transferFloat);
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_AMBIENT) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_AMBIENT, 0, &property)) {
                newMaterial->setAmbientTexture(property.C_Str());
                std::cout << "set ambient texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained ambient texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }
        if ((currentMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &property)) {
                newMaterial->setDiffuseTexture(property.C_Str());
                std::cout << "set diffuse texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained diffuse texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_SPECULAR, 0, &property)) {
                newMaterial->setSpecularTexture(property.C_Str());
                std::cout << "set specular texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained specular texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_OPACITY) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_OPACITY, 0, &property)) {
                newMaterial->setOpacityTexture(property.C_Str());
                std::cout << "set opacity texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained opacity texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        uint32_t maps = 0;
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

        assetManager->getGlHelper()->setMaterial(newMaterial);

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
            meshOffsetmap[currentMesh->mBones[j]->mName.C_Str()] = GLMConverter::AssimpToGLM(
                    currentMesh->mBones[j]->mOffsetMatrix);
            std::string boneName =  currentMesh->mBones[j]->mName.C_Str();
            boneName += "_parent";
            meshOffsetmap[boneName] = parentTransform;
        }
        if(currentMesh->mNumBones == 0 && hasAnimation) {
            //If animated, but a mesh without any bone exits, we should process that mesh specially
            //meshOffsetmap[aiNode->mName.C_Str()] = GLMConverter::AssimpToGLM(aiNode->mTransformation);
            //meshOffsetmap[aiNode->mName.C_Str()] = parentTransform;
            meshOffsetmap[aiNode->mName.C_Str()] = glm::mat4(1.0f);
            std::string boneName =  aiNode->mName.C_Str();
            boneName += "_parent";
            //meshOffsetmap[boneName] = GLMConverter::AssimpToGLM(aiNode->mTransformation);
            meshOffsetmap[boneName] = glm::mat4(1.0f);
        }

        Material *meshMaterial = loadMaterials(scene, currentMesh->mMaterialIndex);
        MeshAsset *mesh;
        try {
            mesh = new MeshAsset(assetManager, currentMesh, aiNode->mName.C_Str(), meshMaterial, rootNode,
                                            parentTransform, hasAnimation);
        } catch(...) {
            continue;
        }

        if(!strncmp(aiNode->mName.C_Str(), "UCX_", strlen("UCX_"))) {
            //if starts with "UCX_"
            simplifiedMeshes[mesh->getName()] = mesh;
            std::cout << "simplified mesh " << currentMesh->mName.C_Str() << " for node " << aiNode->mName.C_Str()
                      << std::endl;
        } else {

            if (meshMaterial->hasOpacityMap()) {
                meshes.push_back(mesh);
            } else {
                meshes.insert(meshes.begin(), mesh);
            }
            std::cout << "set mesh " << currentMesh->mName.C_Str() << " for node " << aiNode->mName.C_Str()
                      << std::endl;
        }

    }

    for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
        createMeshes(scene, aiNode->mChildren[i], parentTransform);
    }
}

BoneNode* ModelAsset::loadNodeTree(aiNode *aiNode) {
    BoneNode *currentNode = new BoneNode();
    currentNode->name = aiNode->mName.C_Str();
    currentNode->boneID = boneIDCounter++;
    currentNode->transformation = GLMConverter::AssimpToGLM(aiNode->mTransformation);
    for (unsigned int i = 0; i < aiNode->mNumChildren; ++i) {
        currentNode->children.push_back(loadNodeTree(aiNode->mChildren[i]));
    }
    return currentNode;
}

bool ModelAsset::findNode(const std::string &nodeName, BoneNode** foundNode, BoneNode* searchRoot) const {
    if (nodeName == searchRoot->name) {
        *foundNode = searchRoot;
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

void ModelAsset::getTransform(long time, std::string animationName, std::vector<glm::mat4> &transformMatrix) const {
/*
    for(auto it = animations.begin(); it != animations.end(); it++) {
        std::cout << "Animations name: " << it->first << " size " << animations.size() <<std::endl;
    }
    */
    if(animationName.empty() || animationName == "") {
        //this means return to bind pose
        //FIXME calculating bind pose for each frame is wrong, but I am assuming this part will be removed, and idle pose
        //will be used instead. If bind pose requirement arises, it should set once, and reused.
        for(std::unordered_map<std::string, glm::mat4>::const_iterator it = meshOffsetmap.begin(); it != meshOffsetmap.end(); it++){
            BoneNode* node;
            std::string name = it->first;
            if(findNode(name, &node, rootNode)) {
                transformMatrix[node->boneID] = meshOffsetmap.at(node->name + "_parent");
                //parent above means parent transform of the mesh node, not the parent of bone.
            }
        }
        std::cout << "bind pose returned. for animation name [" << animationName << "]"<< std::endl;
        return;
    }

    const AnimationAssimp *currentAnimation;
    if(animations.find(animationName) != animations.end()) {
        currentAnimation = animations.at(animationName);
    } else {
        std::cerr << "Animation " << animationName << " not found, playing first animation. " << std::endl;
        currentAnimation = animations.begin()->second;
    }


    float ticksPerSecond;
    if (currentAnimation->getTicksPerSecond() != 0) {
        ticksPerSecond = currentAnimation->getTicksPerSecond();
    } else {
        ticksPerSecond = 60.0f;
    }

    float animationTime = fmod((time / 1000.0f) * ticksPerSecond, currentAnimation->getDuration());
    glm::mat4 parentTransform(1.0f);
    traverseAndSetTransform(rootNode, parentTransform, currentAnimation, animationTime, transformMatrix);
}

void
ModelAsset::traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform,
                                    const AnimationAssimp *animation,
                                    float timeInTicks,
                                    std::vector<glm::mat4> &transforms) const {
/*
    for(auto it = animation->nodes.begin(); it != animation->nodes.end(); it++) {
        std::cout << "Animation node name: " << it->first << std::endl;
    }
*/
    glm::mat4 nodeTransform;
    bool isFound;
    nodeTransform = animation->calculateTransform(boneNode->name, timeInTicks, isFound);

    if(!isFound) {
        nodeTransform = boneNode->transformation;
    }

    nodeTransform = parentTransform * nodeTransform;

    if(meshOffsetmap.find(boneNode->name) != meshOffsetmap.end()) {
        transforms[boneNode->boneID] =
                globalInverseTransform * meshOffsetmap.at(boneNode->name + "_parent") * nodeTransform * meshOffsetmap.at(boneNode->name);
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

void ModelAsset::fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations) {
    aiAnimation* currentAnimation;
    for (unsigned int i = 0; i < numAnimation; ++i) {
        currentAnimation = pAnimations[i];
        std::string animationName = currentAnimation->mName.C_Str();
        std::cout << "add animation with name " << animationName << std::endl;

        AnimationAssimp* animationObject = new AnimationAssimp(currentAnimation);
        animations[animationName] = animationObject;
    }

    //validate
}

