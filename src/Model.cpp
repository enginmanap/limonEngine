//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"
#include "Utils/GLMConverter.h"


Model::Model(GLHelper *glHelper, const float mass, const std::string &modelFile) :
        PhysicalRenderable(glHelper), modelFile(modelFile) {

    Assimp::Importer import;
    //FIXME triangulate creates too many vertices, it is unnecessary, but optimize requires some work.
    const aiScene *scene = import.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_FlipUVs |
                                                      aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_GenNormals);

    if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }

    std::cout << "Load success::ASSIMP::" << modelFile << std::endl;

    bulletMesh = new btTriangleMesh();
    std::vector<btVector3> bulletMeshVertices;
    if (!scene->HasMeshes()) {
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    }

    for (int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh *currentMesh = scene->mMeshes[i];

        if (!currentMesh->HasPositions()) {
            continue; //Not going to process if mesh is empty
        }
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

        GLuint vbo;
        glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
        bufferObjects.push_back(vbo);
        //Fixme vbo seems unnecessary
        glHelper->bufferNormalData(normals, vao, vbo, 4);
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
            Material *newMaterial = new Material(glHelper, property.C_Str());
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
                    "Texture path: [" << property.C_Str() << "]" << std::endl;
                }
            }
            if ((currentMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)) {
                if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &property)) {
                    newMaterial->setDiffuseTexture(property.C_Str());
                    std::cout << "loaded diffuse texture " << property.C_Str() << std::endl;
                } else {
                    std::cerr << "The model contained diffuse texture information, but texture loading failed. \n" <<
                    "Texture path: [" << property.C_Str() << "]" << std::endl;
                }
            }

            if ((currentMaterial->GetTextureCount(aiTextureType_SPECULAR) > 0)) {
                if (AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_SPECULAR, 0, &property)) {
                    newMaterial->setSpecularTexture(property.C_Str());
                    std::cout << "loaded specular texture " << property.C_Str() << std::endl;
                } else {
                    std::cerr << "The model contained specular texture information, but texture loading failed. \n" <<
                    "Texture path: [" << property.C_Str() << "]" << std::endl;
                }
            }

            materialMap[newMaterial->getName()] = newMaterial;

            glHelper->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3, ebo);
            bufferObjects.push_back(vbo);
        }
    }

    aiVector3D min, max;
    AssimpUtils::get_bounding_box(scene, &min, &max);


    std::cout << "bounding box of the model is " << "(" << max.x << "," <<
    max.y << "," <<
    max.z << ")" << ", " <<
    "(" << min.x << "," <<
    min.y << "," <<
    min.z << ")" << std::endl;

    //set up the rigid body

    btConvexTriangleMeshShape *convexShape = new btConvexTriangleMeshShape(bulletMesh);

    centerOffset = glm::vec3((max.x + min.x) / 2, (max.y + min.y) / 2, (max.z + min.z) / 2);
    btDefaultMotionState *boxMotionState = new btDefaultMotionState(
            btTransform(btQuaternion(0, 0, 0, 1), GLMConverter::GLMToBlt(centerOffset)));
    btVector3 fallInertia(0, 0, 0);
    convexShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo *rigidBodyConstructionInfo;
    if (bulletMeshVertices.size() > 24) { // hull shape has 24 vertices, if we already has less, don't generate hull.
        //hull approximation
        btShapeHull *hull = new btShapeHull(convexShape);
        btScalar margin = convexShape->getMargin();
        hull->buildHull(margin);
        simplifiedConvexShape = new btConvexHullShape((const btScalar *) hull->getVertexPointer(),
                                                      hull->numVertices());
        rigidBodyConstructionInfo = new btRigidBody::btRigidBodyConstructionInfo(mass, boxMotionState,
                                                                                 simplifiedConvexShape, fallInertia);

    } else {
        //direct use of the object
        rigidBodyConstructionInfo = new btRigidBody::btRigidBodyConstructionInfo(mass, boxMotionState, convexShape,
                                                                                 fallInertia);
        simplifiedConvexShape = NULL;
    }

    rigidBody = new btRigidBody(*rigidBodyConstructionInfo);
    //TODO check if this values are too low.
    rigidBody->setSleepingThresholds(0.1, 0.1);

    //glHelper->bufferVertexColor(colors,vao,vbo,3);
    worldTransform = glm::mat4(1.0f);

    //set up the program to render object
    renderProgram = new GLSLProgram(glHelper, "./Data/Shaders/Model/vertex.shader",
                                    "./Data/Shaders/Model/fragment.shader");


}

void Model::activateMaterial(const Material *material) {
    GLuint location;
    if (!renderProgram->setUniform("material.ambient", material->getAmbientColor())) {
        std::cerr << "Uniform \"material.ambient\" could not be set." << std::endl;
    }

    if (!renderProgram->setUniform("material.shininess", material->getSpecularExponent())) {
        std::cerr << "Uniform \"material.shininess\" could not be set" << std::endl;
    }

    glHelper->attachTexture(material->getDiffuseTexture()->getID(), diffuseMapAttachPoint);
    //TODO we should support multi texture on one pass
}

bool Model::setupRenderVariables() {
    if (renderProgram->setUniform("worldTransformMatrix", getWorldTransform())) {
        if (renderProgram->setUniform("cameraPosition", glHelper->getCameraPosition())) {
            if (this->materialMap.begin() != this->materialMap.end()) {
                this->activateMaterial(materialMap.begin()->second);
            } else {
                std::cerr << "No material setup, passing rendering. " << std::endl;
            }
            if (!renderProgram->setUniform("diffuseSampler",
                                           diffuseMapAttachPoint)) { //even if diffuse map cannot attach, we still render
                std::cerr << "Uniform \"diffuseSampler\" could not be set, passing rendering." << std::endl;
            }
            return true;
        } else {
            std::cerr << "Uniform \"cameraPosition\" could not be set, passing rendering." << std::endl;
        }
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
    return false;

}

void Model::render() {
    if (setupRenderVariables()) {
        glHelper->render(renderProgram->getID(), vao, ebo, faces.size() * 3);
    }
}

void Model::renderWithProgram(GLSLProgram &program) {
    if (program.setUniform("worldTransformMatrix", getWorldTransform())) {
        glHelper->render(program.getID(), vao, ebo, faces.size() * 3);
    } else {
        std::cerr << "Uniform \"worldTransformMatrix\" could not be set, passing rendering." << std::endl;
    }
}