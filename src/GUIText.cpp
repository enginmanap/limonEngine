//
// Created by Engin Manap on 14.03.2016.
//

#include "GUIText.h"

GUIText::GUIText(GLHelper* glHelper,  Face* face, const std::string text, const glm::lowp_uvec3 color):
        GUIRenderable(glHelper), face(face), text(text){

}

void GUIText::render() {
    GLuint worldTransformlocation, ortoProjLocation;

    float totalAdvance = 0;
    float advance = 0;
    glm::mat4 orthogonalPM = glHelper->getOrthogonalProjectionMatrix();

    renderProgram->getUniformLocation("orthogonalProjectionMatrix", ortoProjLocation);
    glHelper->setUniform(renderProgram->getID(), ortoProjLocation, orthogonalPM);

    if(renderProgram->getUniformLocation("worldTransformMatrix", worldTransformlocation)) {

        glm::vec3 baseTranslate = this->translate;
        glm::vec3 baseScale = this->scale;
        float charTranslateX, charTranslateY, charScaleX, charScaleY;
        const Glyph* glyph;
        for(int i=0; i < text.length(); ++i) {
            glyph = face->getGlyph(text.at(i));
            charScaleX = baseScale.x * (glyph->getSize().x / 2);
            charScaleY = baseScale.y * (glyph->getSize().y / 2);

            charTranslateX = baseTranslate.x + (totalAdvance + charScaleX + glyph->getBearing().x * baseScale.x);
            charTranslateY = baseTranslate.y + (glyph->getBearing().y * baseScale.y - charScaleY);

            glm::mat4 currentTransform = glm::translate(glm::mat4(1.0f), glm::vec3(charTranslateX,charTranslateY,0)) * glm::scale(glm::mat4(1.0f), glm::vec3(charScaleX, charScaleY, 1.0f));

            if(!glHelper->setUniform(renderProgram->getID(), worldTransformlocation, currentTransform)){
                std::cerr << "failed to set uniform" << std::endl;
            }
            glHelper->attachTexture(glyph->getTextureID());
            glHelper->render(renderProgram->getID(), vao, ebo, faces.size() * 3);

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
