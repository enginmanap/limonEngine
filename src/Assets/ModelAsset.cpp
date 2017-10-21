//
// Created by engin on 31.08.2016.
//

#include "ModelAsset.h"
#include "../glm/gtx/matrix_decompose.hpp"


ModelAsset::ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList) : Asset(assetManager,
                                                                                                     fileList),
                                                                                               boneIDCounter(0),
                                                                                               boneIDCounterPerMesh(0) {
    if (fileList.size() < 1) {
        std::cerr << "Model load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = fileList[0];
    if (fileList.size() > 1) {
        std::cerr << "multiple files are sent to Model constructor, extra elements ignored." << std::endl;
    }
    std::cout << "ASSIMP::Loading::" << name << std::endl;
    //FIXME triangulate creates too many vertices, it is unnecessary, but optimize requires some work.
    scene = import.ReadFile(name, aiProcess_FlipUVs |
                                  aiProcessPreset_TargetRealtime_MaxQuality);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }

    aiMatrix4x4 m_GlobalInverseTransform = scene->mRootNode->mTransformation;
    m_GlobalInverseTransform.Inverse();

    globalInverseTransform = GLMConverter::AssimpToGLM(m_GlobalInverseTransform);

    animations = scene->mAnimations;//FIXME this is not acceptable, requires fixing.

    if (scene->mNumAnimations == 0) {
        this->hasAnimation = false;
    } else {
        this->hasAnimation = true;
    }

    std::cout << "ASSIMP::success::" << name << std::endl;

    if (!scene->HasMeshes()) {
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    } else {
        std::cout << "Model "<< this->name << " has " << scene->mNumMeshes << " mesh(es)." << std::endl;
    }

    this->rootNode = loadNodeTree(scene->mRootNode);

    createMeshes(scene->mRootNode, glm::mat4(1.0f));

    aiVector3D min, max;
    AssimpUtils::get_bounding_box(scene, &min, &max);
    boundingBoxMax = GLMConverter::AssimpToGLM(max);
    boundingBoxMin = GLMConverter::AssimpToGLM(min);

    centerOffset = glm::vec3((max.x + min.x) / 2, (max.y + min.y) / 2, (max.z + min.z) / 2);
}


Material *ModelAsset::loadMaterials(const aiScene *scene, unsigned int materialIndex) {

    // create material uniform buffer
    aiMaterial *currentMaterial = scene->mMaterials[materialIndex];
    aiString property;    //contains filename of texture
    if (AI_SUCCESS != currentMaterial->Get(AI_MATKEY_NAME, property)) {
        std::cerr << "Material without a name is not handled." << std::endl;
        std::cerr << "Model " << this->name << std::endl;
        exit(-1);
        //return NULL; should work too.
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

        materialMap[newMaterial->getName()] = newMaterial;
    } else {
        newMaterial = materialMap[property.C_Str()];
    }
    return newMaterial;
}

void ModelAsset::createMeshes(aiNode *aiNode, glm::mat4 parentTransform) {
    parentTransform = parentTransform * GLMConverter::AssimpToGLM(aiNode->mTransformation);

    for (int i = 0; i < aiNode->mNumMeshes; ++i) {
            aiMesh *currentMesh;
            currentMesh = scene->mMeshes[aiNode->mMeshes[i]];
            for (int j = 0; j < currentMesh->mNumBones; ++j) {
                meshOffsetmap[currentMesh->mBones[j]->mName.C_Str()] = GLMConverter::AssimpToGLM(
                        currentMesh->mBones[j]->mOffsetMatrix);
                std::string boneName =  currentMesh->mBones[j]->mName.C_Str();
                boneName = boneName + "_parent";
                meshOffsetmap[boneName] = parentTransform;
            }

            Material *meshMaterial = loadMaterials(scene, currentMesh->mMaterialIndex);
            MeshAsset *mesh = new MeshAsset(assetManager, currentMesh, meshMaterial, rootNode, parentTransform, hasAnimation);
            //FIXME the exception thrown from new is not catched
            meshes.push_back(mesh);
            std::cout << "loaded mesh " << currentMesh->mName.C_Str() << " for node " << aiNode->mName.C_Str()
                      << std::endl;
    }

    for (int i = 0; i < aiNode->mNumChildren; ++i) {
        createMeshes(aiNode->mChildren[i], parentTransform);
    }
}

