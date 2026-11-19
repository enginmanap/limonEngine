//
// Created by engin on 18.11.2026.
//

#include "StageCoverage.h"
#include <algorithm>
#include "ImGui/imgui.h"

void StageCoverage::collectStageLabels(const std::vector<uint64_t> &tagHashes, const std::vector<PipelineStageTags> &stages, std::vector<std::string> &labels) {
    for (const PipelineStageTags &stage : stages) {
        bool matches = false;
        for (const HashUtil::HashedString &stageTag : stage.tags) {
            if (std::find(tagHashes.begin(), tagHashes.end(), stageTag.hash) != tagHashes.end()) {
                matches = true;
                break;
            }
        }
        if (matches && std::find(labels.begin(), labels.end(), stage.name) == labels.end()) {
            labels.push_back(stage.name);
        }
    }
}

std::string StageCoverage::joinLabels(const std::vector<std::string> &labels) {
    std::string joined;
    for (const std::string &label : labels) {
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += label;
    }
    return joined;
}

void StageCoverage::drawLine(const std::vector<std::string> &labels, const std::string &tooltip) {
    if (labels.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped("In the current pipeline: renders nowhere");
        ImGui::PopStyleColor();
    } else {
        ImGui::TextWrapped("In the current pipeline: renders in %s", joinLabels(labels).c_str());
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        if (!tooltip.empty()) {
            ImGui::TextUnformatted(tooltip.c_str());
        }
        ImGui::TextDisabled("Stages pick by tags. This is only the pipeline in use now, another one may draw it differently.");
        ImGui::EndTooltip();
    }
}
