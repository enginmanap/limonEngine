//
// Created by Engin Manap on 14.03.2016.
//

#ifndef UBERGAME_TEXTRENDERER_H
#define UBERGAME_TEXTRENDERER_H

#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "GLHelper.h"
#include "GUIRenderable.h"

class GUIText : public GUIRenderable {

public:
    GUIText(GLHelper* glHelper, TTF_Font* font, const std::string text, const glm::lowp_uvec3 color);

    void updateText(std::string text){
        std::cerr << "text update is not implemented" << std::endl;
    }
};


#endif //UBERGAME_TEXTRENDERER_H
