#pragma once

#include "tracy/Tracy.hpp"
#include "../Profiler/ProfilerState.h"

#ifdef TRACY_ENABLE
#define PROFILE_OVERALL(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceOverallFrameTime)
#define PROFILE_SIMULATION(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceSimulation)
#define PROFILE_VISIBILITY(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceVisibility)
#define PROFILE_RENDERING(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::traceRendering)
#define PROFILE_FRAME() FrameMark
#else
#define PROFILE_OVERALL(name)
#define PROFILE_SIMULATION(name)
#define PROFILE_VISIBILITY(name)
#define PROFILE_RENDERING(name)
#define PROFILE_FRAME()
#endif
