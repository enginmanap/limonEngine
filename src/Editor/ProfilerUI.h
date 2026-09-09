#ifndef LIMONENGINE_PROFILERUI_H
#define LIMONENGINE_PROFILERUI_H

class ProfilerSystem;
class FrameTimeTracker;
class GraphicsInterface;

namespace ProfilerUI {
    void DrawProfilerUI(ProfilerSystem* profilerSystem);

    void DrawFrameStatsUI(const FrameTimeTracker* frameTimeTracker, const GraphicsInterface* graphicsWrapper);

    // If editor mode = null, that means we are not in editor mode, so we should not take mouse input.
    // Otherwise we can get the input
    void DrawProfilerWindow(ProfilerSystem* profilerSystem, const FrameTimeTracker* frameTimeTracker,
                            const GraphicsInterface* graphicsWrapper, bool* editorMode);
}

#endif //LIMONENGINE_PROFILERUI_H
