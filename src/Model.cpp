//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"



Model::Model(GLHelper* glHelper, float mass):
    Renderable(glHelper){

    /*
     * These are the coordinates and faces for star.
     *
    vertices.push_back(glm::vec3(+0.5f, +0.5f,  0.5f));
    vertices.push_back(glm::vec3(-0.5f, -0.5f,  0.5f));
    vertices.push_back(glm::vec3(-0.5f, +0.5f, -0.5f));
    vertices.push_back(glm::vec3(+0.5f, -0.5f, -0.5f));
    vertices.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
    vertices.push_back(glm::vec3(+0.5f, +0.5f, -0.5f));
    vertices.push_back(glm::vec3(+0.5f, -0.5f,  0.5f));
    vertices.push_back(glm::vec3(-0.5f, +0.5f,  0.5f));



    colors.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    colors.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    colors.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    colors.push_back(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
    colors.push_back(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    colors.push_back(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    colors.push_back(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    colors.push_back(glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));



    faces.push_back(glm::mediump_uvec3(1, 0, 2));
    faces.push_back(glm::mediump_uvec3(0, 1, 3));
    faces.push_back(glm::mediump_uvec3(3, 2, 0));
    faces.push_back(glm::mediump_uvec3(2, 3, 1));

    faces.push_back(glm::mediump_uvec3(4, 5, 6));
    faces.push_back(glm::mediump_uvec3(5, 4, 7));
    faces.push_back(glm::mediump_uvec3(6, 7, 4));
    faces.push_back(glm::mediump_uvec3(7, 6, 5));
*/

    //back
    vertices.push_back(glm::vec3(-1.0f,  1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3(-1.0f, -1.0f, -1.0f));
    //front
    vertices.push_back(glm::vec3(-1.0f, -1.0f,  1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f,  1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f,  1.0f));
    vertices.push_back(glm::vec3(-1.0f,  1.0f,  1.0f));
    //up
    vertices.push_back(glm::vec3(-1.0f,  1.0f, -1.0f));
    vertices.push_back(glm::vec3(-1.0f,  1.0f,  1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f,  1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f, -1.0f));
    //down
    vertices.push_back(glm::vec3(-1.0f, -1.0f,  1.0f));
    vertices.push_back(glm::vec3(-1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f,  1.0f));
    //left
    vertices.push_back(glm::vec3(-1.0f,  1.0f, -1.0f));
    vertices.push_back(glm::vec3(-1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3(-1.0f, -1.0f,  1.0f));
    vertices.push_back(glm::vec3(-1.0f,  1.0f,  1.0f));
    //right
    vertices.push_back(glm::vec3( 1.0f,  1.0f,  1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f,  1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f, -1.0f));


    faces.push_back(glm::mediump_uvec3( 0, 1, 2));//front
    faces.push_back(glm::mediump_uvec3( 0, 2, 3));
    faces.push_back(glm::mediump_uvec3( 4, 5, 6));//Back
    faces.push_back(glm::mediump_uvec3( 4, 6, 7));
    faces.push_back(glm::mediump_uvec3( 8, 9,10));//up
    faces.push_back(glm::mediump_uvec3( 8,10,11));
    faces.push_back(glm::mediump_uvec3(12,13,14));//down
    faces.push_back(glm::mediump_uvec3(12,14,15));
    faces.push_back(glm::mediump_uvec3(16,17,18));//left
    faces.push_back(glm::mediump_uvec3(16,18,19));
    faces.push_back(glm::mediump_uvec3(20,21,22));//right
    faces.push_back(glm::mediump_uvec3(20,22,23));

    GLuint vbo;
    glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));

    texture = new Texture(glHelper, ".\\Data\\Textures\\Box\\crate2_diffuse.png");

    glHelper->bufferVertexTextureCoordinates(textureCoordinates,vao,vbo,3,ebo);
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