#pragma once
#include <string>
#include <cstdint>

enum class FrameFilter { All, GameTick, RenderOnly };

struct ProfileEvent {
    std::string Name;
    double StartTime; // ms relative to frame start
    double EndTime;   // ms relative to frame start
    uint32_t Depth;
};
