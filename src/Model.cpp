//
// Created by Engin Manap on 13.02.2016.
//

#include "Model.h"


Model::Model(GLHelper* glHelper):
    Renderable(glHelper){
    vertices.push_back(glm::vec3(+0.5f, +0.5f,  0.5f));
    vertices.push_back(glm::vec3(-0.5f, -0.5f,  0.5f));
    vertices.push_back(glm::vec3(-0.5f, +0.5f, -0.5f));
    vertices.push_back(glm::vec3(+0.5f, -0.5f, -0.5f));
    vertices.push_back(glm::vec3(-0.5f, -0.5f, -0.5f));
    vertices.push_back(glm::vec3(+0.5f, +0.5f, -0.5f));
    vertices.push_back(glm::vec3(+0.5f, -0.5f,  0.5f));
    vertices.push_back(glm::vec3(-0.5f, +0.5f,  0.5f));


    static const GLfloat vertex_colors[] = {
            0.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f,

            0.0f, 1.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f, 1.0f,
            1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 0.0f, 1.0f,
    };


    faces.push_back(glm::mediump_uvec3(1, 0, 2));
    faces.push_back(glm::mediump_uvec3(0, 1, 3));
    faces.push_back(glm::mediump_uvec3(3, 2, 0));
    faces.push_back(glm::mediump_uvec3(2, 3, 1));

    faces.push_back(glm::mediump_uvec3(4, 5, 6));
    faces.push_back(glm::mediump_uvec3(5, 4, 7));
    faces.push_back(glm::mediump_uvec3(6, 7, 4));
    faces.push_back(glm::mediump_uvec3(7, 6, 5));


    glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);

    glHelper->bufferVertexColor(vertex_colors, sizeof(vertex_colors),vao,vbo,3);
    worldTransform = glm::mat4(1.0f);

    uniforms.push_back("cameraTransformMatrix");
    uniforms.push_back("worldTransformMatrix");

    renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/Star/vertex.shader","./Data/Shaders/Star/fragment.shader",uniforms);
}

void Model::render() {

    glm::mat4 viewMatrix = glHelper->getProjectionMatrix() * glHelper->getCameraMatrix();
    GLuint location;
    if(renderProgram->getUniformLocation("cameraTransformMatrix", location)) {
        glHelper->setUniform(renderProgram->getID(), location, viewMatrix);
        if(renderProgram->getUniformLocation("worldTransformMatrix", location)) {
            glHelper->setUniform(renderProgram->getID(), location, worldTransform);
            glHelper->render(renderProgram->getID(), vao, ebo, 24);
        }
    }

    //glHelper->render(0, vao, ebo, worldTransform, 24);
}