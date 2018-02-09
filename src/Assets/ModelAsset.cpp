//
// Created by engin on 31.08.2016.
//

#include <set>
#include "ModelAsset.h"
#include "../glm/gtx/matrix_decompose.hpp"
#include "../Utils/GLMUtils.h"

ModelAsset::ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList) : Asset(assetManager,
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
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
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
        newMaterial = new Material(assetManager, property.C_Str());
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
                std::cout << "loaded ambient texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained ambient texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }
        if ((currentMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &property)) {
                newMaterial->setDiffuseTexture(property.C_Str());
                std::cout << "loaded diffuse texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained diffuse texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_SPECULAR, 0, &property)) {
                newMaterial->setSpecularTexture(property.C_Str());
                std::cout << "loaded specular texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained specular texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

        if ((currentMaterial->GetTextureCount(aiTextureType_OPACITY) > 0)) {
            if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_OPACITY, 0, &property)) {
                newMaterial->setOpacityTexture(property.C_Str());
                std::cout << "loaded opacity texture " << property.C_Str() << std::endl;
            } else {
                std::cerr << "The model contained opacity texture information, but texture loading failed. \n" <<
                          "TextureAsset path: [" << property.C_Str() << "]" << std::endl;
            }
        }

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

            Material *meshMaterial = loadMaterials(scene, currentMesh->mMaterialIndex);
            MeshAsset *mesh = new MeshAsset(assetManager, currentMesh, meshMaterial, rootNode, parentTransform, hasAnimation);
            //FIXME the exception thrown from new is not catched
            if(meshMaterial->hasOpacityMap()) {
                meshes.push_back(mesh);
            } else {
                meshes.insert(meshes.begin(),mesh);
            }
            std::cout << "loaded mesh " << currentMesh->mName.C_Str() << " for node " << aiNode->mName.C_Str()
                      << std::endl;
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
        for(std::map<std::string, glm::mat4>::const_iterator it = meshOffsetmap.begin(); it != meshOffsetmap.end(); it++){
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

    const AnimationSet *currentAnimation;
    if(animations.find(animationName) != animations.end()) {
        currentAnimation = animations.at(animationName);
    } else {
        std::cerr << "Animation " << animationName << " not found, playing first animation. " << std::endl;
        currentAnimation = animations.begin()->second;
    }
    
    
    float ticksPerSecond;
    if (currentAnimation->ticksPerSecond != 0) {
        ticksPerSecond = currentAnimation->ticksPerSecond;
    } else {
        ticksPerSecond = 60.0f;
    }

    float animationTime = fmod((time / 1000.0f) * ticksPerSecond, currentAnimation->duration);
    glm::mat4 parentTransform(1.0f);
    traverseAndSetTransform(rootNode, parentTransform, currentAnimation, animationTime, transformMatrix);

}

