//
// Created by Engin Manap on 14.03.2016.
//

#include "TextRenderer.h"

TextRenderer::TextRenderer(GLHelper* glHelper, const std::string fontFile, const std::string text, const int size, const glm::lowp_uvec3 color): GUIRenderable(glHelper){
    //TODO these init and quit should be done by font manager
    if(TTF_Init() == -1) {
        std::cerr << "SDL ttf could not init, error: \n"<< TTF_GetError() << "\n Exiting.." << std::endl;
        exit(1);
    }
    TTF_Font *font = TTF_OpenFont(fontFile.c_str(), size);
    SDL_Color sdlColor = {(Uint8)color.r,(Uint8)color.g,(Uint8)color.b};
    SDL_Surface *Message = TTF_RenderText_Blended(font, text.c_str(), sdlColor);
    float aspect = (float) Message->h / Message->w;
    if(aspect > 1){
        setScale(1,Message->w / aspect);
    } else {
        setScale(aspect, 1);
    }

    textureID = glHelper->loadTexture(Message->h,Message->w,GL_BGRA,Message->pixels);
}