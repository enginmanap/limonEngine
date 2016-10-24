//
// Created by engin on 31.08.2016.
//

#include "ModelAsset.h"


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
    Assimp::Importer import;
    //FIXME triangulate creates too many vertices, it is unnecessary, but optimize requires some work.
    const aiScene *scene = import.ReadFile(name, aiProcess_Triangulate | aiProcess_FlipUVs |
                                                 aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GenNormals);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }

    std::cout << "ASSIMP::success::" << name << std::endl;

    if (!scene->HasMeshes()) {
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    } else {
        std::cout << "Model has " << scene->mNumMeshes << " mesh(es)." << std::endl;
    }
    MeshAsset *mesh;
    Material *meshMaterial;
    aiMesh *currentMesh;

    if (scene->mNumAnimations > 0) {
        this->rootNode = loadNodeTree(scene->mRootNode);
    }

    for (int i = 0; i < scene->mNumMeshes; ++i) {
        currentMesh = scene->mMeshes[i];
        //we should build bone tree for mesh
        BoneNode *meshBoneRoot = NULL;
        if (currentMesh->mNumBones != 0) {
            meshBoneRoot = createMeshTree(rootNode, currentMesh);
        }
        meshMaterial = loadMaterials(scene, currentMesh->mMaterialIndex);
        mesh = new MeshAsset(assetManager, currentMesh, meshMaterial, meshBoneRoot);
        meshes.push_back(mesh);
    }


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
        exit(-1);
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

BoneNode *ModelAsset::loadNodeTree(aiNode *aiNode) {
    BoneNode *currentNode = new BoneNode();
    currentNode->name = aiNode->mName.C_Str();
    currentNode->boneID = ++boneIDCounter;
    currentNode->offset = GLMConverter::AssimpToGLM(aiNode->mTransformation);
    for (int i = 0; i < aiNode->mNumChildren; ++i) {
        currentNode->children.push_back(loadNodeTree(aiNode->mChildren[i]));
    }
    return currentNode;
}

BoneNode *ModelAsset::createMeshTree(const BoneNode *sceneNode, const aiMesh *mesh) {
    int boneIndex;
    if (findNode(sceneNode, mesh, &boneIndex)) {
        BoneNode *newBone = new BoneNode();
        newBone->boneID = ++boneIDCounterPerMesh;
        newBone->name = sceneNode->name;
        newBone->offset = sceneNode->offset;
        newBone->offset = GLMConverter::AssimpToGLM(mesh->mBones[boneIndex]->mOffsetMatrix);
        for (int i = 0; i < sceneNode->children.size(); ++i) {
            newBone->children.push_back(createMeshTree(sceneNode->children[i], mesh));
        }
        return newBone;
    } else {
        //if the current node is not part of bone tree, get to the children
        for (int i = 0; i < sceneNode->children.size(); ++i) {
            // FIXME this assumes only one child of the node can be found
            // We assume not found case can only be if we did not get to skeleton tree yet, so it should work
            BoneNode *temp = NULL;
            temp = createMeshTree(sceneNode->children[i], mesh);
            if (temp != NULL) {
                return temp;
            }
        }
    }
    return NULL;
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
