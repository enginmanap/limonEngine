//
// Created by engin on 22.09.2019.
//

#include "QuadRender.h"
#include "Graphics/GraphicsInterface.h"
#include "Graphics/GraphicsProgram.h"

QuadRender::QuadRender(GraphicsInterface* graphicsWrapper) : graphicsWrapper(graphicsWrapper){
    std::vector<glm::vec3> vertices;

    vertices.push_back(glm::vec3( -1.0f,  1.0f, 0.0f));
    vertices.push_back(glm::vec3( -1.0f, -1.0f, 0.0f));
    vertices.push_back(glm::vec3(  1.0f,  1.0f, 0.0f));
    vertices.push_back(glm::vec3(  1.0f, -1.0f, 0.0f));

    std::vector<glm::mediump_uvec3> faces;

    faces.push_back(glm::mediump_uvec3(0,1,2));
    faces.push_back(glm::mediump_uvec3(2,1,3));

    std::vector<glm::vec2> textureCoordinates;

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));


    uint_fast32_t vbo;
    graphicsWrapper->bufferVertexData(vertices, faces, vao, vbo, 1, ebo);
    bufferObjects.push_back(vbo);

    graphicsWrapper->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 2);
    bufferObjects.push_back(vbo);

}

void QuadRender::render(std::shared_ptr<GraphicsProgram> renderProgram) {
    graphicsWrapper->render(renderProgram->getID(), vao, ebo, 3 * 2);//2 triangles
}
