#include "ProfilerUI.h"
#include <imgui.h>
#include "../Profiler/ProfilerState.h"
#include "../Profiler/ProfilerSystem.h"
#include "limonAPI/Options.h"
#include "Utils/HashUtil.h"

namespace ProfilerUI {
    void DrawProfilerUI(ProfilerSystem* profilerSystem) {
#ifdef TRACY_ENABLE
        if (ImGui::CollapsingHeader("Profiler")) {
            ImGui::Checkbox("Trace Overall Frame Time", &ProfilerState::traceOverallFrameTime);
            ImGui::Checkbox("Trace Simulation", &ProfilerState::traceSimulation);
            ImGui::Checkbox("Trace Visibility", &ProfilerState::traceVisibility);
            ImGui::Checkbox("Trace Rendering", &ProfilerState::traceRendering);
            ImGui::NewLine();
            if (!profilerSystem->getTracingServerOption()) {
                std::cerr << "Tracing Server Option returned null, nut tracy is enabled, this should never happen." << std::endl;
            }
            bool serverEnabled = profilerSystem->getTracingServerOption()->getOrDefault(true);
            if (ImGui::Checkbox("Enable Embedded Server", &serverEnabled)) {
                profilerSystem->getTracingServerOption()->set(serverEnabled);
            }

            if (serverEnabled) {

                if (ProfilerState::traceOverallFrameTime) {
                    const auto& frame_times = profilerSystem->GetFrameTimeHistory();
                    if (!frame_times.empty()) {
                        ImGui::PlotLines("Frame Times (ms)", frame_times.data(), static_cast<int>(frame_times.size()), 0, nullptr, 0.0f, 30.0f, ImVec2(0, 80));
                        ImGui::Text("Overall Frame Time:");
                        ImGui::Text("  Average: %.3f ms", profilerSystem->GetAverageFrameTime());
                        ImGui::Text("  Min: %.3f ms", profilerSystem->GetMinFrameTime());
                        ImGui::Text("  Max: %.3f ms", profilerSystem->GetMaxFrameTime());
                        ImGui::Text("  1%% Low: %.3f ms", profilerSystem->GetPercentileFrameTime(0.01f));
                    } else {
                        ImGui::Text("Collecting overall frame data...");
                    }
                }

                if (ProfilerState::traceSimulation) {
                    const auto& sim_times = profilerSystem->GetZoneTimeHistory("World::play");
                    if (!sim_times.empty()) {
                        ImGui::PlotLines("Simulation Times (ms)", sim_times.data(), static_cast<int>(sim_times.size()), 0, nullptr, 0.0f, 30.0f, ImVec2(0, 80));
                        ImGui::Text("Simulation Time:");
                        ImGui::Text("  Average: %.3f ms", profilerSystem->GetZoneAverageFrameTime("World::play"));
                        ImGui::Text("  Min: %.3f ms", profilerSystem->GetZoneMinFrameTime("World::play"));
                        ImGui::Text("  Max: %.3f ms", profilerSystem->GetZoneMaxFrameTime("World::play"));
                        ImGui::Text("  1%% Low: %.3f ms", profilerSystem->GetZonePercentileFrameTime("World::play", 0.01f));
                    } else {
                        ImGui::Text("Collecting simulation data...");
                    }
                }

                if (ProfilerState::traceVisibility) {
                    const auto& vis_times = profilerSystem->GetZoneTimeHistory("VisibilityManager::update");
                    if (!vis_times.empty()) {
                        ImGui::PlotLines("Visibility Times (ms)", vis_times.data(), static_cast<int>(vis_times.size()), 0, nullptr, 0.0f, 30.0f, ImVec2(0, 80));
                        ImGui::Text("Visibility Time:");
                        ImGui::Text("  Average: %.3f ms", profilerSystem->GetZoneAverageFrameTime("VisibilityManager::update"));
                        ImGui::Text("  Min: %.3f ms", profilerSystem->GetZoneMinFrameTime("VisibilityManager::update"));
                        ImGui::Text("  Max: %.3f ms", profilerSystem->GetZoneMaxFrameTime("VisibilityManager::update"));
                        ImGui::Text("  1%% Low: %.3f ms", profilerSystem->GetZonePercentileFrameTime("VisibilityManager::update", 0.01f));
                    } else {
                        ImGui::Text("Collecting visibility data...");
                    }
                }
            }
        }
#endif
    }
}
