//
// Created by Engin Manap on 14.03.2016.
//

#ifndef UBERGAME_TEXTRENDERER_H
#define UBERGAME_TEXTRENDERER_H

#include <SDL2/SDL_ttf.h>
#include <iostream>
#include "GLHelper.h"
#include "GUIRenderable.h"
#include "FontManager.h"


class GUIText : public GUIRenderable {
    Face* face;
    std::string text;
    int height,width;
    int bearingUp;
public:
    GUIText(GLHelper* glHelper, Face* font, const std::string text, const glm::lowp_uvec3 color);

    void updateText(std::string text){
        std::cerr << "text update is not implemented" << std::endl;
    }

    void render();
    void renderDebug();
};


#endif //UBERGAME_TEXTRENDERER_H
