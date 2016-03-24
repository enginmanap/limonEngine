//
// Created by Engin Manap on 14.03.2016.
//

#ifndef UBERGAME_TEXTRENDERER_H
#define UBERGAME_TEXTRENDERER_H

#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "GLHelper.h"
#include "GUIRenderable.h"

class TextRenderer : public GUIRenderable {

public:
    TextRenderer(GLHelper* glHelper, const std::string fontFile, const std::string text, const int size, const glm::lowp_uvec3 color);



    ~TextRenderer() {
        TTF_Quit();
    }

    void updateText(std::string text){
        std::cerr << "text update is not implemented" << std::endl;
    }
};


#endif //UBERGAME_TEXTRENDERER_H
