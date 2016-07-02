//
// Created by engin on 24.03.2016.
//

#include "GUIRenderable.h"

GUIRenderable::GUIRenderable(GLHelper *glHelper) : Renderable(glHelper) {
    vertices.push_back(glm::vec3(-1.0f, -1.0f, 0.0f));
    vertices.push_back(glm::vec3(1.0f, -1.0f, 0.0f));
    vertices.push_back(glm::vec3(1.0f, 1.0f, 0.0f));
    vertices.push_back(glm::vec3(-1.0f, 1.0f, 0.0f));

    faces.push_back(glm::mediump_uvec3(0, 1, 2));//front
    faces.push_back(glm::mediump_uvec3(0, 2, 3));

    GLuint vbo;
    glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);
    bufferObjects.push_back(vbo);

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    glHelper->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 3, ebo);

    renderProgram = new GLSLProgram(glHelper, "./Data/Shaders/GUI/vertex.shader", "./Data/Shaders/GUI/fragment.shader");
}

bool GUIRenderable::setupRenderVariables() {
    if (renderProgram->setUniform("worldTransformMatrix", getWorldTransform())) {
        glHelper->attachTexture(textureID);
        return true;
    } else {
        std::cerr << "Uniform \"cameraTransformMatrix\" could not be set, passing rendering of GUIRenderable" <<
        std::endl;
    }
    return false;
}

void GUIRenderable::render() {
    if (setupRenderVariables()) {
        glHelper->render(renderProgram->getID(), vao, ebo, faces.size() * 3);
    }
}

void GUIRenderable::renderDebug() {
    float up = translate.y + scale.y;
    float down = translate.y - scale.y;

    float right = translate.x + scale.x;
    float left = translate.x - scale.x;

    //now build 4 lines;
    glHelper->drawLine(glm::vec3(left, up, 0.0f), glm::vec3(left, down, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
                       glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(right, up, 0.0f), glm::vec3(right, down, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
                       glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(left, up, 0.0f), glm::vec3(right, up, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
                       glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(left, down, 0.0f), glm::vec3(right, down, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f),
                       glm::vec3(1.0f, 1.0f, 1.0f), false);
}