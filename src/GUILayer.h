//
// Created by Engin Manap on 23.03.2016.
//

#ifndef UBERGAME_GUILAYER_H
#define UBERGAME_GUILAYER_H

#include "GLHelper.h"
#include "GUIRenderable.h"

class GUILayer {
    GLHelper *glHelper;
    int level;
    bool isDebug;
    std::vector<GUIRenderable *> guiElements;

public:
    GUILayer(GLHelper *glHelper, int level) : glHelper(glHelper), level(level), isDebug(false) { };

    int getLevel() { return level; }

    bool getDebug() const {
        return isDebug;
    }

    void setDebug(bool isDebug) {
        GUILayer::isDebug = isDebug;
    }

    void addGuiElement(GUIRenderable *guiElement) {
        guiElements.push_back(guiElement);
    }

    void render(Light *light);
};


#endif //UBERGAME_GUILAYER_H
