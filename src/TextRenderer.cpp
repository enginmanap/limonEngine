//
// Created by engin on 14.03.2016.
//

#include "TextRenderer.h"

TextRenderer::TextRenderer(GLHelper* glHelper, const std::string fontFile, const std::string text, const int size, const glm::lowp_uvec3 color): Renderable(glHelper){
        //TODO these init and quit should be done by font manager
        if(TTF_Init() == -1) {
            std::cerr << "SDL ttf could not init, error: \n"<< TTF_GetError() << "\n Exiting.." << std::endl;
            exit(1);
        }
        TTF_Font *font = TTF_OpenFont(fontFile.c_str(), size);
        SDL_Color sdlColor = {(Uint8)color.r,(Uint8)color.g,(Uint8)color.b};
        SDL_Surface *Message = TTF_RenderText_Blended(font, text.c_str(), sdlColor);
        textureID = glHelper->loadTexture(Message->h,Message->w,GL_BGRA,Message->pixels);


        vertices.push_back(glm::vec3(-1.0f, -1.0f,  0.0f));
        vertices.push_back(glm::vec3( 1.0f, -1.0f,  0.0f));
        vertices.push_back(glm::vec3( 1.0f,  1.0f,  0.0f));
        vertices.push_back(glm::vec3(-1.0f,  1.0f,  0.0f));

        faces.push_back(glm::mediump_uvec3( 0, 1, 2));//front
        faces.push_back(glm::mediump_uvec3( 0, 2, 3));

        glHelper->bufferVertexData(vertices, faces, vao, vbo, 2, ebo);


        textureCoordinates.push_back(glm::vec2(0.0f, 1.0f));
        textureCoordinates.push_back(glm::vec2(1.0f, 1.0f));
        textureCoordinates.push_back(glm::vec2(1.0f, 0.0f));
        textureCoordinates.push_back(glm::vec2(0.0f, 0.0f));
        glHelper->bufferVertexTextureCoordinates(textureCoordinates,vao,vbo,3,ebo);

        uniforms.push_back("worldTransformMatrix");
        renderProgram = new GLSLProgram(glHelper,"./Data/Shaders/GUI/vertex.shader","./Data/Shaders/GUI/fragment.shader",uniforms);
}

void TextRenderer::render(){
    GLuint location;
    if(renderProgram->getUniformLocation("worldTransformMatrix", location)) {
        glHelper->setUniform(renderProgram->getID(), location, getWorldTransform());
        glHelper->attachTexture(textureID);
        glHelper->render(renderProgram->getID(), vao, ebo, faces.size()*3);
    }
}
