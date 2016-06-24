//
// Created by Engin Manap on 23.03.2016.
//

#include "GUILayer.h"

void GUILayer::render(Light *light) {
    for (std::vector<GUIRenderable *>::iterator it = guiElements.begin(); it != guiElements.end(); ++it) {
        (*it)->render(light);
    }
    if (isDebug) {
        for (std::vector<GUIRenderable *>::iterator it = guiElements.begin(); it != guiElements.end(); ++it) {
            (*it)->renderDebug();
        }
    }
}