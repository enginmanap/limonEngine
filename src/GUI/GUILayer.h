//
// Created by Engin Manap on 23.03.2016.
//

#ifndef LIMONENGINE_GUILAYER_H
#define LIMONENGINE_GUILAYER_H

#include <tinyxml2.h>
#include "../GLHelper.h"

class BulletDebugDrawer;
class GUIRenderable;

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

    void addGuiElement(GUIRenderable *guiElement);

    void removeGuiElement(uint32_t guiElementID);


    void render();

    void setupForTime(long time);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *LayersListNode, Options *options);

    GUIRenderable* getRenderableFromCoordinate(const glm::vec2& coordinates);
};


#endif //LIMONENGINE_GUILAYER_H
