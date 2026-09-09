//
// Created by Engin Manap on 3.09.2026.
//

#include "FrameTimeTracker.h"

#include <iostream>

#include "SDL2Helper.h"
#include "limonAPI/util/HashUtil.h"

FrameTimeTracker::FrameTimeTracker(OptionsUtil::Options *options) {
    long windowMilliseconds = options->getOption<long>(HASH("debug_fpsWindowMs")).getOrDefault(DEFAULT_WINDOW_MILLISECONDS);
    if (windowMilliseconds < MINIMUM_WINDOW_MILLISECONDS || windowMilliseconds > MAXIMUM_WINDOW_MILLISECONDS) {
        std::cerr << "debug_fpsWindowMs " << windowMilliseconds << " is out of range ["
                  << MINIMUM_WINDOW_MILLISECONDS << ", " << MAXIMUM_WINDOW_MILLISECONDS << "], using "
                  << DEFAULT_WINDOW_MILLISECONDS << " instead." << std::endl;
        windowMilliseconds = DEFAULT_WINDOW_MILLISECONDS;
    }
    windowMicroseconds = (uint64_t)windowMilliseconds * 1000;
    performanceFrequency = SDL2Helper::getPerformanceFrequency();
    frameTimes.resize(((size_t)windowMilliseconds * MAX_EXPECTED_FPS) / 1000 + 1, 0);
}

void FrameTimeTracker::dropOldest() {
    frameTimeSum -= frameTimes[oldestIndex];
    oldestIndex = (oldestIndex + 1) % frameTimes.size();
    --sampleCount;
}

void FrameTimeTracker::pushSample(uint32_t frameTimeMicroseconds) {
    if (sampleCount == frameTimes.size()) {
        dropOldest();
    }
    frameTimes[(oldestIndex + sampleCount) % frameTimes.size()] = frameTimeMicroseconds;
    frameTimeSum += frameTimeMicroseconds;
    ++sampleCount;
}

void FrameTimeTracker::tick() {
    uint64_t now = SDL2Helper::getPerformanceCounter();
    if (lastFrameTimestamp == 0) {
        // if there is no last frame, then sampling would get us whole process time, not a real value
        lastFrameTimestamp = now;
        return;
    }
    uint64_t elapsedMicroseconds = ((now - lastFrameTimestamp) * 1000000) / performanceFrequency;
    lastFrameTimestamp = now;
    pushSample((uint32_t)elapsedMicroseconds);

    while (sampleCount > 1 && frameTimeSum > windowMicroseconds) {
        dropOldest();
    }
}

uint32_t FrameTimeTracker::getAverageFrameTimeMicroseconds() const {
    if (sampleCount == 0) {
        return 0;
    }
    return (uint32_t)(frameTimeSum / sampleCount);
}

uint32_t FrameTimeTracker::getMinimumFrameTimeMicroseconds() const {
    if (sampleCount == 0) {
        return 0;
    }
    uint32_t minimum = frameTimes[oldestIndex];
    for (size_t i = 1; i < sampleCount; ++i) {
        uint32_t sample = frameTimes[(oldestIndex + i) % frameTimes.size()];
        if (sample < minimum) {
            minimum = sample;
        }
    }
    return minimum;
}

uint32_t FrameTimeTracker::getMaximumFrameTimeMicroseconds() const {
    if (sampleCount == 0) {
        return 0;
    }
    uint32_t maximum = frameTimes[oldestIndex];
    for (size_t i = 1; i < sampleCount; ++i) {
        uint32_t sample = frameTimes[(oldestIndex + i) % frameTimes.size()];
        if (sample > maximum) {
            maximum = sample;
        }
    }
    return maximum;
}

float FrameTimeTracker::getFramesPerSecond() const {
    uint32_t averageFrameTime = getAverageFrameTimeMicroseconds();
    if (averageFrameTime == 0) {
        return 0.0f;
    }
    return 1000000.0f / (float)averageFrameTime;
}
