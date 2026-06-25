#include "ProfilerUI.h"
#include <imgui.h>
#include <set>
#include <cstdlib>
#include "MultiLinePlot.h"
#include "../Profiler/ProfilerState.h"
#include "../Profiler/ProfilerSystem.h"
#include "limonAPI/Options.h"
#include "Utils/HashUtil.h"
#include "FlameGraph.h"

namespace ProfilerUI {
    void DrawProfilerUI(ProfilerSystem* profilerSystem) {
#ifdef TRACY_ENABLE
        static FlameGraph profilerGraph;
        static std::vector<ProfileEvent> capturedFlameData;

        // Master tracing toggle — drives zone activation via ProfilerMacros.
        ImGui::Checkbox("Enable Tracing", &ProfilerState::traceOverallFrameTime);
        if (ProfilerState::traceOverallFrameTime) {
            ImGui::Indent();
            ImGui::Checkbox("Trace Simulation",      &ProfilerState::traceSimulation);
            ImGui::Checkbox("Trace Visibility",       &ProfilerState::traceVisibility);
            ImGui::Checkbox("Trace Rendering (CPU)", &ProfilerState::traceRendering);
            ImGui::Checkbox("Trace Rendering (GPU)", &ProfilerState::traceGpuRendering);
            ImGui::Unindent();
        }

        ImGui::NewLine();

        if (!profilerSystem->getTracingServerOption()) {
            std::cerr << "Tracing Server Option returned null, but tracy is enabled, this should never happen." << std::endl;
        }
        bool serverEnabled = profilerSystem->getTracingServerOption()->getOrDefault(true);
        if (ImGui::Checkbox("Enable Embedded Server", &serverEnabled)) {
            profilerSystem->getTracingServerOption()->set(serverEnabled);
        }

        // Flame graph is active whenever the embedded server is on.
        ProfilerState::showFlameGraph = serverEnabled;

        if (serverEnabled) {

            // ── Hierarchical stats ────────────────────────────────────────
            if (ImGui::TreeNodeEx("Frame Time", ImGuiTreeNodeFlags_DefaultOpen)) {

                if (ProfilerState::traceOverallFrameTime) {
                    const auto& frame_times = profilerSystem->GetFrameTimeHistory();
                    if (!frame_times.empty()) {
                        ImGui::Text("Frame -> Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                            profilerSystem->GetAverageFrameTime(),
                            profilerSystem->GetMinFrameTime(),
                            profilerSystem->GetMaxFrameTime(),
                            profilerSystem->GetPercentileFrameTime(0.01f));
                    } else {
                        ImGui::Text("Collecting frame data...");
                    }
                }

                if (ProfilerState::traceSimulation || ProfilerState::traceVisibility || ProfilerState::traceRendering) {
                    if (ImGui::TreeNodeEx("CPU", ImGuiTreeNodeFlags_DefaultOpen)) {

                        if (ProfilerState::traceSimulation) {
                            if (!profilerSystem->GetZoneTimeHistory("World::play").empty()) {
                                ImGui::Text("Simulation -> Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                    profilerSystem->GetZoneAverageFrameTime("World::play"),
                                    profilerSystem->GetZoneMinFrameTime("World::play"),
                                    profilerSystem->GetZoneMaxFrameTime("World::play"),
                                    profilerSystem->GetZonePercentileFrameTime("World::play", 0.01f));
                            } else {
                                ImGui::Text("Collecting simulation data...");
                            }
                        }

                        if (ProfilerState::traceVisibility) {
                            if (!profilerSystem->GetZoneTimeHistory("VisibilityManager::update").empty()) {
                                ImGui::Text("Visibility -> Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                    profilerSystem->GetZoneAverageFrameTime("VisibilityManager::update"),
                                    profilerSystem->GetZoneMinFrameTime("VisibilityManager::update"),
                                    profilerSystem->GetZoneMaxFrameTime("VisibilityManager::update"),
                                    profilerSystem->GetZonePercentileFrameTime("VisibilityManager::update", 0.01f));

                                const auto threadNames = profilerSystem->GetZoneThreadNames("fillVisibleObjectPerCamera");
                                for (const auto& threadName : threadNames) {
                                    const std::string key = "fillVisibleObjectPerCamera::" + threadName;
                                    if (!profilerSystem->GetZoneTimeHistory(key).empty()) {
                                        ImGui::Text("  [%s] -> Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                            threadName.c_str(),
                                            profilerSystem->GetZoneAverageFrameTime(key),
                                            profilerSystem->GetZoneMinFrameTime(key),
                                            profilerSystem->GetZoneMaxFrameTime(key),
                                            profilerSystem->GetZonePercentileFrameTime(key, 0.01f));
                                    }
                                }
                            } else {
                                ImGui::Text("Collecting visibility data...");
                            }
                        }

                        if (ProfilerState::traceRendering) {
                            if (!profilerSystem->GetZoneTimeHistory("Render").empty()) {
                                ImGui::Text("Render -> Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                    profilerSystem->GetZoneAverageFrameTime("Render"),
                                    profilerSystem->GetZoneMinFrameTime("Render"),
                                    profilerSystem->GetZoneMaxFrameTime("Render"),
                                    profilerSystem->GetZonePercentileFrameTime("Render", 0.01f));
                            } else {
                                ImGui::Text("Collecting render data...");
                            }
                        }

                        ImGui::TreePop();
                    }
                }

                if (ProfilerState::traceGpuRendering) {
                    const auto gpuZoneNames = profilerSystem->GetGpuZoneNames();
                    if (gpuZoneNames.empty()) {
                        ImGui::Text("Collecting GPU render data...");
                    } else if (ImGui::TreeNodeEx("GPU", ImGuiTreeNodeFlags_DefaultOpen)) {
                        for (const auto& stageName : gpuZoneNames) {
                            const std::string key = "GPU::" + stageName;
                            if (!profilerSystem->GetZoneTimeHistory(key).empty()) {
                                ImGui::Text("[%s] -> Avg: %.3f ms  Min: %.3f ms  Max: %.3f ms  1%% Low: %.3f ms",
                                    stageName.c_str(),
                                    profilerSystem->GetZoneAverageFrameTime(key),
                                    profilerSystem->GetZoneMinFrameTime(key),
                                    profilerSystem->GetZoneMaxFrameTime(key),
                                    profilerSystem->GetZonePercentileFrameTime(key, 0.01f));
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                ImGui::TreePop();
            }

            // ── Plots ─────────────────────────────────────────────────────
            if (ProfilerState::traceOverallFrameTime) {
                const auto& frame_times = profilerSystem->GetFrameTimeHistory();
                if (!frame_times.empty()) {
                    const float frameMax = profilerSystem->GetMaxFrameTime() * 1.2f;
                    ImGui::PlotLines("Frame Times (ms)", frame_times.data(), static_cast<int>(frame_times.size()), 0, nullptr, 0.0f, frameMax, ImVec2(0, 80));
                }
            }
            if (ProfilerState::traceRendering) {
                const auto& render_times = profilerSystem->GetZoneTimeHistory("Render");
                if (!render_times.empty()) {
                    const float renderMax = profilerSystem->GetZoneMaxFrameTime("Render") * 1.2f;
                    ImGui::PlotLines("Render Times (ms)", render_times.data(), static_cast<int>(render_times.size()), 0, nullptr, 0.0f, renderMax, ImVec2(0, 80));
                }
            }
            if (ProfilerState::traceVisibility) {
                static const char* visStatNames[] = {
                    "Frustum Culled", "LOD Skipped", "Occluders", "Occluded", "Total Visible"
                };
                std::vector<std::string> allPlotNames = profilerSystem->GetUserPlotNames();
                std::set<std::string> cameraNameSet;
                for (const std::string& plotName : allPlotNames) {
                    size_t sep = plotName.find("::");
                    if (sep != std::string::npos) {
                        cameraNameSet.insert(plotName.substr(sep + 2));
                    }
                }
                // We know name has frustum size, we should order by it for UX
                auto parseFrustumWidth = [](const std::string& cameraId) -> float {
                    size_t openBracket = cameraId.find('[');
                    if (openBracket == std::string::npos) return 0.0f;
                    const char* start = cameraId.c_str() + openBracket + 1;
                    char* end = nullptr;
                    float value = std::strtof(start, &end);
                    return (end > start) ? value : 0.0f;
                };
                std::vector<std::string> cameraNames(cameraNameSet.begin(), cameraNameSet.end());
                std::sort(cameraNames.begin(), cameraNames.end(),
                    [&parseFrustumWidth](const std::string& a, const std::string& b) {
                        return parseFrustumWidth(a) < parseFrustumWidth(b);
                    });
                for (const std::string& cameraName : cameraNames) {
                    std::string displayName = cameraName;
                    size_t bracketPos = displayName.find('[');
                    if (bracketPos != std::string::npos) {
                        displayName = displayName.substr(0, bracketPos);
                    }
                    std::string treeLabel = displayName + "##" + cameraName;
                    if (ImGui::TreeNodeEx(treeLabel.c_str())) {
                        static const ImU32 seriesColors[] = {
                            IM_COL32(255, 100, 100, 255), // Frustum Culled - red
                            IM_COL32(255, 220,  50, 255), // LOD Skipped    - yellow
                            IM_COL32(100, 149, 237, 255), // Occluders      - cornflower blue
                            IM_COL32(180, 100, 220, 255), // Occluded       - purple
                            IM_COL32(100, 220, 100, 255), // Total Visible  - green
                        };
                        std::vector<MultiLineSeries> seriesList;
                        for (int s = 0; s < 5; ++s) {
                            std::string key = std::string(visStatNames[s]) + "::" + cameraName;
                            seriesList.push_back({
                                visStatNames[s],
                                profilerSystem->GetPlotHistory(key),
                                profilerSystem->GetPlotCurrentValue(key),
                                seriesColors[s]
                            });
                        }
                        DrawMultiLinePlot(seriesList);
                        ImGui::TreePop();
                    }
                }
            }

            // ── Flame Graph ───────────────────────────────────────────────
            {
                static bool frozen = false;
                static std::vector<std::string> capturedDebugLines;
                static FrameFilter frameFilter = FrameFilter::All;

                ImGui::Separator();
                ImGui::Text("Flame Graph");

                FrameFilter prevFilter = frameFilter;
                ImGui::Text("Frame:");
                ImGui::SameLine();
                if (ImGui::RadioButton("All",         frameFilter == FrameFilter::All))        frameFilter = FrameFilter::All;
                ImGui::SameLine();
                if (ImGui::RadioButton("Game Tick",   frameFilter == FrameFilter::GameTick))   frameFilter = FrameFilter::GameTick;
                ImGui::SameLine();
                if (ImGui::RadioButton("Render Only", frameFilter == FrameFilter::RenderOnly)) frameFilter = FrameFilter::RenderOnly;

                if (frameFilter != prevFilter) profilerGraph.ResetView();

                if (!frozen) {
                    capturedFlameData  = profilerSystem->GetLastFrameEvents(frameFilter);
                    capturedDebugLines = profilerSystem->GetGpuDebugInfo(frameFilter);
                }

                if (frozen) {
                    if (ImGui::Button("Resume")) frozen = false;
                } else {
                    if (ImGui::Button("Freeze")) frozen = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Reset View")) profilerGraph.ResetView();
                ImGui::SameLine();
                ImGui::TextDisabled("Right-drag: pan  |  Scroll: zoom");

                profilerGraph.Render(capturedFlameData);

                if (ImGui::CollapsingHeader("GPU Zone Filter Log")) {
                    for (const auto& line : capturedDebugLines) {
                        ImGui::TextUnformatted(line.c_str());
                    }
                }
            }
        }
#endif
    }
}
