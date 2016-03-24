//
// Created by Engin Manap on 23.03.2016.
//

#ifndef UBERGAME_GUILAYER_H
#define UBERGAME_GUILAYER_H

#include "GLHelper.h"
#include "GUIRenderable.h"

class GUILayer {
    GLHelper* glHelper;
    int level;
    std::vector<GUIRenderable* > guiElements;

public:
    GUILayer(GLHelper* glHelper, int level): glHelper(glHelper), level(level) {};

    int getLevel(){ return level;}
    void addGuiElement(GUIRenderable* guiElement){
        guiElements.push_back(guiElement);
    }

    void render();
};


#endif //UBERGAME_GUILAYER_H
