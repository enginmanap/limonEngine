#ifndef LIMONENGINE_MULTILINEPLOT_H
#define LIMONENGINE_MULTILINEPLOT_H

#include <vector>
#include <algorithm>
#include <imgui.h>

struct MultiLineSeries {
    const char*        label;
    std::vector<float> history;       // by value — max 100 entries, copy is cheap
    float              currentValue;
    ImU32              color;
};

// Draws all series onto a single canvas with a shared Y axis, then renders a compact
// color-coded legend below. Series with empty history are silently skipped, so callers
// can always pass the full set and let availability drive what appears.
inline void DrawMultiLinePlot(const std::vector<MultiLineSeries>& series, float canvasHeight = 120.0f) {
    float globalMax = 1.0f;
    bool anyData = false;
    for (const MultiLineSeries& s : series) {
        if (s.history.empty()) continue;
        anyData = true;
        float seriesMax = *std::max_element(s.history.begin(), s.history.end());
        globalMax = std::max(globalMax, seriesMax);
    }
    if (!anyData) return;
    globalMax *= 1.2f;

    ImVec2 canvasMin = ImGui::GetCursorScreenPos();
    float canvasWidth = ImGui::GetContentRegionAvail().x;
    ImVec2 canvasMax(canvasMin.x + canvasWidth, canvasMin.y + canvasHeight);
    ImGui::Dummy(ImVec2(canvasWidth, canvasHeight));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(canvasMin, canvasMax, IM_COL32(30, 30, 30, 200));
    dl->AddRect(canvasMin, canvasMax, IM_COL32(100, 100, 100, 255));
    dl->PushClipRect(canvasMin, canvasMax, true);

    for (const MultiLineSeries& s : series) {
        if (s.history.empty()) continue;
        int count = static_cast<int>(s.history.size());
        std::vector<ImVec2> points(count);
        for (int i = 0; i < count; ++i) {
            float t = (count > 1) ? static_cast<float>(i) / (count - 1) : 0.5f;
            float x = canvasMin.x + t * canvasWidth;
            float y = canvasMax.y - (s.history[i] / globalMax) * canvasHeight;
            points[i] = ImVec2(x, y);
        }
        dl->AddPolyline(points.data(), count, s.color, 0, 1.5f);
    }
    dl->PopClipRect();

    int id = 0;
    for (const MultiLineSeries& s : series) {
        if (s.history.empty()) { ++id; continue; }
        ImGui::PushID(id++);
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(s.color);
        ImGui::ColorButton("##", col,
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_NoPicker,
            ImVec2(10, 10));
        ImGui::PopID();
        ImGui::SameLine(0, 4);
        ImGui::Text("%s: %.0f", s.label, s.currentValue);
    }
}

#endif // LIMONENGINE_MULTILINEPLOT_H
