//
// Created by engin on 18.04.2019.
//

#include <nodeGraph/src/Connection.h>
#include <nodeGraph/src/Node.h>
#include "PipelineStageExtension.h"



void PipelineStageExtension::drawDetailPane(Node *node) {
    ImGui::Indent( 16.0f );
    if (ImGui::CollapsingHeader("Input Textures##PipelineStageExtension")) {
        ImGui::BeginChild("Input Textures##PipelineStageExtensionInputRegion", ImVec2(0, 120), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const Connection *connection:node->getInputConnections()) {
            if (inputTextureIndexes.find(connection) == inputTextureIndexes.end()) {
                inputTextureIndexes[connection] = 0;
            }
            ImGui::InputInt((connection->getName() + " texture Attachment Index").c_str(), &(inputTextureIndexes[connection]));
            if (inputTextureIndexes[connection] < 0) {
                inputTextureIndexes[connection] = 0;//Don't allow negative values
            }
        }
        ImGui::EndChild();
    }
    if (ImGui::CollapsingHeader("Output Textures##PipelineStageExtension")) {

        ImGui::BeginChild("Output Textures##PipelineStageExtensionOutputRegion", ImVec2(0, 120), true, ImGuiWindowFlags_HorizontalScrollbar);
        auto usedTextures = this->pipelineExtension->getUsedTextures();
        for (const Connection *connection:node->getOutputConnections()) {
            std::string currentTextureName = "None";
            if (outputTextures.find(connection) != outputTextures.end()) {
                currentTextureName = outputTextures[connection].first;
            }
            if (ImGui::BeginCombo(connection->getName().c_str(), currentTextureName.c_str())) {
                for (auto it = usedTextures.begin();
                     it != usedTextures.end(); it++) {
                    if (ImGui::Selectable(it->first.c_str())) {
                        outputTextures[connection] = std::make_pair(it->first, it->second);
                    }
                    if (currentTextureName == it->first) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
        }
        ImGui::EndChild();
    }
    if (ImGui::CollapsingHeader("Render Settings##PipelineStageExtension")) {
        //ImGui::Text("Cull Mode##FromNodeExtensionSetting");
        ImGui::Text("Cull Mode");//FIXME this is possibly a bug, but it doesn't allow adding escaped text
        if (ImGui::RadioButton("No Change##CullModeFromNodeExtension", cullMode == GLHelper::CullModes::NO_CHANGE)) { cullMode = GLHelper::CullModes::NO_CHANGE; }
        ImGui::SameLine();
        if (ImGui::RadioButton("None##CullModeFromNodeExtension", cullMode == GLHelper::CullModes::NONE)) { cullMode = GLHelper::CullModes::NONE; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Front##CullModeFromNodeExtension", cullMode == GLHelper::CullModes::FRONT)) { cullMode = GLHelper::CullModes::FRONT; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Back##CullModeFromNodeExtension", cullMode == GLHelper::CullModes::BACK)) { cullMode = GLHelper::CullModes::BACK; }

        ImGui::Checkbox("Blend", &blendEnabled);

        ImGui::Checkbox("Clear", &blendEnabled);
        if (ImGui::BeginCombo("Render Method##RenderMethodCombo", currentMethodName.c_str())) {
            for (size_t i = 0; i < PipelineExtension::renderMethodNames->size(); ++i) {
                const std::string &methodName = PipelineExtension::renderMethodNames[i];
                if (ImGui::Selectable(methodName.c_str())) {
                    currentMethodName = methodName;
                }
                if (currentMethodName == methodName) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }
    ImGui::Unindent( 16.0f );
}
