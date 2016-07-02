//
// Created by Engin Manap on 14.03.2016.
//

#include "GUIText.h"

GUIText::GUIText(GLHelper *glHelper, Face *face, const std::string text, const glm::vec3 color) :
        GUIRenderable(glHelper), color(color.x / 256, color.y / 256, color.z / 256), face(face), text(text), height(0),
        width(0), bearingUp(0) {
    //calculate the size of text. This also force glyph load.

    int up = 0;
    const Glyph *glyph;
    for (int i = 0; i < text.length(); ++i) {
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

    std::cout << "for " << text << " up: " << up << ", down: " << bearingUp << ", width: " << width << std::endl;
}

void GUIText::render() {

    float totalAdvance = 0.0f;

    renderProgram->setUniform("inColor", color);

    renderProgram->setUniform("orthogonalProjectionMatrix", glHelper->getOrthogonalProjectionMatrix());

    glm::mat4 currentTransform;

    //Setup position
    float quadPositionX, quadPositionY, quadSizeX, quadSizeY;
    const Glyph *glyph;
    for (int i = 0; i < text.length(); ++i) {
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
        if (isRotated) {
            currentTransform = glm::scale(
                    glm::translate(
                            (glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation)),
                            glm::vec3(quadPositionX, quadPositionY, 0) -
                            glm::vec3(width * scale.x / 2.0f, height * scale.y / 2.0f, 0.0f)),
                    this->scale * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        } else {
            //this branch removes quaternion cast, so double translate is not necessary.
            currentTransform = glm::scale(
                    glm::translate(glm::mat4(1.0f), translate +
                                                    glm::vec3(quadPositionX, quadPositionY, 0) -
                                                    glm::vec3(width * scale.x / 2.0f, height * scale.y / 2.0f,
                                                              0.0f)),
                    this->scale * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        }

        if (!renderProgram->setUniform("worldTransformMatrix", currentTransform)) {
            std::cerr << "failed to set uniform" << std::endl;
        }
        glHelper->attachTexture(glyph->getTextureID());
        glHelper->render(renderProgram->getID(), vao, ebo, (const GLuint) (faces.size() * 3));

        totalAdvance += glyph->getAdvance() / 64;
    }

}

void GUIText::renderDebug() {
    glm::mat4 orthogonalPM = glHelper->getOrthogonalProjectionMatrix();

    glm::mat4 transform = (orthogonalPM * getWorldTransform());

    glm::vec4 upLeft = (transform * glm::vec4(-width / 2, height / 2 - bearingUp, 0.0f, 1.0f));
    glm::vec4 upRight = (transform * glm::vec4(width / 2, height / 2 - bearingUp, 0.0f, 1.0f));
    glm::vec4 downLeft = (transform * glm::vec4(-width / 2, -height / 2 - bearingUp, 0.0f, 1.0f));
    glm::vec4 downRight = (transform * glm::vec4(width / 2, -height / 2 - bearingUp, 0.0f, 1.0f));

    glHelper->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(upRight.x, upRight.y, upRight.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(downLeft.x, downLeft.y, downLeft.z), glm::vec3(downRight.x, downRight.y, downRight.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(downLeft.x, downLeft.y, downLeft.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(upRight.x, upRight.y, upRight.z), glm::vec3(downRight.x, downRight.y, downRight.z),
                       glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), false);
    //per glyph debug render:

    float totalAdvance = 0.0f;
    float advance;

    float quadPositionX, quadPositionY, quadSizeX, quadSizeY;
    const Glyph *glyph;
    glm::mat4 currentTransform;
    for (int i = 0; i < text.length(); ++i) {
        glyph = face->getGlyph(text.at(i));
        quadSizeX = glyph->getSize().x / 2.0f;
        quadSizeY = glyph->getSize().y / 2.0f;

        quadPositionX = totalAdvance + glyph->getBearing().x + quadSizeX; //origin is left side
        quadPositionY = glyph->getBearing().y - quadSizeY; // origin is the bottom line

        if (isRotated) {
            currentTransform = glm::scale(
                    glm::translate(
                            (glm::translate(glm::mat4(1.0f), translate) * glm::mat4_cast(orientation)),
                            glm::vec3(quadPositionX, quadPositionY, 0) -
                            glm::vec3(width * scale.x / 2.0f, height * scale.y / 2.0f, 0.0f)),
                    this->scale * glm::vec3(quadSizeX, quadSizeY, 1.0f)
            );
        } else {
            currentTransform = glm::scale(
                    glm::translate(glm::mat4(1.0f), translate +
                                                    glm::vec3(quadPositionX, quadPositionY, 0) -
                                                    glm::vec3(width * scale.x / 2.0f, height * scale.y / 2.0f, 0.0f)),
                    this->scale * glm::vec3(quadSizeX, quadSizeY, 1.0f)
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

        glHelper->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(upRight.x, upRight.y, upRight.z),
                           glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), false);
        glHelper->drawLine(glm::vec3(downLeft.x, downLeft.y, downLeft.z),
                           glm::vec3(downRight.x, downRight.y, downRight.z), glm::vec3(1.0f, 0.0f, 0.0f),
                           glm::vec3(1.0f, 0.0f, 0.0f), false);
        glHelper->drawLine(glm::vec3(upLeft.x, upLeft.y, upLeft.z), glm::vec3(downLeft.x, downLeft.y, downLeft.z),
                           glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), false);
        glHelper->drawLine(glm::vec3(upRight.x, upRight.y, upRight.z), glm::vec3(downRight.x, downRight.y, downRight.z),
                           glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), false);
    }

}