void
ModelAsset::traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform, const AnimationSet *animation,
                                    float timeInTicks,
                                    std::vector<glm::mat4> &transforms) const {
/*
    for(auto it = animation->nodes.begin(); it != animation->nodes.end(); it++) {
        std::cout << "Animation node name: " << it->first << std::endl;
    }
*/
    glm::mat4 nodeTransform;

    if (animation->nodes.find(boneNode->name) == animation->nodes.end()) {//if the bone has no animation, it can happen
        nodeTransform = boneNode->transformation;
    } else {
        // Interpolate scaling and generate scaling transformation matrix
        AnimationNode *nodeAnimation = animation->nodes.at(boneNode->name);
        nodeTransform = calculateTransform(nodeAnimation, timeInTicks);
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

glm::vec3 ModelAsset::getPositionVector(const float timeInTicks, const AnimationNode *nodeAnimation) const {
    glm::vec3 transformVector;
    if (nodeAnimation->translates.size() == 1) {
            transformVector = nodeAnimation->translates[0];
        } else {
            unsigned int positionIndex = 0;
            for (unsigned int i = 0; i < nodeAnimation->translates.size(); i++) {
                if (timeInTicks < nodeAnimation->translateTimes[i + 1]) {
                    positionIndex = i;
                    break;
                }
            }

            unsigned int NextPositionIndex = (positionIndex + 1);
            assert(NextPositionIndex < nodeAnimation->translates.size());
            float DeltaTime = (float) (nodeAnimation->translateTimes[NextPositionIndex] -
                                       nodeAnimation->translateTimes[positionIndex]);
            float Factor = (timeInTicks - (float) nodeAnimation->translateTimes[positionIndex]) / DeltaTime;
            assert(Factor >= 0.0f && Factor <= 1.0f);
            const glm::vec3 &Start = nodeAnimation->translates[positionIndex];
            const glm::vec3 &End = nodeAnimation->translates[NextPositionIndex];
            glm::vec3 Delta = End - Start;
            transformVector = Start + Factor * Delta;
        }
    return transformVector;
}

glm::vec3 ModelAsset::getScalingVector(const float timeInTicks, const AnimationNode *nodeAnimation) const {
    glm::vec3 scalingTransformVector;
    if (nodeAnimation->scales.size() == 1) {
            scalingTransformVector = nodeAnimation->scales[0];
        } else {
            unsigned int ScalingIndex = 0;
            assert(nodeAnimation->scales.size() > 0);
            for (unsigned int i = 0; i < nodeAnimation->scales.size(); i++) {
                if (timeInTicks < nodeAnimation->scaleTimes[i + 1]) {
                    ScalingIndex = i;
                    break;
                }
            }


            unsigned int NextScalingIndex = (ScalingIndex + 1);
            assert(NextScalingIndex < nodeAnimation->scales.size());
            float DeltaTime = (nodeAnimation->scaleTimes[NextScalingIndex] -
                                       nodeAnimation->scaleTimes[ScalingIndex]);
            float Factor = (timeInTicks - (float) nodeAnimation->scaleTimes[ScalingIndex]) / DeltaTime;
            assert(Factor >= 0.0f && Factor <= 1.0f);
            const glm::vec3 &Start = nodeAnimation->scales[ScalingIndex];
            const glm::vec3 &End = nodeAnimation->scales[NextScalingIndex];
            glm::vec3 Delta = End - Start;
            scalingTransformVector = Start + Factor * Delta;
        }
    return scalingTransformVector;
}

glm::quat ModelAsset::getRotationQuat(const float timeInTicks, const AnimationNode *nodeAnimation) const {
    glm::quat rotationTransformQuaternion;
    if (nodeAnimation->rotations.size() == 1) {
        rotationTransformQuaternion = nodeAnimation->rotations[0];
        } else {

            int rotationIndex = 0;

            assert(nodeAnimation->rotations.size() > 0);

            for (unsigned int i = 0; i < nodeAnimation->rotations.size(); i++) {
                if (timeInTicks < (float) nodeAnimation->rotationTimes[i + 1]) {
                    rotationIndex = i;
                    break;
                }
            }

            unsigned int NextRotationIndex = (rotationIndex + 1);
            assert(NextRotationIndex < nodeAnimation->rotations.size());
            float DeltaTime = (nodeAnimation->rotationTimes[NextRotationIndex] -
                                       nodeAnimation->rotationTimes[rotationIndex]);
            float Factor = (timeInTicks - (float) nodeAnimation->rotationTimes[rotationIndex]) / DeltaTime;
            assert(Factor >= 0.0f && Factor <= 1.0f);
            const glm::quat &StartRotationQ = nodeAnimation->rotations[rotationIndex];
            const glm::quat &EndRotationQ = nodeAnimation->rotations[NextRotationIndex];
            rotationTransformQuaternion = glm::normalize(glm::slerp(StartRotationQ, EndRotationQ, Factor));
        }
    return rotationTransformQuaternion;
}

bool ModelAsset::isAnimated() const {
    return hasAnimation;
}

void ModelAsset::fillAnimationSet(unsigned int numAnimation, aiAnimation **pAnimations) {
    aiAnimation* currentAnimation;
    for (unsigned int i = 0; i < numAnimation; ++i) {
        currentAnimation = pAnimations[i];
        std::string animationName = currentAnimation->mName.C_Str();
        std::cerr << "add animation with name " << animationName << std::endl;

        AnimationSet* animationSet = new AnimationSet();
        animationSet->duration = currentAnimation->mDuration;
        animationSet->ticksPerSecond = currentAnimation->mTicksPerSecond;
        //create and attach AnimationNodes
        for (unsigned int j = 0; j < currentAnimation->mNumChannels; ++j) {
            AnimationNode* node = new AnimationNode();
            for (unsigned int k = 0; k < currentAnimation->mChannels[j]->mNumPositionKeys; ++k) {
                node->translates.push_back(glm::vec3(
                        currentAnimation->mChannels[j]->mPositionKeys[k].mValue.x,
                        currentAnimation->mChannels[j]->mPositionKeys[k].mValue.y,
                        currentAnimation->mChannels[j]->mPositionKeys[k].mValue.z));
                node->translateTimes.push_back(currentAnimation->mChannels[j]->mPositionKeys[k].mTime);
            }

            for (unsigned int k = 0; k < currentAnimation->mChannels[j]->mNumScalingKeys; ++k) {
                node->scales.push_back(glm::vec3(
                        currentAnimation->mChannels[j]->mScalingKeys[k].mValue.x,
                        currentAnimation->mChannels[j]->mScalingKeys[k].mValue.y,
                        currentAnimation->mChannels[j]->mScalingKeys[k].mValue.z));
                node->scaleTimes.push_back(currentAnimation->mChannels[j]->mScalingKeys[k].mTime);
            }

            for (unsigned int k = 0; k < currentAnimation->mChannels[j]->mNumRotationKeys; ++k) {
                node->rotations.push_back(glm::quat(
                        currentAnimation->mChannels[j]->mRotationKeys[k].mValue.w,
                        currentAnimation->mChannels[j]->mRotationKeys[k].mValue.x,
                        currentAnimation->mChannels[j]->mRotationKeys[k].mValue.y,
                        currentAnimation->mChannels[j]->mRotationKeys[k].mValue.z));
                node->rotationTimes.push_back(currentAnimation->mChannels[j]->mRotationKeys[k].mTime);
            }
            animationSet->nodes[currentAnimation->mChannels[j]->mNodeName.C_Str()] = node;
        }
        animations[animationName] = animationSet;
    }

    //validate
}


glm::mat4 ModelAsset::calculateTransform(AnimationNode *animation, float time) const {
//this method can benefit from move and also reusing the intermediate matrices
    glm::vec3 scalingTransformVector, transformVector;
    glm::quat rotationTransformQuaternion;

    scalingTransformVector = getScalingVector(time, animation);
    rotationTransformQuaternion = getRotationQuat(time, animation);
    transformVector = getPositionVector(time, animation);

    glm::mat4 rotationMatrix = glm::mat4_cast(rotationTransformQuaternion);
    glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), transformVector);
    glm::mat4 scaleTransform = glm::scale(glm::mat4(1.0f), scalingTransformVector);
    return translateMatrix * rotationMatrix * scaleTransform;
}