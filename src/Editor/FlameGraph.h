#pragma once
#include <vector>
#include "imgui.h"
#include "../Profiler/ProfilerEvent.h"

class FlameGraph {
public:
    double ScrollOffset = 0.0;
    float PixelsPerMs   = 40.0f;
    float RowHeight     = 20.0f;
    int   SelectedIdx   = -1;

    void Render(const std::vector<ProfileEvent>& events);
    void ResetView();

private:
    bool  userHasInteracted = false;
    float maxGraphHeight    = 0.0f;

    void HandleInput(ImVec2 pos, ImVec2 size);
    static ImU32 GetEventColor(const ProfileEvent& e);
};
