//
// Created by Engin Manap on 14.03.2016.
//

#include "GUIText.h"

GUIText::GUIText(GLHelper* glHelper,  Face* face, const std::string text, const glm::lowp_uvec3 color):
        GUIRenderable(glHelper), face(face), text(text) , height(0), width(0), bearingUp(0){
    //calculate the size of text. This also force glyph load.

    int up=0;
    const Glyph* glyph;
    for (int i = 0; i < text.length() ; ++i) {
        glyph = face->getGlyph(text.at(i));
        width += glyph->getBearing().x + (glyph->getAdvance() /64);
        if(up < glyph->getBearing().y ) {
            up = glyph->getBearing().y;
        }
        if(bearingUp < glyph->getSize().y - glyph->getBearing().y) {
            bearingUp = glyph->getSize().y - glyph->getBearing().y;
        }
    }
    /**
     * Last characters advance is not used, size is used instead
     * */
    //width += face->getGlyph(text.at(0))->getSize().x / 2;
    width += face->getGlyph(text.at(text.length()-1))->getSize().x;
    width -= face->getGlyph(text.at(text.length()-1))->getAdvance() / 64;

    height = up+bearingUp;

    std::cout << "for " << text << " up: " << up <<", down: " << bearingUp <<", width: " << width << std::endl;
}

void GUIText::render() {
    GLuint worldTransformlocation, ortoProjLocation;

    float totalAdvance = 0.0f;
    float advance = 0.0f;
    glm::mat4 orthogonalPM = glHelper->getOrthogonalProjectionMatrix();

    renderProgram->getUniformLocation("orthogonalProjectionMatrix", ortoProjLocation);
    glHelper->setUniform(renderProgram->getID(), ortoProjLocation, orthogonalPM);

    if(renderProgram->getUniformLocation("worldTransformMatrix", worldTransformlocation)) {

        glm::vec3 baseTranslate = this->translate + glm::vec3(-width * scale.x /2.0f, -height * scale.y /2.0f, 0.0f);
        glm::vec3 baseScale = this->scale;
        float charTranslateX, charTranslateY, charScaleX, charScaleY;
        const Glyph* glyph;
        for(int i=0; i < text.length(); ++i) {
            glyph = face->getGlyph(text.at(i));
            charScaleX = baseScale.x * (glyph->getSize().x / 2.0f);
            charScaleY = baseScale.y * (glyph->getSize().y / 2.0f);

            charTranslateX = baseTranslate.x + (totalAdvance + charScaleX + glyph->getBearing().x * baseScale.x);
            charTranslateY = baseTranslate.y + (glyph->getBearing().y * baseScale.y - charScaleY);

            glm::mat4 currentTransform = glm::translate(glm::mat4(1.0f), glm::vec3(charTranslateX,charTranslateY,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(charScaleX, charScaleY, 1.0f));

            if(!glHelper->setUniform(renderProgram->getID(), worldTransformlocation, currentTransform)){
                std::cerr << "failed to set uniform" << std::endl;
            }
            glHelper->attachTexture(glyph->getTextureID());
            glHelper->render(renderProgram->getID(), vao, ebo, (const GLuint) (faces.size() * 3));

            advance = glyph->getAdvance() /64 * baseScale.x;
            totalAdvance += advance;

            //FIXME these are debug drawing commands. they need their own method.
            charTranslateX = orthogonalPM[0][0] * charTranslateX + orthogonalPM[3][0];
            charTranslateY = orthogonalPM[1][1] * charTranslateY + orthogonalPM[3][1];

            charScaleX = orthogonalPM[0][0] * charScaleX;
            charScaleY = orthogonalPM[1][1] * charScaleY;

            float up =  charTranslateY + charScaleY;
            float down = charTranslateY - charScaleY;

            float right = charTranslateX  + charScaleX;
            float left = charTranslateX - charScaleX;

            glHelper->drawLine(glm::vec3(left,up,0.0f),glm::vec3(left,down,0.0f),glm::vec3(1.0f, 0.0f, 0.0f),glm::vec3(1.0f, 0.0f, 0.0f), false);
            glHelper->drawLine(glm::vec3(right,up,0.0f),glm::vec3(right,down,0.0f),glm::vec3(1.0f, 0.0f, 0.0f),glm::vec3(1.0f, 0.0f, 0.0f), false);
            glHelper->drawLine(glm::vec3(left,up,0.0f),glm::vec3(right,up,0.0f),glm::vec3(1.0f, 0.0f, 0.0f),glm::vec3(1.0f, 0.0f, 0.0f), false);
            glHelper->drawLine(glm::vec3(left,down,0.0f),glm::vec3(right,down,0.0f),glm::vec3(1.0f, 0.0f, 0.0f),glm::vec3(1.0f, 0.0f, 0.0f), false);
        }
    }
}

void GUIText::renderDebug() {
    glm::mat4 orthogonalPM = glHelper->getOrthogonalProjectionMatrix();
    float charTranslateX, charTranslateY, charScaleX, charScaleY;

    charTranslateX = orthogonalPM[0][0] * translate.x + orthogonalPM[3][0];
    charTranslateY = orthogonalPM[1][1] * translate.y + orthogonalPM[3][1];

    charScaleX = orthogonalPM[0][0] * width/2 * scale.x;
    charScaleY = orthogonalPM[1][1] * height/2 * scale.y;

    float up =  charTranslateY + charScaleY - bearingUp * scale.y * orthogonalPM[1][1];
    float down = charTranslateY - charScaleY - bearingUp * scale.y * orthogonalPM[1][1];

    float right = charTranslateX  + charScaleX;
    float left = charTranslateX - charScaleX;

    glHelper->drawLine(glm::vec3(left,up,0.0f),glm::vec3(left,down,0.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(right,up,0.0f),glm::vec3(right,down,0.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(left,up,0.0f),glm::vec3(right,up,0.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f), false);
    glHelper->drawLine(glm::vec3(left,down,0.0f),glm::vec3(right,down,0.0f),glm::vec3(1.0f, 1.0f, 1.0f),glm::vec3(1.0f, 1.0f, 1.0f), false);
    //std::cout << "for " << text << " up: " << up <<", down: " << down <<", left: " << left << ", right: " << right << std::endl;
}
