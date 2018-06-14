//
// Created by Engin Manap on 9.04.2016.
//

#ifndef LIMONENGINE_GUIFPSCOUNTER_H
#define LIMONENGINE_GUIFPSCOUNTER_H

#include "GUIText.h"
#include <SDL2/SDL.h>

class GUIFPSCounter : public GUIText {
    Uint32 currentTime;
    Uint32 lastUpdateTime = 0;
    Uint32 previousFrameTimes[100];
    Uint32 lastRenderTime;
    Uint32 lastFrameTime;
    short framePointer;
public:
    GUIFPSCounter(GLHelper *glHelper, uint32_t id, Face *font, const std::string &text, const glm::lowp_uvec3 color) :
            GUIText(glHelper, id, font, text, color),
            previousFrameTimes{0}, lastRenderTime(0), lastFrameTime(0), framePointer(0) {
            this->name = "FPS Counter";
    }

    void render();

    void updateFPS();

    bool serialize(tinyxml2::XMLDocument &document __attribute((unused)), tinyxml2::XMLElement *parentNode __attribute((unused))) override { return true;};//This object is used for debug logging, it should not be serialized
};


#endif //LIMONENGINE_GUIFPSCOUNTER_H
