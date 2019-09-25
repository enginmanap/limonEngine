//
// Created by engin on 12.12.2018.
//

#include "QuadRenderBase.h"
#include "Graphics/GraphicsInterface.h"
#include "Graphics/GLSLProgram.h"

QuadRenderBase::QuadRenderBase(GraphicsInterface* graphicsWrapper) : graphicsWrapper(graphicsWrapper){
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

void QuadRenderBase::render() {

    //we should attach textures
    for (auto textureIterator = textureAttachments.begin();
         textureIterator != textureAttachments.end(); ++textureIterator) {
        program->setUniform(textureIterator->first, textureIterator->second);
    }

    graphicsWrapper->render(program->getID(), vao, ebo, 3 * 2);//2 triangles
}

void QuadRenderBase::setSourceTexture(std::string samplerName, int32_t textureID) {
    auto uniformMap = program->getUniformMap();
    auto uniformToSetIt = uniformMap.find(samplerName);
    if (uniformToSetIt == uniformMap.end()) {
        std::cerr << "Source texture [" << samplerName << "] for QuadRenderer is not defined in set program[" << program->getProgramName() << "]! " << std::endl;
        return;
    }

    if (uniformToSetIt->second == nullptr) {
        std::cerr << "Uniform found, but object creation was failed for it! " << std::endl;
        return;
    }
    if (uniformToSetIt->second->type == GraphicsInterface::VariableTypes::CUBEMAP ||
        uniformToSetIt->second->type == GraphicsInterface::VariableTypes::CUBEMAP_ARRAY ||
        uniformToSetIt->second->type == GraphicsInterface::VariableTypes::TEXTURE_2D ||
        uniformToSetIt->second->type == GraphicsInterface::VariableTypes::TEXTURE_2D_ARRAY) {
        textureAttachments[samplerName] = textureID;
    } else {
        std::cerr << "Source texture [" << samplerName << "] for QuadRenderer is not a texture type in set program[" << program->getProgramName() << "]! " << std::endl;
    }
}
