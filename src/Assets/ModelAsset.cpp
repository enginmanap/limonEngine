//
// Created by engin on 31.08.2016.
//

#include <set>
#include "ModelAsset.h"
#include "../glm/gtx/matrix_decompose.hpp"
#include "../Utils/GLMUtils.h"
#include "Animations/AnimationAssimp.h"
#include "../GLHelper.h"
#include "Animations/AnimationAssimpSection.h"

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

    this->deserializeCustomizations();
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

void ModelAsset::getTransform(long time, bool looped, std::string animationName, std::vector<glm::mat4> &transformMatrix) const {
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
            BoneNode* node;
            std::string name = it->first;
            if(findNode(name, &node, rootNode)) {
                transformMatrix[node->boneID] = boneInformationMap.at(node->name).parentOffset;
                //parent above means parent transform of the mesh node, not the parent of bone.
            }
        }
        std::cout << "bind pose returned. for animation name [" << animationName << "]"<< std::endl;
        return;
    }

    const AnimationInterface *currentAnimation;
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

    float requestedTime = (time / 1000.0f) * ticksPerSecond;
    if(requestedTime < currentAnimation->getDuration()) {
        animationTime = requestedTime;
    } else {
        if (looped) {
            animationTime = fmod(requestedTime, currentAnimation->getDuration());
        } else {
            animationTime = currentAnimation->getDuration();
        }
    }

    glm::mat4 parentTransform(1.0f);
    traverseAndSetTransform(rootNode, parentTransform, currentAnimation, animationTime, transformMatrix);
}

void
ModelAsset::traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform,
                                    const AnimationInterface *animation,
                                    float timeInTicks,
                                    std::vector<glm::mat4> &transforms) const {
/*
    for(auto it = animation->nodes.begin(); it != animation->nodes.end(); it++) {
        std::cout << "Animation node name: " << it->first << std::endl;
    }
*/
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

bool ModelAsset::addAnimationAsSubSequence(const std::string &baseAnimationName, const std::string newAnimationName,
                                           float startTime, float endTime) {
    if(this->animations.find(baseAnimationName) == this->animations.end()) {
        //base animation not found
        return false;
    }
    AnimationInterface* animationAssimp = this->animations[baseAnimationName];
    AnimationAssimpSection* animation = new AnimationAssimpSection(animationAssimp, startTime, endTime);

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
