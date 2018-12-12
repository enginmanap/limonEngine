//
// Created by engin on 12.12.2018.
//

#include "CombiningObject.h"
#include "../GLHelper.h"
#include "../GLSLProgram.h"

CombiningObject::CombiningObject(GLHelper *glHelper) : glHelper(glHelper){
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
    glHelper->bufferVertexData(vertices, faces, vao, vbo, 1, ebo);
    bufferObjects.push_back(vbo);

    glHelper->bufferVertexTextureCoordinates(textureCoordinates, vao, vbo, 2);
    bufferObjects.push_back(vbo);

    program = new GLSLProgram(glHelper, "./Engine/Shaders/CombineAll/vertex.glsl",
            "./Engine/Shaders/CombineAll/fragment.glsl", false);

}

void CombiningObject::render() {

    //we should attach textures
    for (auto textureIterator = textureAttachments.begin();
         textureIterator != textureAttachments.end(); ++textureIterator) {
        program->setUniform(textureIterator->first, textureIterator->second);
    }

    glHelper->render(program->getID(), vao, ebo, 3 * 2);//2 triangles
}
