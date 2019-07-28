//
// Created by engin on 23.04.2019.
//

#include <imgui/imgui.h>
#include <string>


#include "IterationExtension.h"
#include <nodeGraph/src/Node.h>

void IterationExtension::drawDetailPane(Node *node) {
    const std::string iterateOverList[] = { "Directional Lights", "Point lights" };
    if (ImGui::BeginCombo("Iterate Over", currentIterateOver.c_str())) {

        for (int i = 0; i < 2; ++i) {
            if (ImGui::Selectable(iterateOverList[i].c_str())) {
                currentIterateOver = iterateOverList[i];
                std::string baseName = node->getName().substr(0, node->getName().find("("));
                node->setName(baseName + " (" + currentIterateOver + ")");
            }
            if(currentIterateOver == iterateOverList[i]) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    const std::string outputTypes[] = {"Texture" , "Texture array", "Cubemap", "Cubemap array"};
    std::string currentOutputType = node->getOutputConnections()[0]->getDataType();
    if (ImGui::BeginCombo("Output Type", currentOutputType.c_str())) {

        for (int i = 0; i < 4; ++i) {
            if (ImGui::Selectable(outputTypes[i].c_str())) {
                currentOutputType = outputTypes[i];
                std::string name = node->getOutputConnections()[0]->getName();
                node->removeAllOutputs();
                ConnectionDesc desc;
                desc.name = name;
                desc.type = currentOutputType;
                node->addOutput(desc);
            }
            if(currentOutputType == outputTypes[i]) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}
