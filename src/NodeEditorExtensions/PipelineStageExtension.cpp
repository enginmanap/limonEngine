//
// Created by engin on 18.04.2019.
//

#include <nodeGraph/src/Connection.h>
#include <nodeGraph/src/Node.h>
#include "PipelineStageExtension.h"



void PipelineStageExtension::drawDetailPane(Node *node) {
    auto usedTextures = this->pipelineExtension->getUsedTextures();

    for (const Connection* connection:node->getOutputConnections()) {
        std::string currentTextureName = "None";
        if(outputTextures.find(connection) != outputTextures.end()) {
            currentTextureName = outputTextures[connection].first;
        }
        if (ImGui::BeginCombo(connection->getName().c_str(), currentTextureName.c_str())) {
            for (auto it = usedTextures.begin();
                 it != usedTextures.end(); it++) {
                if (ImGui::Selectable(it->first.c_str())) {
                    outputTextures[connection] = std::make_pair(it->first, it->second);
                }
                if(currentTextureName == it->first) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

}
