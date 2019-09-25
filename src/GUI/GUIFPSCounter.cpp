//
// Created by Engin Manap on 9.04.2016.
//

#include "GUIFPSCounter.h"


void GUIFPSCounter::updateFPS() {
    currentTime = SDL_GetTicks();//this uses real time, because this needs real data, not game time
    Uint32 newFrameTime = currentTime - lastRenderTime;
    lastFrameTime += newFrameTime - previousFrameTimes[framePointer];
    previousFrameTimes[framePointer] = newFrameTime;
    framePointer = (framePointer + 1) % 100;
    lastRenderTime = currentTime;

    int normalizedFrameRate = (unsigned int) (1000.0f / (lastFrameTime / 100.0f));
    if(currentTime - lastUpdateTime > 250) {//4 update per second max
        text = std::to_string(normalizedFrameRate);
        lastUpdateTime = currentTime;
    }
}

void GUIFPSCounter::renderWithProgram(std::shared_ptr<GraphicsProgram> renderProgram) {
    updateFPS();
    GUITextBase::renderWithProgram(renderProgram);
}
