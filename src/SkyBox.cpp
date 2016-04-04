//
// Created by Engin Manap on 1.03.2016.
//

#include "SkyBox.h"

SkyBox::SkyBox(GLHelper* glHelper, std::string right, std::string left, std::string top, std::string down, std::string back,
               std::string front):
    Renderable(glHelper)
    {
    cubeMap = new CubeMap(glHelper, "./Data/Textures/Skyboxes/ThickCloudsWater",
                          "right.jpg", "left.jpg",
                          "top.jpg", "bottom.jpg",
                          "back.jpg", "front.jpg");


    vertices.push_back(glm::vec3(-1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f, -1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f, -1.0f));
    vertices.push_back(glm::vec3(-1.0f,  1.0f, -1.0f));

    vertices.push_back(glm::vec3(-1.0f, -1.0f, 1.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f, 1.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f, 1.0f));
    vertices.push_back(glm::vec3(-1.0f,  1.0f, 1.0f));

    //front
    faces.push_back(glm::mediump_uvec3(0, 1, 2));
    faces.push_back(glm::mediump_uvec3(0, 2, 3));
    //Back
    faces.push_back(glm::mediump_uvec3(4, 7, 6));
    faces.push_back(glm::mediump_uvec3(4, 6, 5));
    //right
    faces.push_back(glm::mediump_uvec3(4, 0, 3));
    faces.push_back(glm::mediump_uvec3(4, 3, 7));
    //left
    faces.push_back(glm::mediump_uvec3(5, 6, 2));
    faces.push_back(glm::mediump_uvec3(5, 2, 1));
    //down
    faces.push_back(glm::mediump_uvec3(4, 1, 0));
    faces.push_back(glm::mediump_uvec3(4, 5, 1));
    //up
    faces.push_back(glm::mediump_uvec3(3, 6, 7));
    faces.push_back(glm::mediump_uvec3(3, 2, 6));


    GLuint vbo;
    glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    uniforms.push_back("cameraTransformMatrix");
    renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/SkyCube/vertex.shader","./Data/Shaders/SkyCube/fragment.shader",uniforms);
}

void SkyBox::render(){
    glHelper->attachCubeMap(cubeMap->getID());
    glm::mat4 viewMatrix = glHelper->getProjectionMatrix() * glm::mat4(glm::mat3(glHelper->getCameraMatrix()));
    GLuint location;
    if(renderProgram->getUniformLocation("cameraTransformMatrix", location)) {
        glHelper->setUniform(renderProgram->getID(), location, viewMatrix);
        glHelper->render(renderProgram->getID(), vao, ebo, 36);
    }
}
