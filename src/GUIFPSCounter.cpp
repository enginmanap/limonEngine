//
// Created by Engin Manap on 9.04.2016.
//

#include "GUIFPSCounter.h"

void GUIFPSCounter::render(Light *light) {
    currentTime = SDL_GetTicks();
    Uint32 newFrameTime = currentTime - lastRenderTime;
    lastFrameTime += newFrameTime - previousFrameTimes[framePointer];
    previousFrameTimes[framePointer] = newFrameTime;
    framePointer = (framePointer + 1) % 100;
    lastRenderTime = currentTime;

    int normalizedFrameRate = (unsigned int) (1000.0f / (lastFrameTime / 100.0f));
    this->text = std::to_string(normalizedFrameRate);
    GUIText::render(light);
}