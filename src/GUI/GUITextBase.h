//
// Created by Engin Manap on 14.03.2016.
//

#ifndef LIMONENGINE_TEXTRENDERER_H
#define LIMONENGINE_TEXTRENDERER_H

#include <iostream>

#include "Graphics/OpenGLGraphics.h"
#include "GUIRenderable.h"
#include "../FontManager.h"
#include "Graphics/OpenGLGraphics.h"


class GUITextBase : public GUIRenderable {
    void calculateSizes();

protected:
    std::string text;
    glm::vec3 color;
    Face *face;
    int glyphAttachPoint = 1;
    int height, width;
    int bearingUp;

    static std::shared_ptr<GLSLProgram> textRenderProgram;

    //Don't allow constructing of this object itself
    GUITextBase(OpenGLGraphics *glHelper, Face *font, const std::string text, const glm::vec3 color);
    GUITextBase(OpenGLGraphics *glHelper, Face *font, const glm::vec3 color) : GUITextBase(glHelper, font, "",
                                                                                           color) {};

public:
    virtual ~GUITextBase(){
        //delete renderProgram;// since the program is shared, don't remove
    };


    virtual void renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram) override;

    virtual void renderDebug(BulletDebugDrawer *debugDrawer);

    void updateText(const std::string& text) {
        this->text = text;
        calculateSizes();
    }

    void getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const override;

};


#endif //LIMONENGINE_TEXTRENDERER_H
