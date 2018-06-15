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
    GUIText(GLHelper *glHelper, uint32_t id, Face *font, const std::string text, const glm::vec3 color);
    GUIText(GLHelper *glHelper, uint32_t id, Face *font, const glm::vec3 color) : GUIText(glHelper, id, font, "", color) {};
    static GUIText* deserialize(tinyxml2::XMLElement *GUIRenderableNode, GLHelper* glHelper, FontManager* fontManager); //will turn into factory class at some point

    virtual void render();

    void renderDebug(BulletDebugDrawer *debugDrawer);

    void updateText(const std::string& text) {
        this->text = text;
        name = this->text + "-" + std::to_string(getWorldID());
    }

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;

    virtual bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) override;
};


#endif //LIMONENGINE_TEXTRENDERER_H
