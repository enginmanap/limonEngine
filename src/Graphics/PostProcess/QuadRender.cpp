//
// Created by engin on 22.09.2019.
//

#include "QuadRender.h"
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "limonAPI/Graphics/GraphicsProgram.h"

QuadRender::QuadRender(GraphicsInterface* graphicsWrapper) : graphicsWrapper(graphicsWrapper){
    std::vector<glm::vec3> vertices;

    vertices.push_back(glm::vec3( -1.0f,  1.0f, 0.0f));
    vertices.push_back(glm::vec3( -1.0f, -1.0f, 0.0f));
    vertices.push_back(glm::vec3(  1.0f,  1.0f, 0.0f));
    vertices.push_back(glm::vec3(  1.0f, -1.0f, 0.0f));

    std::vector<glm::uvec3> faces;

    faces.push_back(glm::uvec3(0,1,2));
    faces.push_back(glm::uvec3(2,1,3));

    std::vector<glm::vec2> textureCoordinates;

    textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));
    textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));


    uint32_t vbo;
    graphicsWrapper->bufferVertexData(vertices, faces, vao, vbo, 1, ebo);
    bufferObjects.push_back(vbo);

    graphicsWrapper->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 2);
    bufferObjects.push_back(vbo);

}

void QuadRender::render(std::shared_ptr<GraphicsProgram> renderProgram, const std::string &cameraName [[gnu::unused]], const std::vector<HashUtil::HashedString> &tags [[gnu::unused]]) {
    graphicsWrapper->render(renderProgram->getID(), vao, ebo, 3 * 2);//2 triangles
}
