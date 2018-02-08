//
// Created by Engin Manap on 9.04.2016.
//

#ifndef LIMONENGINE_GUIFPSCOUNTER_H
#define LIMONENGINE_GUIFPSCOUNTER_H

#include "GUIText.h"
#include <SDL2/SDL.h>

/**
 * FIXME
 * This class has 2 issues.
 * 1) The base class is accessable, which feels like an encapsulation violation. I need to check
 * 2) It uses SDL2 code directly, which creates a coupling I rather not Have
 */
class GUIFPSCounter : public GUIText {
    Uint32 currentTime;
    Uint32 previousFrameTimes[100];
    Uint32 lastRenderTime;
    Uint32 lastFrameTime;
    short framePointer;
public:
    GUIFPSCounter(GLHelper *glHelper, Face *font, const std::string &text, const glm::lowp_uvec3 color) :
            GUIText(glHelper, font, text, color),
            previousFrameTimes{0}, lastRenderTime(0), lastFrameTime(0), framePointer(0) {

    }

    void updateText(const std::string &text) {
        std::cerr << "Text update on GUIFPSCounter class is invalid operation." << std::endl;
    }

    void render();

    void updateFPS();
};


#endif //LIMONENGINE_GUIFPSCOUNTER_H
