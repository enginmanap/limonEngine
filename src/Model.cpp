//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"
#include "Utils/GLMConverter.h"


Model::Model(GLHelper* glHelper, float mass):
    PhysicalRenderable(glHelper){

    Assimp::Importer import;
    //FIXME triangulate creates too many vertices, it is unnecessary, but optimize requires some work.
    const aiScene* scene = import.ReadFile("./Data/Models/Box/Box.obj", aiProcess_Triangulate);

    if(!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }

    std::cout << "Load success::ASSIMP" << std::endl;

    if(!scene->HasMeshes()){
        std::cout << "Model does not contain a mesh. This is not handled." << std::endl;
        exit(-1);
    }

    for (int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* currentMesh = scene->mMeshes[i];

        if(!currentMesh->HasPositions()){
            continue; //Not going to process if mesh is empty
        }
        if(currentMesh->HasTextureCoords(0))
            for (int j= 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
                textureCoordinates.push_back(glm::vec2(currentMesh->mTextureCoords[0][j].x, currentMesh->mTextureCoords[0][j].y));
            }
        else {
            for (int j = 0; j < currentMesh->mNumVertices; ++j) {
                vertices.push_back(GLMConverter::AssimpToGLM(currentMesh->mVertices[j]));
            }
        }
    
        for (int j= 0; j < currentMesh->mNumFaces; ++j) {
            faces.push_back(glm::vec3(currentMesh->mFaces[j].mIndices[0],
                                      currentMesh->mFaces[j].mIndices[1],
                                      currentMesh->mFaces[j].mIndices[2]));
        }



        // create material uniform buffer
        aiMaterial *currentMaterial = scene->mMaterials[currentMesh->mMaterialIndex];

        GLuint vbo;
        glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
        bufferObjects.push_back(vbo);

        aiString texturePath;	//contains filename of texture
        if(AI_SUCCESS == currentMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath)){
            //bind texture
            texture = new Texture(glHelper, texturePath.C_Str());

            glHelper->bufferVertexTextureCoordinates(textureCoordinates,vao,vbo,3,ebo);
        }
        else {
            std::cerr << "The model contained texture information, but texture loading failed. \n" <<
                         "Texture path: " << texturePath.C_Str() << std::endl;
        }

    }


    //glHelper->bufferVertexColor(colors,vao,vbo,3);
    worldTransform = glm::mat4(1.0f);

    //set up the program to render object
    uniforms.push_back("cameraTransformMatrix");
    uniforms.push_back("worldTransformMatrix");
    renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/Box/vertex.shader","./Data/Shaders/Box/fragment.shader",uniforms);

    //set up the rigit body

    btCollisionShape* boxShape = new btBoxShape(btVector3(1, 1, 1));
    btDefaultMotionState *boxMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 25, 0)));
    btVector3 fallInertia(0, 0, 0);
    boxShape->calculateLocalInertia(mass, fallInertia);
    btRigidBody::btRigidBodyConstructionInfo
            boxRigidBodyCI(mass, boxMotionState, boxShape, fallInertia);
    rigidBody = new btRigidBody(boxRigidBodyCI);

}

void Model::render() {

    glm::mat4 viewMatrix = glHelper->getProjectionMatrix() * glHelper->getCameraMatrix();
    GLuint location;
    if(renderProgram->getUniformLocation("cameraTransformMatrix", location)) {
        glHelper->setUniform(renderProgram->getID(), location, viewMatrix);
        if(renderProgram->getUniformLocation("worldTransformMatrix", location)) {
            glHelper->setUniform(renderProgram->getID(), location, getWorldTransform());
            glHelper->attachTexture(texture->getID());
            glHelper->render(renderProgram->getID(), vao, ebo, faces.size()*3);
        }
    }

    //glHelper->render(0, vao, ebo, worldTransform, 24);
}