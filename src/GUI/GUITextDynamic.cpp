//
// Created by engin on 14.11.2017.
//

#include <SDL_timer.h>
#include "GUITextDynamic.h"


void GUITextDynamic::renderWithProgram(std::shared_ptr<GLSLProgram> renderProgram) {
    //first move all logs to our list
    Logger::LogLine* logLine = source->getLog();
    while(logLine != nullptr) {
        this->textList.push_back(TextLine(logLine,logLineCount++));
        delete logLine;
        logLine = source->getLog();
    }
    float totalAdvance = 0.0f;

    renderProgram->setUniform("inColor", color);

    renderProgram->setUniform("orthogonalProjectionMatrix", graphicsWrapper->getOrthogonalProjectionMatrix());

    glm::mat4 currentTransform;

    //Setup position
    float quadPositionX, quadPositionY, quadSizeX, quadSizeY;
    const Glyph *glyph;
    if(textList.empty()) {
        return;
    }
    int lineCount=1;//the line 0 would be out of the box

    int MaxLineCount = (height)/(lineHeight);
    int removeLineCount = textList.size() + totalExtraLines - MaxLineCount;
    if(removeLineCount > 0) {
        std::list<TextLine>::iterator it = textList.begin();
        std::advance(it,removeLineCount);
        for(std::list<TextLine>::iterator lineIt = textList.begin(); lineIt != it; lineIt++) {
            totalExtraLines = totalExtraLines - lineIt->extraLines;
        }
        textList.erase(textList.begin(),it);
    }

    for(std::list<TextLine>::iterator lineIt = textList.begin(); lineIt != textList.end(); lineIt++, lineCount++) {
        if(renderSetupTime - lineIt->time > duration) {//this is done here, because this way we don't iterate the list twice
            std::list<TextLine>::iterator test = lineIt;
            lineIt++;
            totalExtraLines = totalExtraLines - test->extraLines;
            textList.erase(test);
        } else {
            for (unsigned int character = 0; character < lineIt->text.length(); ++character) {
                glm::vec3 lineTranslate = transformation.getTranslate() + glm::vec3(0, height - (lineCount * lineHeight), 0);

                glyph = face->getGlyph(lineIt->text.at(character));
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
                                    (glm::translate(glm::mat4(1.0f), lineTranslate) * glm::mat4_cast(transformation.getOrientation())),
                                    glm::vec3(quadPositionX, quadPositionY, 0) -
                                    glm::vec3(width * transformation.getScale().x / 2.0f, height * transformation.getScale().y / 2.0f, 0.0f)),
                            this->transformation.getScale() * glm::vec3(quadSizeX, quadSizeY, 1.0f)
                    );
                } else {
                    //this branch removes quaternion cast, so double translate is not necessary.
                    currentTransform = glm::scale(
                            glm::translate(glm::mat4(1.0f), lineTranslate +
                                                            glm::vec3(quadPositionX, quadPositionY, 0) -
                                                            glm::vec3(width * transformation.getScale().x / 2.0f, height * transformation.getScale().y / 2.0f,
                                                                      0.0f)),
                            this->transformation.getScale() * glm::vec3(quadSizeX, quadSizeY, 1.0f)
                    );
                }

                if (!renderProgram->setUniform("worldTransformMatrix", currentTransform)) {
                    std::cerr << "failed to set uniform \"worldTransformMatrix\"" << std::endl;
                }

                if (!renderProgram->setUniform("GUISampler", glyphAttachPoint)) {
                    std::cerr << "failed to set uniform \"GUISampler\"" << std::endl;
                }
                graphicsWrapper->attachTexture(glyph->getTextureID(), glyphAttachPoint);
                graphicsWrapper->render(renderProgram->getID(), vao, ebo, (GLuint) (faces.size() * 3));

                totalAdvance += glyph->getAdvance() / 64;
                if(totalAdvance + maxCharWidth >= width) {
                    lineCount++;
                    totalAdvance = 0;
                    if(!lineIt->renderedBefore) {
                        lineIt->extraLines++;
                        totalExtraLines++;
                    }
                }
            }
            lineIt->renderedBefore = true;
            totalAdvance = 0;
        }

    }

}