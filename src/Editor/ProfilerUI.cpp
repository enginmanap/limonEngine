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
            ImGui::Checkbox("Trace Rendering (CPU)", &ProfilerState::traceRendering);
            ImGui::Checkbox("Trace Rendering (GPU)", &ProfilerState::traceGpuRendering);
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

                        const auto threadNames = profilerSystem->GetZoneThreadNames("fillVisibleObjectPerCamera");
                        if (!threadNames.empty()) {
                            ImGui::Separator();
                            ImGui::Text("Per-camera breakdown:");
                            for (const auto& threadName : threadNames) {
                                const std::string key = "fillVisibleObjectPerCamera::" + threadName;
                                const auto& camTimes = profilerSystem->GetZoneTimeHistory(key);
                                if (!camTimes.empty()) {
                                    ImGui::Text("  [%s]", threadName.c_str());
                                    const std::string plotLabel = "##vis_" + threadName;
                                    ImGui::PlotLines(plotLabel.c_str(), camTimes.data(), static_cast<int>(camTimes.size()), 0, nullptr, 0.0f, 30.0f, ImVec2(0, 40));
                                    ImGui::Text("    Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                        profilerSystem->GetZoneAverageFrameTime(key),
                                        profilerSystem->GetZoneMinFrameTime(key),
                                        profilerSystem->GetZoneMaxFrameTime(key),
                                        profilerSystem->GetZonePercentileFrameTime(key, 0.01f));
                                }
                            }
                        }
                    } else {
                        ImGui::Text("Collecting visibility data...");
                    }
                }

                if (ProfilerState::traceRendering) {
                    const auto& render_times = profilerSystem->GetZoneTimeHistory("Render");
                    if (!render_times.empty()) {
                        ImGui::PlotLines("Render Times (ms)", render_times.data(), static_cast<int>(render_times.size()), 0, nullptr, 0.0f, 30.0f, ImVec2(0, 80));
                        ImGui::Text("Render Time (CPU):");
                        ImGui::Text("  Average: %.3f ms", profilerSystem->GetZoneAverageFrameTime("Render"));
                        ImGui::Text("  Min: %.3f ms", profilerSystem->GetZoneMinFrameTime("Render"));
                        ImGui::Text("  Max: %.3f ms", profilerSystem->GetZoneMaxFrameTime("Render"));
                        ImGui::Text("  1%% Low: %.3f ms", profilerSystem->GetZonePercentileFrameTime("Render", 0.01f));
                    } else {
                        ImGui::Text("Collecting render data...");
                    }
                }

                if (ProfilerState::traceGpuRendering) {
                    const auto gpuZoneNames = profilerSystem->GetGpuZoneNames();
                    if (gpuZoneNames.empty()) {
                        ImGui::Text("Collecting GPU render data...");
                    } else {
                        ImGui::Text("Render Time (GPU) per stage:");
                        for (const auto& stageName : gpuZoneNames) {
                            const std::string key = "GPU::" + stageName;
                            const auto& gpuTimes = profilerSystem->GetZoneTimeHistory(key);
                            if (!gpuTimes.empty()) {
                                ImGui::Text("  [%s]", stageName.c_str());
                                const std::string plotLabel = "##gpu_" + stageName;
                                ImGui::PlotLines(plotLabel.c_str(), gpuTimes.data(), static_cast<int>(gpuTimes.size()), 0, nullptr, 0.0f, 30.0f, ImVec2(0, 40));
                                ImGui::Text("    Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                    profilerSystem->GetZoneAverageFrameTime(key),
                                    profilerSystem->GetZoneMinFrameTime(key),
                                    profilerSystem->GetZoneMaxFrameTime(key),
                                    profilerSystem->GetZonePercentileFrameTime(key, 0.01f));
                            }
                        }
                    }
                }
            }
        }
#endif
    }
}
