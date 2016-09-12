//
// Created by engin on 31.08.2016.
//

#include "ModelAsset.h"

ModelAsset::ModelAsset(AssetManager *assetManager, const std::vector<std::string> &fileList) : Asset(assetManager,
                                                                                                     fileList) {
    if (fileList.size() < 1) {
        std::cerr << "Model load failed because file name vector is empty." << std::endl;
        exit(-1);
    }
    name = fileList[0];
    if (fileList.size() > 1) {
        std::cerr << "multiple files are sent to Model constructor, extra elements ignored." << std::endl;
    }


    Assimp::Importer import;
    //FIXME triangulate creates too many vertices, it is unnecessary, but optimize requires some work.
    const aiScene *scene = import.ReadFile(name, aiProcess_Triangulate | aiProcess_FlipUVs |
                                                 aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GenNormals);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }

    std::cout << "Load success::ASSIMP::" << name << std::endl;

    bulletMesh = new btTriangleMesh();

    if (!scene->HasMeshes()) {
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    }

    triangleCount = 0;
    vertexCount = 0;
    for (int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *currentMesh = scene->mMeshes[i];
        triangleCount += currentMesh->mNumFaces;
        if (!currentMesh->HasPositions()) {
            continue; //Not going to process if mesh is empty
        }

        vertexCount += currentMesh->mNumVertices;
        if (currentMesh->HasTextureCoords(0)) {
            for (int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
                bulletMeshVertices.push_back(GLMConverter::AssimpToBullet(currentMesh->mVertices[j]));
                textureCoordinates.push_back(
                        glm::vec2(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y));

            }
        } else {
            for (int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                normals.push_back(GLMConverter::AssimpToGLM(currentMesh->mNormals[j]));
                bulletMeshVertices.push_back(GLMConverter::AssimpToBullet(currentMesh->mVertices[j]));
            }
        }

        for (int j = 0; j < currentMesh->mNumFaces; ++j) {
            faces.push_back(glm::vec3(currentMesh->mFaces[j].mIndices[0],
                                      currentMesh->mFaces[j].mIndices[1],
                                      currentMesh->mFaces[j].mIndices[2]));
            bulletMesh->addTriangle(bulletMeshVertices[currentMesh->mFaces[j].mIndices[0]],
                                    bulletMeshVertices[currentMesh->mFaces[j].mIndices[1]],
                                    bulletMeshVertices[currentMesh->mFaces[j].mIndices[2]]);
        }
        if (vertexCount <= 24) {
            for (int j = 0; j < currentMesh->mNumFaces; ++j) {
                bulletMeshFaces.push_back(glm::mediump_uvec3(currentMesh->mFaces[j].mIndices[0],
                                                             currentMesh->mFaces[j].mIndices[1],
                                                             currentMesh->mFaces[j].mIndices[2]));
            }
        }

        convexShape = new btConvexTriangleMeshShape(bulletMesh);
        if (vertexCount > 24) { // hull shape has 24 vertices, if we already has less, don't generate hull.
            //hull approximation

            btShapeHull *hull = new btShapeHull(convexShape);
            btScalar margin = convexShape->getMargin();
            hull->buildHull(margin);
            delete convexShape;
            bulletMeshVertices.clear();

            simplifiedConvexShape = new btConvexHullShape((const btScalar *) hull->getVertexPointer(),
                                                          hull->numVertices());
            for (int j = 0; j < hull->numVertices(); ++j) {
                bulletMeshVertices.push_back(hull->getVertexPointer()[j]);
            }
            for (int k = 0; k < hull->numIndices(); k = k + 3) {
                bulletMeshFaces.push_back(glm::mediump_uvec3(hull->getIndexPointer()[k],
                                                             hull->getIndexPointer()[k + 1],
                                                             hull->getIndexPointer()[k + 2]));
            }
            bulletTriangleCount = hull->numTriangles();
            vertexCount = hull->numVertices();
        } else {
            //direct use of the object
            simplifiedConvexShape = convexShape;
            bulletTriangleCount = triangleCount;
        }
        convexShape = NULL; //since simplified Convex shape has taken over, we don't need the pointer anymore

        GLuint vbo;
        assetManager->getGlHelper()->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
        bufferObjects.push_back(vbo);


        assetManager->getGlHelper()->bufferNormalData(normals, vao, vbo, 4);
        bufferObjects.push_back(vbo);

        // create material uniform buffer
        aiMaterial *currentMaterial = scene->mMaterials[currentMesh->mMaterialIndex];
        aiString property;    //contains filename of texture
        if (AI_SUCCESS != currentMaterial->Get(AI_MATKEY_NAME, property)) {
            std::cerr << "Material without a name is not handled." << std::endl;
            exit(-1);
        }

        if (materialMap.find(property.C_Str()) == materialMap.end()) {//search for the name
            //if the material is not loaded before
            Material *newMaterial = new Material(assetManager, property.C_Str());
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

            assetManager->getGlHelper()->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3);
            bufferObjects.push_back(vbo);
        }
    }


    aiVector3D min, max;
    AssimpUtils::get_bounding_box(scene, &min, &max);
    boundingBoxMax = GLMConverter::AssimpToGLM(max);
    boundingBoxMin = GLMConverter::AssimpToGLM(min);

    centerOffset = glm::vec3((max.x + min.x) / 2, (max.y + min.y) / 2, (max.z + min.z) / 2);
}
