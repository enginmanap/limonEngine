//
// Created by Engin Manap on 23.03.2016.
//

#ifndef LIMONENGINE_GUILAYER_H
#define LIMONENGINE_GUILAYER_H

#include <tinyxml2.h>
#include "../GLHelper.h"

class GUIText;
class BulletDebugDrawer;

class GUILayer {
    GLHelper *glHelper;
    BulletDebugDrawer* debugDrawer;
    int level;
    bool isDebug;
    std::vector<GUIText *> guiElements;

public:
    GUILayer(GLHelper *glHelper, BulletDebugDrawer* debugDrawer, int level) : glHelper(glHelper), debugDrawer(debugDrawer), level(level), isDebug(false) { };

    int getLevel() { return level; }

    bool getDebug() const {
        return isDebug;
    }

    void setDebug(bool isDebug) {
        GUILayer::isDebug = isDebug;
    }

    void addGuiElement(GUIText *guiElement);

    void removeGuiElement(uint32_t guiElementID);


    void render();

    void setupForTime(long time);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *LayersListNode, Options *options);

    GUIText* getRenderableFromCoordinate(const glm::vec2& coordinates);
};


#endif //LIMONENGINE_GUILAYER_H
