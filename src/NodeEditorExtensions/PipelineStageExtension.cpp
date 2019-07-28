//
// Created by engin on 18.04.2019.
//

#include <nodeGraph/src/Connection.h>
#include <nodeGraph/src/Node.h>
#include "PipelineStageExtension.h"



void PipelineStageExtension::drawDetailPane(Node *node) {

    for (const Connection* connection:node->getInputConnections()) {
        if(inputTextureIndexes.find(connection) == inputTextureIndexes.end()) {
            inputTextureIndexes[connection] = 0;
        }
        ImGui::InputInt((connection->getName() + " texture Attachment Index").c_str(), &(inputTextureIndexes[0]));
    }

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

    ImGui::Text("Cull Mode##FromNodeExtensionSetting");
    if(ImGui::RadioButton("No Change##CullModeFromNodeExtension", cullmode == GLHelper::CullModes::NO_CHANGE)) { cullmode = GLHelper::CullModes::NO_CHANGE;}
    if(ImGui::RadioButton("None##CullModeFromNodeExtension", cullmode == GLHelper::CullModes::NONE)) { cullmode = GLHelper::CullModes::NONE;}
    if(ImGui::RadioButton("Front##CullModeFromNodeExtension", cullmode == GLHelper::CullModes::FRONT)) { cullmode = GLHelper::CullModes::FRONT;}
    if(ImGui::RadioButton("Back##CullModeFromNodeExtension", cullmode == GLHelper::CullModes::BACK)) { cullmode = GLHelper::CullModes::BACK;}

    ImGui::Checkbox("Blend", &blendEnabled);

    ImGui::Checkbox("Clear", &blendEnabled);

}
