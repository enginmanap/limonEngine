//
// Created by Engin Manap on 3.09.2026.
//

#ifndef LIMONENGINE_FRAMETIMETRACKER_H
#define LIMONENGINE_FRAMETIMETRACKER_H

#include <cstdint>
#include <vector>

#include "limonAPI/Options.h"

/**
 * Implementation of a Sliding window for frame durations. It uses wall time, as a base, and counts
 * frames, not how long a frame took.
 */
class FrameTimeTracker {
    static const size_t MAX_EXPECTED_FPS = 2000;
    static const long DEFAULT_WINDOW_MILLISECONDS = 500;
    static const long MINIMUM_WINDOW_MILLISECONDS = 50;
    static const long MAXIMUM_WINDOW_MILLISECONDS = 10000;

    std::vector<uint32_t> frameTimes;//microseconds ring buffer
    size_t oldestIndex = 0;
    size_t sampleCount = 0;
    uint64_t frameTimeSum = 0;//microseconds, kept in sync with samples in the ring
    uint64_t lastFrameTimestamp = 0;
    uint64_t performanceFrequency;
    uint64_t windowMicroseconds;

    void pushSample(uint32_t frameTimeMicroseconds);
    void dropOldest();

public:
    explicit FrameTimeTracker(OptionsUtil::Options *options);

    void tick();

    uint32_t getAverageFrameTimeMicroseconds() const;
    uint32_t getMinimumFrameTimeMicroseconds() const;
    uint32_t getMaximumFrameTimeMicroseconds() const;
    float getFramesPerSecond() const;

    size_t getSampleCount() const {
        return sampleCount;
    }

    uint64_t getWindowMicroseconds() const {
        return windowMicroseconds;
    }
};


#endif //LIMONENGINE_FRAMETIMETRACKER_H
