#pragma once

#include "tracy/Tracy.hpp"
#include "../Profiler/ProfilerState.h"

#ifdef TRACY_ENABLE
#define PROFILE_OVERALL(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceOverallFrameTime || ProfilerState::showFlameGraph)
#define PROFILE_SIMULATION(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceSimulation || ProfilerState::showFlameGraph)
#define PROFILE_VISIBILITY(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceVisibility || ProfilerState::showFlameGraph)
#define PROFILE_RENDERING(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceRendering || ProfilerState::showFlameGraph)
#define PROFILE_FRAME() FrameMark
#define PLOT_VISIBILITY(name, value) do { if (ProfilerState::traceVisibility) TracyPlot(name, (int64_t)(value)); } while(0)
#else
#define PROFILE_OVERALL(name)
#define PROFILE_SIMULATION(name)
#define PROFILE_VISIBILITY(name)
#define PROFILE_RENDERING(name)
#define PROFILE_FRAME()
#define PLOT_VISIBILITY(name, value)
#endif
