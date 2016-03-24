//
// Created by engin on 24.03.2016.
//

#include "GUIRenderable.h"

GUIRenderable::GUIRenderable(GLHelper* glHelper) : Renderable(glHelper) {
    vertices.push_back(glm::vec3(-1.0f, -1.0f,  0.0f));
    vertices.push_back(glm::vec3( 1.0f, -1.0f,  0.0f));
    vertices.push_back(glm::vec3( 1.0f,  1.0f,  0.0f));
    vertices.push_back(glm::vec3(-1.0f,  1.0f,  0.0f));

    faces.push_back(glm::mediump_uvec3( 0, 1, 2));//front
    faces.push_back(glm::mediump_uvec3( 0, 2, 3));

    GLuint vbo;
    glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    glHelper->bufferVertexTextureCoordinates(textureCoordinates,vao,vbo,3,ebo);

    uniforms.push_back("worldTransformMatrix");
    renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/GUI/vertex.shader","./Data/Shaders/GUI/fragment.shader",uniforms);
}

void GUIRenderable::render(){
    GLuint location;
    if(renderProgram->getUniformLocation("worldTransformMatrix", location)) {
        glHelper->setUniform(renderProgram->getID(), location, getWorldTransform());
        glHelper->attachTexture(textureID);
        glHelper->render(renderProgram->getID(), vao, ebo, faces.size()*3);
    }
}

