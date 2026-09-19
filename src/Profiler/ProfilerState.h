#ifndef LIMONENGINE_PROFILERSTATE_H
#define LIMONENGINE_PROFILERSTATE_H

#include "limonAPI/Options.h"

namespace ProfilerState {
    //bound by ProfilerSystem after options load, zones before that see the defaults below
    inline OptionsUtil::Options::Option<bool> tracingEnabledOption;
    inline OptionsUtil::Options::Option<bool> traceSimulationOption;
    inline OptionsUtil::Options::Option<bool> traceVisibilityOption;
    inline OptionsUtil::Options::Option<bool> traceRenderingOption;
    inline OptionsUtil::Options::Option<bool> traceGpuRenderingOption;
    inline OptionsUtil::Options::Option<bool> enableServerOption;

    inline bool isTracingEnabled() { return tracingEnabledOption.getOrDefault(true); }
    //categories are gated by the master switch here, so no caller can trace while it is off
    inline bool isTracingSimulation() { return isTracingEnabled() && traceSimulationOption.getOrDefault(false); }
    inline bool isTracingVisibility() { return isTracingEnabled() && traceVisibilityOption.getOrDefault(false); }
    inline bool isTracingRendering() { return isTracingEnabled() && traceRenderingOption.getOrDefault(false); }
    inline bool isTracingGpuRendering() { return isTracingEnabled() && traceGpuRenderingOption.getOrDefault(false); }
    inline bool isEmbeddedServerEnabled() { return enableServerOption.getOrDefault(true); }//flame graph needs every zone while the server runs
}

#endif //LIMONENGINE_PROFILERSTATE_H
