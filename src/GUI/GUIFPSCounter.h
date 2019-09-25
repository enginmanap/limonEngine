//
// Created by Engin Manap on 9.04.2016.
//

#ifndef LIMONENGINE_GUIFPSCOUNTER_H
#define LIMONENGINE_GUIFPSCOUNTER_H

#include "GUITextBase.h"
#include <SDL2/SDL.h>

class GUIFPSCounter : public GUITextBase {
    Uint32 currentTime;
    Uint32 lastUpdateTime = 0;
    Uint32 previousFrameTimes[100];
    Uint32 lastRenderTime;
    Uint32 lastFrameTime;
    short framePointer;
public:
    GUIFPSCounter(GraphicsInterface* graphicsWrapper, Face *font, const std::string &text, const glm::lowp_uvec3 color) :
            GUITextBase(graphicsWrapper, font, text, color),
            previousFrameTimes{0}, lastRenderTime(0), lastFrameTime(0), framePointer(0) {
    }

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram) override;

    void updateFPS();
};


#endif //LIMONENGINE_GUIFPSCOUNTER_H
