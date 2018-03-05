//
// Created by Engin Manap on 23.03.2016.
//

#ifndef LIMONENGINE_GUILAYER_H
#define LIMONENGINE_GUILAYER_H

#include "../GLHelper.h"
#include "GUIRenderable.h"

class GUILayer {
    GLHelper *glHelper;
    BulletDebugDrawer* debugDrawer;
    int level;
    bool isDebug;
    std::vector<GUIRenderable *> guiElements;

public:
    GUILayer(GLHelper *glHelper, BulletDebugDrawer* debugDrawer, int level) : glHelper(glHelper), debugDrawer(debugDrawer), level(level), isDebug(false) { };

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

    void render();

    void setupForTime(long time);
};


#endif //LIMONENGINE_GUILAYER_H
