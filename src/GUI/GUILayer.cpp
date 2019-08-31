//
// Created by Engin Manap on 23.03.2016.
//

#include "GUILayer.h"
#include "GUIRenderable.h"
#include "../GameObjects/GUIText.h"
#include "../GameObjects/GUIImage.h"
#include "../GameObjects/GUIButton.h"
#include "../GameObjects/GUIAnimation.h"


class Options;

void GUILayer::renderWithProgram(std::shared_ptr<GLSLProgram> textRenderProgram, std::shared_ptr<GLSLProgram> imageRenderProgram) {
    for (auto it = guiElements.begin(); it != guiElements.end(); ++it) {
        if(it->second == RenderTypes::TEXT) {
            it->first->renderWithProgram(textRenderProgram);
        }
    }

    for (auto it = guiElements.begin(); it != guiElements.end(); ++it) {
        if(it->second == RenderTypes::IMAGE) {
            it->first->renderWithProgram(imageRenderProgram);
        }
    }

    if (isDebug) {
        for (auto it = guiElements.begin(); it != guiElements.end(); ++it) {
            it->first->renderDebug(debugDrawer);
        }
    }
}

void GUILayer::setupForTime(long time){
    for (auto it = guiElements.begin(); it != guiElements.end(); ++it) {
        it->first->setupForTime(time);
    }
}

void GUILayer::addGuiElement(GUIRenderable *guiElement) {
        //guiElements.push_back(guiElement);
        GameObject* guiGameObject = dynamic_cast<GameObject*>(guiElement);
        if(guiGameObject != nullptr) {
            switch (guiGameObject->getTypeID()) {
                case GameObject::ObjectTypes::GUI_TEXT:
                    static_cast<GUIText*>(guiElement)->addedToLayer(this);
                    guiElements.push_back(std::make_pair(guiElement, RenderTypes::TEXT));
                    break;
                case GameObject::ObjectTypes::GUI_IMAGE:
                    static_cast<GUIImage*>(guiElement)->addedToLayer(this);
                    guiElements.push_back(std::make_pair(guiElement, RenderTypes::IMAGE));
                    break;
                case GameObject::ObjectTypes::GUI_BUTTON:
                    static_cast<GUIButton*>(guiElement)->addedToLayer(this);
                    guiElements.push_back(std::make_pair(guiElement, RenderTypes::IMAGE));
                    break;
                case GameObject::ObjectTypes::GUI_ANIMATION:
                    static_cast<GUIAnimation*>(guiElement)->addedToLayer(this);
                    guiElements.push_back(std::make_pair(guiElement, RenderTypes::IMAGE));
                    break;
                default:
                    std::cerr << "A GUI Element add failed to layer because of unknown type!" << std::endl;

            }
        }
}

void GUILayer::removeGuiElement(uint32_t guiElementID) {
    for (size_t i = 0; i < guiElements.size(); ++i) {
        //for non game object elements, this operation is ignored
        GameObject* guiGameObject = dynamic_cast<GameObject*>(guiElements[i].first);
        if(guiGameObject != nullptr) {
            uint32_t worldObjectID = 0;
            switch (guiGameObject->getTypeID()) {
                case GameObject::ObjectTypes::GUI_TEXT:
                    worldObjectID = static_cast<GUIText*>(guiGameObject)->getWorldObjectID();
                    break;
                case GameObject::ObjectTypes::GUI_IMAGE:
                    worldObjectID = static_cast<GUIImage*>(guiGameObject)->getWorldObjectID();
                    break;
                case GameObject::ObjectTypes::GUI_BUTTON:
                    worldObjectID = static_cast<GUIButton*>(guiGameObject)->getWorldObjectID();
                    break;
                case GameObject::ObjectTypes::GUI_ANIMATION:
                    worldObjectID = static_cast<GUIAnimation*>(guiGameObject)->getWorldObjectID();
                    break;
                default:
                    std::cerr << "A GUI Element remove failed to layer because of unknown type!" << std::endl;
            }
            if(worldObjectID == guiElementID) {
                guiElements.erase(guiElements.begin() + i);
                return;
            }
        }
    }
}

bool GUILayer::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *LayersListNode, Options *options) {

    tinyxml2::XMLElement *layerNode= document.NewElement("GUILayer");
    LayersListNode->InsertEndChild(layerNode);

    tinyxml2::XMLElement *levelNode = document.NewElement("Level");
    levelNode->SetText(std::to_string(level).c_str());
    layerNode->InsertEndChild(levelNode);
    for (size_t i = 0; i < guiElements.size(); ++i) {
        GameObject* guiGameObject = dynamic_cast<GameObject*>(guiElements[i].first);
        //for non game object elements, this operation is ignored
        if(guiGameObject != nullptr) {
            switch (guiGameObject->getTypeID()) {
                case GameObject::ObjectTypes::GUI_TEXT:
                    static_cast<GUIText*>(guiGameObject)->serialize(document, layerNode, options);
                    break;
                case GameObject::ObjectTypes::GUI_IMAGE:
                    static_cast<GUIImage*>(guiGameObject)->serialize(document, layerNode, options);
                    break;
                case GameObject::ObjectTypes::GUI_BUTTON:
                    static_cast<GUIButton*>(guiGameObject)->serialize(document, layerNode, options);
                    break;
                case GameObject::ObjectTypes::GUI_ANIMATION:
                    static_cast<GUIAnimation*>(guiGameObject)->serialize(document, layerNode, options);
                    break;
                default:
                    break;//do nothing
            }
        }
    }

    return true;
}

/**
 * TODO: This method doesn't handle angled items, it should be improved.
 *
 * This method returns the first element found. If you need to put multiple elements on top of each other,
 * you should use layers.
 *
 * @param coordinates
 * @return null if no element found.
 */
GUIRenderable *GUILayer::getRenderableFromCoordinate(const glm::vec2 &coordinates) {
    glm::vec2 aabbMin, aabbMax;
    for (size_t i = 0; i < guiElements.size(); ++i) {
        guiElements[i].first->getAABB(aabbMin, aabbMax);
        if(aabbMin.x <= coordinates.x && coordinates.x <= aabbMax.x &&
                    aabbMin.y <= coordinates.y && coordinates.y <= aabbMax.y ) {
            return guiElements[i].first;
        }
    }

    return nullptr;
}

std::vector<GameObject*> GUILayer::getGuiElements() {
    std::vector<GameObject*> elements;
    for (auto iterator = guiElements.begin(); iterator != guiElements.end(); ++iterator) {
        GameObject* element = dynamic_cast<GameObject*> (iterator->first);
        if(element != nullptr) {
            elements.push_back(element);
        }
    }
    return elements;
}