BoneNode *ModelAsset::loadNodeTree(aiNode *aiNode) {
    BoneNode *currentNode = new BoneNode();
    currentNode->name = aiNode->mName.C_Str();
    currentNode->boneID = boneIDCounter++;
    currentNode->transformation = GLMConverter::AssimpToGLM(aiNode->mTransformation);
    for (int i = 0; i < aiNode->mNumChildren; ++i) {
        currentNode->children.push_back(loadNodeTree(aiNode->mChildren[i]));
    }
    return currentNode;
}

bool ModelAsset::findNode(const BoneNode *nodeToMatch, const aiMesh *meshToCheckBone, int *index) {
    std::string name = nodeToMatch->name;
    for (int i = 0; i < meshToCheckBone->mNumBones; ++i) {
        if (name == meshToCheckBone->mBones[i]->mName.C_Str()) {
            (*index) = i;
            return true;
        }
    }
    return false;
}

void ModelAsset::getTransform(long time, std::vector<glm::mat4> &transformMatrix) const {

    aiAnimation *currentAnimation = animations[0];

    float ticksPersecond;
    if (currentAnimation->mTicksPerSecond != 0) {
        ticksPersecond = currentAnimation->mTicksPerSecond;
    } else {
        ticksPersecond = 25.0f;
    }

    float animationTime = fmod((time / 1000.0f) * ticksPersecond, currentAnimation->mDuration);

    glm::mat4 parentTransform = glm::mat4(1.0f);
    traverseAndSetTransform(rootNode, parentTransform, currentAnimation, animationTime, transformMatrix);

}

void
ModelAsset::traverseAndSetTransform(const BoneNode *boneNode, const glm::mat4 &parentTransform, aiAnimation *animation,
                                    float timeInTicks,
                                    std::vector<glm::mat4> &transforms) const {
    const aiNodeAnim *nodeAnimation = findNodeAnimation(animation, boneNode->name);
    glm::mat4 nodeTransform;

    if (nodeAnimation == NULL) {
        nodeTransform = boneNode->transformation;
    } else {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D scalingTransformVector, transformVector;
        aiQuaternion rotationTransformQuaternion;

        scalingTransformVector = getScalingVector(timeInTicks, nodeAnimation);
        rotationTransformQuaternion = getRotationQuat(timeInTicks, nodeAnimation);
        transformVector = getPositionVector(timeInTicks, nodeAnimation);

        glm::mat4 rotationMatrix = glm::mat4_cast(GLMConverter::AssimpToGLM(rotationTransformQuaternion));
        glm::mat4 translateMatrix = glm::translate(glm::mat4(1.0f), GLMConverter::AssimpToGLM(transformVector));
        glm::mat4 scaleTransform = glm::scale(glm::mat4(1.0f), GLMConverter::AssimpToGLM(scalingTransformVector));
        nodeTransform =  translateMatrix * rotationMatrix * scaleTransform;
    }

    nodeTransform = parentTransform * nodeTransform;

    if(meshOffsetmap.find(boneNode->name) != meshOffsetmap.end()) {
        transforms[boneNode->boneID] =
                globalInverseTransform * meshOffsetmap.at(boneNode->name + "_parent") * nodeTransform * meshOffsetmap.at(boneNode->name);
        //parent above means parent transform of the mesh node, not the parent of bone.
    }

    //Call children even if parent does not have animation attached.
    for (int i = 0; i < boneNode->children.size(); ++i) {
        traverseAndSetTransform(boneNode->children[i], nodeTransform, animation, timeInTicks, transforms);
    }
}

aiVector3D ModelAsset::getPositionVector(const float timeInTicks, const aiNodeAnim *nodeAnimation) const {
    aiVector3D transformVector;
    if (nodeAnimation->mNumPositionKeys == 1) {
            transformVector = nodeAnimation->mPositionKeys[0].mValue;
        } else {

            int positionIndex = 0;

            for (int i = 0; i < nodeAnimation->mNumPositionKeys; i++) {
                if (timeInTicks < (float) nodeAnimation->mPositionKeys[i + 1].mTime) {
                    positionIndex = i;
                    break;
                }
            }

            int NextPositionIndex = (positionIndex + 1);
            assert(NextPositionIndex < nodeAnimation->mNumPositionKeys);
            float DeltaTime = (float) (nodeAnimation->mPositionKeys[NextPositionIndex].mTime -
                                       nodeAnimation->mPositionKeys[positionIndex].mTime);
            float Factor = (timeInTicks - (float) nodeAnimation->mPositionKeys[positionIndex].mTime) / DeltaTime;
            assert(Factor >= 0.0f && Factor <= 1.0f);
            const aiVector3D &Start = nodeAnimation->mPositionKeys[positionIndex].mValue;
            const aiVector3D &End = nodeAnimation->mPositionKeys[NextPositionIndex].mValue;
            aiVector3D Delta = End - Start;
            transformVector = Start + Factor * Delta;
        }
    return transformVector;
}

