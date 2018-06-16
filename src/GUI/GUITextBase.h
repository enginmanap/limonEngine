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


class GUITextBase : public GUIRenderable {

protected:
    std::string text;
    glm::vec3 color;
    Face *face;
    int glyphAttachPoint = 1;
    int height, width;
    int bearingUp;

    //Don't allow constructing of this object itself
    GUITextBase(GLHelper *glHelper, Face *font, const std::string text, const glm::vec3 color);
    GUITextBase(GLHelper *glHelper, Face *font, const glm::vec3 color) : GUITextBase(glHelper, font, "",
                                                                                                  color) {};

public:
    virtual void render();

    virtual void renderDebug(BulletDebugDrawer *debugDrawer);

    void updateText(const std::string& text) {
        this->text = text;
    }

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;

};


#endif //LIMONENGINE_TEXTRENDERER_H
