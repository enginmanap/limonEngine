//
// Created by Engin Manap on 14.03.2016.
//

#ifndef LIMONENGINE_TEXTRENDERER_H
#define LIMONENGINE_TEXTRENDERER_H

#include <iostream>

#include "../GLHelper.h"
#include "GUIRenderable.h"
#include "../FontManager.h"
#include "../GLHelper.h"


class GUIText : public GUIRenderable {

protected:
    std::string text;
    glm::vec3 color;
    Face *face;
    int glyphAttachPoint = 1;
    int height, width;
    int bearingUp;
public:
    GUIText(GLHelper *glHelper, Face *font, const std::string text, const glm::vec3 color);
    GUIText(GLHelper *glHelper, Face *font, const glm::vec3 color) : GUIText(glHelper, font, "", color) {};

    virtual void updateText(const std::string &text) {
        this->text = text;
    }

    virtual void render();

    void renderDebug(BulletDebugDrawer *debugDrawer);
};


#endif //LIMONENGINE_TEXTRENDERER_H
