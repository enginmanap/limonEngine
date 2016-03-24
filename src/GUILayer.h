//
// Created by Engin Manap on 23.03.2016.
//

#ifndef UBERGAME_GUILAYER_H
#define UBERGAME_GUILAYER_H

#include "GLHelper.h"
#include "Renderable.h"

class GUILayer {
    GLHelper* glHelper;
    int level;
    std::vector<Renderable* > guiElements;

public:
    GUILayer(GLHelper* glHelper, int level): glHelper(glHelper), level(level) {};

    int getLevel(){ return level;}
    //renderable is wrong, this should be guiElement
    void addGuiElement(Renderable* renderable){
        guiElements.push_back(renderable);
    }

    void render();
};


#endif //UBERGAME_GUILAYER_H
