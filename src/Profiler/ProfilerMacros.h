#ifndef LIMONENGINE_PROFILERMACROS_H
#define LIMONENGINE_PROFILERMACROS_H

#include "tracy/Tracy.hpp"
#include "../Profiler/ProfilerState.h"

#ifdef TRACY_ENABLE
#define PROFILE_OVERALL(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::isTracingEnabled())
#define PROFILE_SIMULATION(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::isTracingEnabled() && (ProfilerState::isTracingSimulation() || ProfilerState::isEmbeddedServerEnabled()))
#define PROFILE_VISIBILITY(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::isTracingEnabled() && (ProfilerState::isTracingVisibility() || ProfilerState::isEmbeddedServerEnabled()))
#define PROFILE_RENDERING(name) ZoneNamedN(___tracy_scoped_zone, name, ProfilerState::isTracingEnabled() && (ProfilerState::isTracingRendering() || ProfilerState::isEmbeddedServerEnabled()))
#define PROFILE_FRAME() FrameMark
#define PLOT_VISIBILITY(name, value) do { if (ProfilerState::isTracingVisibility()) TracyPlot(name, (int64_t)(value)); } while(0)
#else
#define PROFILE_OVERALL(name)
#define PROFILE_SIMULATION(name)
#define PROFILE_VISIBILITY(name)
#define PROFILE_RENDERING(name)
#define PROFILE_FRAME()
#define PLOT_VISIBILITY(name, value)
#endif

#endif //LIMONENGINE_PROFILERMACROS_H
