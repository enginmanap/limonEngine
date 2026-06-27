//
// Created by Engin Manap on 9.04.2016.
//

#ifndef LIMONENGINE_GUIFPSCOUNTER_H
#define LIMONENGINE_GUIFPSCOUNTER_H

#include "GUITextBase.h"
#include <cstdint>

class GUIFPSCounter : public GUITextBase {
    static const size_t PREVIOUS_FRAME_COUNT = 300;
    uint64_t currentTime;
    uint64_t lastUpdateTime = 0;
    uint64_t previousFrameTimes[PREVIOUS_FRAME_COUNT];
    uint64_t lastRenderTime;
    uint64_t lastFrameTime;
    short framePointer;
public:
    GUIFPSCounter(GraphicsInterface* graphicsWrapper, Face *font, const std::string &text, const glm::lowp_uvec3 color) :
            GUITextBase(graphicsWrapper, font, text, color),
            previousFrameTimes{0}, lastRenderTime(0), lastFrameTime(0), framePointer(0) {
    }

    void renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram, uint32_t lodLevel) override;

    void updateFPS();
};


#endif //LIMONENGINE_GUIFPSCOUNTER_H
