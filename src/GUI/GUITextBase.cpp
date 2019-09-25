//
// Created by Engin Manap on 14.03.2016.
//

#include "GUITextBase.h"


std::shared_ptr<GLSLProgram> GUITextBase::textRenderProgram = nullptr;

void GUITextBase::calculateSizes() {
    width = 0;
    height = 0;
    bearingUp = 0;
    if (text.length() != 0) {
        //calculate the size of text. This also force glyph load.

        int up = 0;
        const Glyph *glyph;
        for (unsigned int i = 0; i < text.length(); ++i) {
            glyph = face->getGlyph(text.at(i));
            if (i == 0) {
                //for first element, add the bearing
                width = glyph->getBearing().x;
            }
            width += (glyph->getAdvance() / 64);
            up = std::max(up, glyph->getBearing().y);
            bearingUp = std::max(bearingUp, glyph->getSize().y - glyph->getBearing().y + 1);
        }
        /**
         * Last characters advance is not used, size is used instead
         * */
        //width += face->getGlyph(text.at(0))->getSize().x / 2;
        width += face->getGlyph(text.at(text.length() - 1))->getSize().x;
        width += face->getGlyph(text.at(text.length() - 1))->getBearing().x;
        width -= face->getGlyph(text.at(text.length() - 1))->getAdvance() / 64;

        height = up + bearingUp;
    }
}

GUITextBase::GUITextBase(OpenGLGraphics *glHelper, Face *face, const std::string text, const glm::vec3 color) :
        GUIRenderable(glHelper), text(text), color(color.x / 256, color.y / 256, color.z / 256), face(face), height(0),
        width(0), bearingUp(0) {
    calculateSizes();
}

