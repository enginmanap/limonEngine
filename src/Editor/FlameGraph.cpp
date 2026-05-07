#include "FlameGraph.h"
#include <algorithm>
#include <limits>

void FlameGraph::ResetView() {
    ScrollOffset      = 0.0;
    SelectedIdx       = -1;
    userHasInteracted = false;
    maxGraphHeight    = 0.0f;
    // PixelsPerMs is auto-computed on the next Render() call.
}

ImU32 FlameGraph::GetEventColor(const ProfileEvent& e) {
    size_t hash = std::hash<std::string>{}(e.Name);
    return IM_COL32(
        100 + (hash         % 100),
        120 + ((hash >>  8) % 100),
        150 + ((hash >> 16) % 100),
        255
    );
}

void FlameGraph::Render(const std::vector<ProfileEvent>& events) {
    if (events.empty()) {
        ImGui::Text("No flame graph data yet.");
        return;
    }

    uint32_t maxDepth = 0;
    for (const auto& e : events) maxDepth = std::max(maxDepth, e.Depth);
    float graphHeight = std::max((maxDepth + 2) * (RowHeight + 2), 80.0f);
    maxGraphHeight = std::max(maxGraphHeight, graphHeight);

    ImGui::SetNextWindowScroll(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("FlameGraphArea", ImVec2(0, maxGraphHeight), true,
        ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl      = ImGui::GetWindowDrawList();
    ImVec2 canvasPos    = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize   = ImGui::GetContentRegionAvail();

    if (!userHasInteracted && canvasSize.x > 0) {
        float maxEnd = 0;
        for (const auto& e : events) maxEnd = std::max(maxEnd, (float)e.EndTime);
        if (maxEnd > 0) {
            PixelsPerMs  = canvasSize.x / maxEnd;
            ScrollOffset = 0.0;
        }
    }

    HandleInput(canvasPos, canvasSize);

    // First pass: find the single best hover candidate using unclamped coordinates.
    // Prefer highest depth (most specific zone); break ties by shortest duration.
    int hoveredIdx = -1;
    {
        ImVec2 mouse     = ImGui::GetMousePos();
        bool   inCanvas  = ImGui::IsWindowHovered() &&
                           mouse.x >= canvasPos.x &&
                           mouse.x <  canvasPos.x + canvasSize.x &&
                           mouse.y >= canvasPos.y &&
                           mouse.y <  canvasPos.y + maxGraphHeight;

        if (inCanvas) {
            uint32_t bestDepth    = 0;
            double   bestDuration = std::numeric_limits<double>::max();

            for (int i = 0; i < (int)events.size(); ++i) {
                const auto& e = events[i];
                float sx1 = canvasPos.x + (float)((e.StartTime - ScrollOffset) * PixelsPerMs);
                float sx2 = canvasPos.x + (float)((e.EndTime   - ScrollOffset) * PixelsPerMs);
                float sy1 = canvasPos.y + e.Depth * (RowHeight + 2);
                float sy2 = sy1 + RowHeight;

                if (sx2 < canvasPos.x || sx1 > canvasPos.x + canvasSize.x) continue;
                if (mouse.x < sx1 || mouse.x >= sx2) continue;
                if (mouse.y < sy1 || mouse.y >= sy2) continue;

                double dur = e.EndTime - e.StartTime;
                if (hoveredIdx == -1 ||
                    e.Depth > bestDepth ||
                    (e.Depth == bestDepth && dur < bestDuration)) {
                    hoveredIdx    = i;
                    bestDepth     = e.Depth;
                    bestDuration  = dur;
                }
            }
        }
    }

    // Second pass: draw all events, highlighting only the winner.
    for (int i = 0; i < (int)events.size(); ++i) {
        const auto& e = events[i];

        float x1 = (float)((e.StartTime - ScrollOffset) * PixelsPerMs);
        float x2 = (float)((e.EndTime   - ScrollOffset) * PixelsPerMs);
        float y1 = e.Depth * (RowHeight + 2);

        if (x2 < 0.0f || x1 > canvasSize.x) continue;

        ImVec2 bmin = ImVec2(canvasPos.x + std::max(0.0f, x1), canvasPos.y + y1);
        ImVec2 bmax = ImVec2(canvasPos.x + std::min(canvasSize.x, x2), canvasPos.y + y1 + RowHeight);

        ImU32 color = GetEventColor(e);

        if (SelectedIdx == i) {
            color = IM_COL32(255, 255, 0, 255);
        } else if (hoveredIdx == i) {
            color = IM_COL32(255, 255, 255, 200);
            ImGui::SetTooltip("%s\nStart: %.3f ms\nDuration: %.3f ms",
                e.Name.c_str(), e.StartTime, e.EndTime - e.StartTime);
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) SelectedIdx = i;
        }

        dl->AddRectFilled(bmin, bmax, color);
        dl->AddRect(bmin, bmax, IM_COL32(0, 0, 0, 80));

        if (x2 - x1 > 20.0f) {
            dl->PushClipRect(bmin, bmax, true);
            dl->AddText(ImVec2(bmin.x + 3, bmin.y + 3), IM_COL32(0, 0, 0, 255), e.Name.c_str());
            dl->PopClipRect();
        }
    }

    ImGui::Dummy(ImVec2(0.0f, maxGraphHeight + 1.0f));
    ImGui::EndChild();
}

void FlameGraph::HandleInput(ImVec2 pos, ImVec2 size) {
    ImGuiIO& io    = ImGui::GetIO();
    bool hovered   = ImGui::IsWindowHovered();

    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ScrollOffset      -= io.MouseDelta.x / PixelsPerMs;
        userHasInteracted  = true;
    }

    if (hovered && io.MouseWheel != 0.0f) {
        double mouseTime  = (io.MousePos.x - pos.x) / PixelsPerMs + ScrollOffset;
        float factor      = (io.MouseWheel > 0.0f) ? 1.15f : 0.87f;
        PixelsPerMs       = std::max(1.0f, std::min(PixelsPerMs * factor, 10000.0f));
        ScrollOffset      = mouseTime - (io.MousePos.x - pos.x) / PixelsPerMs;
        userHasInteracted = true;
    }
}
