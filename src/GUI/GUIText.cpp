//
// Created by Engin Manap on 14.03.2016.
//

#include "GUIText.h"

GUIText::GUIText(GLHelper *glHelper, uint32_t id, Face *face, const std::string text, const glm::vec3 color) :
        GUIRenderable(glHelper, id), text(text), color(color.x / 256, color.y / 256, color.z / 256), face(face), height(0),
        width(0), bearingUp(0) {

    if(text.length() != 0) {
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

        //std::cout << "for " << text << " up: " << up << ", down: " << bearingUp << ", width: " << width << std::endl;
        name = this->text + "-" + std::to_string(getWorldID());
    } else {
        name = "Gui_Text-" + std::to_string(getWorldID());
        std::cout << "No text provided, rendering for empty" << std::endl;
    }


}

void GUIText::render() {

    float totalAdvance = 0.0f;

    renderProgram->setUniform("inColor", color);

    renderProgram->setUniform("orthogonalProjectionMatrix", glHelper->getOrthogonalProjectionMatrix());

    glm::mat4 currentTransform;

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

        if (!renderProgram->setUniform("worldTransformMatrix", currentTransform)) {
            std::cerr << "failed to set uniform \"worldTransformMatrix\"" << std::endl;
        }

        if (!renderProgram->setUniform("GUISampler", glyphAttachPoint)) {
            std::cerr << "failed to set uniform \"GUISampler\"" << std::endl;
        }
        glHelper->attachTexture(glyph->getTextureID(), glyphAttachPoint);
        glHelper->render(renderProgram->getID(), vao, ebo, (const GLuint) (faces.size() * 3));

        totalAdvance += glyph->getAdvance() / 64;
    }

}

void GUIText::renderDebug(BulletDebugDrawer *debugDrawer) {
    glm::mat4 orthogonalPM = glHelper->getOrthogonalProjectionMatrix();

    glm::mat4 transform = (orthogonalPM * transformation.getWorldTransform());

    glm::vec4 upLeft = (transform * glm::vec4(-width / 2.0f, height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 upRight = (transform * glm::vec4(width / 2.0f, height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 downLeft = (transform * glm::vec4(-width / 2.0f, -height / 2.0f - bearingUp, 0.0f, 1.0f));
    glm::vec4 downRight = (transform * glm::vec4(width / 2.0f, -height / 2.0f - bearingUp, 0.0f, 1.0f));

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

bool GUIText::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode) {
    tinyxml2::XMLElement *guiTextNode = document.NewElement("GUIElement");
    parentNode->InsertEndChild(guiTextNode);

    tinyxml2::XMLElement *currentElement = document.NewElement("Type");
    currentElement->SetText("GUIText");
    guiTextNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ID");
    currentElement->SetText(getWorldID());
    guiTextNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Text");
    currentElement->SetText(this->text.c_str());
    guiTextNode->InsertEndChild(currentElement);

    tinyxml2::XMLElement *fontNode = document.NewElement("Font");
    guiTextNode->InsertEndChild(fontNode);

    currentElement = document.NewElement("Path");
    currentElement->SetText(this->face->getPath().c_str());
    fontNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("Size");
    currentElement->SetText(std::to_string(this->face->getSize()).c_str());
    fontNode->InsertEndChild(currentElement);

    tinyxml2::XMLElement * parent = document.NewElement("Color");
    currentElement = document.NewElement("R");
    currentElement->SetText(color.r * 256);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("G");
    currentElement->SetText(color.g * 256);
    parent->InsertEndChild(currentElement);
    currentElement = document.NewElement("B");
    currentElement->SetText(color.b * 256);
    parent->InsertEndChild(currentElement);
    guiTextNode->InsertEndChild(parent);

    transformation.serialize(document, guiTextNode);
    return true;
}

GUIText *GUIText::deserialize(tinyxml2::XMLElement *GUIRenderableNode, GLHelper* glHelper, FontManager* fontManager) {

    tinyxml2::XMLElement* GUIRenderableAttribute;

    GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Type");
    if (GUIRenderableAttribute == nullptr) {
        std::cerr << "GUI renderable must have a type. Skipping" << std::endl;
        return nullptr;
    }
    std::string type = GUIRenderableAttribute->GetText();
    if(type == "GUIText") {
        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("ID");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI renderable must have a ID. Skipping" << std::endl;
            return nullptr;
        }
        uint32_t id = std::stoi(GUIRenderableAttribute->GetText());
        std::string text;

        GUIRenderableAttribute = GUIRenderableNode->FirstChildElement("Text");
        if (GUIRenderableAttribute == nullptr) {
            std::cout << "GUI Text without text found, assuming empty" << std::endl;
        } else {
            text = GUIRenderableAttribute->GetText();
        }

        //now get the font
        tinyxml2::XMLElement* FontAttribute = GUIRenderableNode->FirstChildElement("Font");


        GUIRenderableAttribute = FontAttribute->FirstChildElement("Path");
        if (GUIRenderableAttribute == nullptr) {
            std::cout << "GUI Text Font path can't be read. Skipping" << std::endl;
            return nullptr;
        }
        std::string path = GUIRenderableAttribute->GetText();

        uint32_t size;
        GUIRenderableAttribute = FontAttribute->FirstChildElement("Size");
        if (GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI Text font size can't be read. Assumin 32" << std::endl;
            size = 32;
        } else {
            size = std::stoi(GUIRenderableAttribute->GetText());
        }

        //now read the color information
        glm::vec3 color;

        tinyxml2::XMLElement* colorNode = GUIRenderableNode->FirstChildElement("Color");
        if (colorNode == nullptr) {
            color.x = color.y = color.z = 0.0f;
        } else {
            GUIRenderableAttribute = colorNode->FirstChildElement("R");
            if (GUIRenderableAttribute != nullptr) {
                color.x = std::stof(GUIRenderableAttribute->GetText());
            } else {
                color.x = 0.0f;
            }
            GUIRenderableAttribute = colorNode->FirstChildElement("G");
            if (GUIRenderableAttribute != nullptr) {
                color.y = std::stof(GUIRenderableAttribute->GetText());
            } else {
                color.y = 0.0f;
            }
            GUIRenderableAttribute = colorNode->FirstChildElement("B");
            if (GUIRenderableAttribute != nullptr) {
                color.z = std::stof(GUIRenderableAttribute->GetText());
            } else {
                color.z = 0.0f;
            }
        }

        GUIRenderableAttribute =  GUIRenderableNode->FirstChildElement("Transformation");
        if(GUIRenderableAttribute == nullptr) {
            std::cerr << "GUI Text does not have transformation. Skipping" << std::endl;
            return nullptr;
        }
        Transformation tr;
        tr.deserialize(GUIRenderableAttribute);

        //now we have everything, create the GUI Text
        GUIText* element = new GUIText(glHelper, id, fontManager->getFont(path, size), text, color);
        element->getTransformation()->setTranslate(tr.getTranslate());
        element->getTransformation()->setOrientation(tr.getOrientation());
        return element;
    }

    //unknown type case
    return nullptr;
}