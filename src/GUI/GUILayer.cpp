//
// Created by Engin Manap on 23.03.2016.
//

#include "GUILayer.h"
#include "GUIRenderable.h"

void GUILayer::render() {
    for (std::vector<GUIRenderable *>::iterator it = guiElements.begin(); it != guiElements.end(); ++it) {
        (*it)->render();
    }
    if (isDebug) {
        for (std::vector<GUIRenderable *>::iterator it = guiElements.begin(); it != guiElements.end(); ++it) {
            (*it)->renderDebug(debugDrawer);
        }
    }
}

void GUILayer::setupForTime(long time){
    for (std::vector<GUIRenderable *>::iterator it = guiElements.begin(); it != guiElements.end(); ++it) {
        (*it)->setupForTime(time);
    }
}

void GUILayer::addGuiElement(GUIRenderable *guiElement) {
        guiElements.push_back(guiElement);
        guiElement->addedToLayer(this);
}

void GUILayer::removeGuiElement(uint32_t guiElementID) {
    for (size_t i = 0; i < guiElements.size(); ++i) {
        if(guiElements[i]->getWorldID() == guiElementID) {
            guiElements.erase(guiElements.begin() + i);
        }
    }
}

bool GUILayer::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *LayersListNode) {

    tinyxml2::XMLElement *layerNode= document.NewElement("GUILayer");
    LayersListNode->InsertEndChild(layerNode);

    tinyxml2::XMLElement *levelNode = document.NewElement("Level");
    levelNode->SetText(std::to_string(level).c_str());
    layerNode->InsertEndChild(levelNode);

    for (size_t i = 0; i < guiElements.size(); ++i) {
        guiElements[i]->serialize(document, layerNode);
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
        guiElements[i]->getAABB(aabbMin, aabbMax);
        if(aabbMin.x <= coordinates.x && coordinates.x <= aabbMax.x &&
                    aabbMin.y <= coordinates.y && coordinates.y <= aabbMax.y ) {
            return guiElements[i];
        }
    }

    return nullptr;
}