aiVector3D ModelAsset::getScalingVector(const float timeInTicks, const aiNodeAnim *nodeAnimation) const {
    aiVector3D scalingTransformVector;
    if (nodeAnimation->mNumScalingKeys == 1) {
            scalingTransformVector = nodeAnimation->mScalingKeys[0].mValue;
        } else {
            int ScalingIndex = 0;

            assert(nodeAnimation->mNumScalingKeys > 0);

            for (int i = 0; i < nodeAnimation->mNumScalingKeys; i++) {
                if (timeInTicks < (float) nodeAnimation->mScalingKeys[i + 1].mTime) {
                    ScalingIndex = i;
                    break;
                }
            }


            int NextScalingIndex = (ScalingIndex + 1);
            assert(NextScalingIndex < nodeAnimation->mNumScalingKeys);
            float DeltaTime = (float) (nodeAnimation->mScalingKeys[NextScalingIndex].mTime -
                                       nodeAnimation->mScalingKeys[ScalingIndex].mTime);
            float Factor = (timeInTicks - (float) nodeAnimation->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
            assert(Factor >= 0.0f && Factor <= 1.0f);
            const aiVector3D &Start = nodeAnimation->mScalingKeys[ScalingIndex].mValue;
            const aiVector3D &End = nodeAnimation->mScalingKeys[NextScalingIndex].mValue;
            aiVector3D Delta = End - Start;
            scalingTransformVector = Start + Factor * Delta;
        }
    return scalingTransformVector;
}

aiQuaternion ModelAsset::getRotationQuat(const float timeInTicks, const aiNodeAnim *nodeAnimation) const {
    aiQuaternion rotationTransformQuaternion;
    if (nodeAnimation->mNumRotationKeys == 1) {
        rotationTransformQuaternion = nodeAnimation->mRotationKeys[0].mValue;
        } else {

            int rotationIndex = 0;

            assert(nodeAnimation->mNumRotationKeys > 0);

            for (int i = 0; i < nodeAnimation->mNumRotationKeys; i++) {
                if (timeInTicks < (float) nodeAnimation->mRotationKeys[i + 1].mTime) {
                    rotationIndex = i;
                    break;
                }
            }

            int NextRotationIndex = (rotationIndex + 1);
            assert(NextRotationIndex < nodeAnimation->mNumRotationKeys);
            float DeltaTime = (float) (nodeAnimation->mRotationKeys[NextRotationIndex].mTime -
                                       nodeAnimation->mRotationKeys[rotationIndex].mTime);
            float Factor = (timeInTicks - (float) nodeAnimation->mRotationKeys[rotationIndex].mTime) / DeltaTime;
            assert(Factor >= 0.0f && Factor <= 1.0f);
            const aiQuaternion &StartRotationQ = nodeAnimation->mRotationKeys[rotationIndex].mValue;
            const aiQuaternion &EndRotationQ = nodeAnimation->mRotationKeys[NextRotationIndex].mValue;
            aiQuaternion::Interpolate(rotationTransformQuaternion, StartRotationQ, EndRotationQ, Factor);
            rotationTransformQuaternion = rotationTransformQuaternion.Normalize();
        }
    return rotationTransformQuaternion;
}

const aiNodeAnim *ModelAsset::findNodeAnimation(aiAnimation *animation, std::string nodeName) const {
    for (int i = 0; i < animation->mNumChannels; i++) {
        const aiNodeAnim *nodeAnimation = animation->mChannels[i];

        if (nodeAnimation->mNodeName.C_Str() == nodeName) {
            return nodeAnimation;
        }
    }

    return NULL;
}

bool ModelAsset::isAnimated() const {
    return hasAnimation;
}
