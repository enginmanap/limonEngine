//
// Created by Engin Manap on 14.03.2016.
//

#ifndef UBERGAME_TEXTRENDERER_H
#define UBERGAME_TEXTRENDERER_H

#include <iostream>

#include "../GLHelper.h"
#include "GUIRenderable.h"
#include "../FontManager.h"
#include "../GLHelper.h"


class GUIText : public GUIRenderable {
    glm::vec3 color;
    Face *face;
    int height, width;
    int bearingUp;
    int glyphAttachPoint = 1;
protected:
    std::string text;
public:
    GUIText(GLHelper *glHelper, Face *font, const std::string text, const glm::vec3 color);

    virtual void updateText(const std::string &text) {
        this->text = text;
    }

    virtual void render();

    void renderDebug(BulletDebugDrawer *debugDrawer);
};


#endif //UBERGAME_TEXTRENDERER_H