void GUITextBase::renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram){

    float totalAdvance = 0.0f;

    renderProgram->setUniform("inColor", color);

    renderProgram->setUniform("orthogonalProjectionMatrix", glHelper->getOrthogonalProjectionMatrix());

    glm::mat4 currentTransform;

    this->transformation.getWorldTransform();//this is called incase part of animation, if not it returns value from cache, so no performance hit.

    //Setup position
    float quadPositionX, quadPositionY, quadSizeX, quadSizeY;
    const Glyph *glyph;
    for (unsigned int i = 0; i < text.length(); ++i) {
        glyph = face->getGlyph(text.at(i));
        quadSizeX = glyph->getSize().x / 2.0f;
        quadSizeY = glyph->getSize().y / 2.0f;

        quadPositionX = totalAdvance + glyph->getBearing().x + quadSizeX; //origin is left side
        quadPositionY = glyph->getBearing().y - quadSizeY; // origin is the bottom line


        /**
         * the scale, translate and rotate functions apply the transition to first element, so the below code is
         * scale to quadSizeX/Y * this->scale first,
         * than translate to quadPositionX/Y - width/height* scale/2,
         * than rotate using orientation,
         * than translate to this->translate
         *
         * The double translate is because we want to rotate from center of the text.
         */
        if (transformation.isRotated()) {
            currentTransform = glm::scale(
                    glm::translate(
                            (glm::translate(glm::mat4(1.0f), transformation.getTranslate()) * glm::mat4_cast(transformation.getOrientation())),
                            glm::vec3(quadPositionX, quadPositionY, 0) -
                            glm::vec3(width * transformation.getScale().x / 2.0f, height * transformation.getScale().y / 2.0f, 0.0f)),
                    this->transformation.getScale() * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        } else {
            //this branch removes quaternion cast, so double translate is not necessary.
            currentTransform = glm::scale(
                    glm::translate(glm::mat4(1.0f), transformation.getTranslate() +
                                                    glm::vec3(quadPositionX, quadPositionY, 0) -
                                                    glm::vec3(width * transformation.getScale().x / 2.0f, height * transformation.getScale().y / 2.0f,
                                                              0.0f)),
                    this->transformation.getScale() * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        }
        currentTransform[3][2] = this->getTransformation()->getTranslate().z;//alpha is set on translate z, since it is not used
        if (!renderProgram->setUniform("worldTransformMatrix", currentTransform)) {
            std::cerr << "failed to set uniform \"worldTransformMatrix\"" << std::endl;
        }

        if (!renderProgram->setUniform("GUISampler", glyphAttachPoint)) {
            std::cerr << "failed to set uniform \"GUISampler\"" << std::endl;
        }
        glHelper->attachTexture(glyph->getTextureID(), glyphAttachPoint);
        glHelper->render(renderProgram->getID(), vao, ebo, (GLuint) (faces.size() * 3));

        totalAdvance += (glyph->getAdvance() / 64.0f) * this->getScale().x;
    }

}

void GUITextBase::renderDebug(BulletDebugDrawer *debugDrawer) {
    glm::mat4 orthogonalPM = glHelper->getOrthogonalProjectionMatrix();

    glm::mat4 transform = (orthogonalPM * transformation.getWorldTransform());

    glm::vec4 upLeft    = (transform * glm::vec4(-width / 2.0f,  height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 upRight   = (transform * glm::vec4( width / 2.0f,  height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 downLeft  = (transform * glm::vec4(-width / 2.0f, -height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 downRight = (transform * glm::vec4( width / 2.0f, -height / 2.0f - bearingUp, 0.0f, 1.0f));

    debugDrawer->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(upRight.x, upRight.y, upRight.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    debugDrawer->drawLine(glm::vec3(downLeft.x, downLeft.y, downLeft.z), glm::vec3(downRight.x, downRight.y, downRight.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    debugDrawer->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(downLeft.x, downLeft.y, downLeft.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    debugDrawer->drawLine(glm::vec3(upRight.x, upRight.y, upRight.z), glm::vec3(downRight.x, downRight.y, downRight.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    //per glyph debug render:

    float totalAdvance = 0.0f;
    float advance;

    float quadPositionX, quadPositionY, quadSizeX, quadSizeY;
    const Glyph *glyph;
    glm::mat4 currentTransform;
    for (unsigned int i = 0; i < text.length(); ++i) {
        glyph = face->getGlyph(text.at(i));
        quadSizeX = glyph->getSize().x / 2.0f;
        quadSizeY = glyph->getSize().y / 2.0f;

        quadPositionX = totalAdvance + glyph->getBearing().x + quadSizeX; //origin is left side
        quadPositionY = glyph->getBearing().y - quadSizeY; // origin is the bottom line

        if (transformation.isRotated()) {
            currentTransform = glm::scale(
                    glm::translate(
                            (glm::translate(glm::mat4(1.0f), transformation.getTranslate()) * glm::mat4_cast(transformation.getOrientation())),
                            glm::vec3(quadPositionX, quadPositionY, 0) -
                            glm::vec3(width * transformation.getScale().x / 2.0f, height * transformation.getScale().y / 2.0f, 0.0f)),
                    this->transformation.getScale() * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        } else {
            currentTransform = glm::scale(
                    glm::translate(glm::mat4(1.0f), transformation.getTranslate() +
                                                    glm::vec3(quadPositionX, quadPositionY, 0) -
                                                    glm::vec3(width * transformation.getScale().x / 2.0f, height * transformation.getScale().y / 2.0f, 0.0f)),
                    this->transformation.getScale() * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        }
        advance = glyph->getAdvance() / 64;
        totalAdvance += advance;

        //all variables are reused
        transform = (orthogonalPM * currentTransform);
        upLeft = (transform * glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f));
        upRight = (transform * glm::vec4(1.0f, 1.0f, 0.0f, 1.0f));
        downLeft = (transform * glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f));
        downRight = (transform * glm::vec4(1.0f, -1.0f, 0.0f, 1.0f));

        debugDrawer->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(upRight.x, upRight.y, upRight.z),
                           glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), false);
        debugDrawer->drawLine(glm::vec3(downLeft.x, downLeft.y, downLeft.z),
                           glm::vec3(downRight.x, downRight.y, downRight.z), glm::vec3(1.0f, 0.0f, 0.0f),
                           glm::vec3(1.0f, 0.0f, 0.0f), false);
        debugDrawer->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(downLeft.x, downLeft.y, downLeft.z),
                           glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), false);
        debugDrawer->drawLine(glm::vec3(upRight.x, upRight.y, upRight.z), glm::vec3(downRight.x, downRight.y, downRight.z),
                           glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), false);
    }

}

void GUITextBase::getAABB(glm::vec2 &aabbMin, glm::vec2 &aabbMax) const {
    Transformation temp = transformation;
    glm::vec4 upRight   = (temp.getWorldTransform() * glm::vec4( width / 2.0f,  height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 downLeft  = (temp.getWorldTransform() * glm::vec4(-width / 2.0f, -height / 2.0f - bearingUp, 0.0f, 1.0f));

    //it is possible with rotation the up value to be lower, right value to be more left then left. assign by check;
    aabbMin.x = std::min(downLeft.x, upRight.x);
    aabbMin.y = std::min(downLeft.y, upRight.y);

    aabbMax.x = std::max(downLeft.x, upRight.x);
    aabbMax.y = std::max(downLeft.y, upRight.y);
}
