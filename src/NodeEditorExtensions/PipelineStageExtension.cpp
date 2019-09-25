//
// Created by engin on 18.04.2019.
//

#include <nodeGraph/src/Connection.h>
#include <nodeGraph/src/Node.h>
#include "PipelineStageExtension.h"
#include "../Graphics/Texture.h"
#include "../Graphics/GraphicsPipeline.h"

const std::string PipelineStageExtension::LIGHT_TYPES[] = {"NONE", "DIRECTIONAL", "POINT" };

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
                for (auto it = usedTextures.begin(); it != usedTextures.end(); it++) {
                    if (ImGui::Selectable(it->first.c_str())) {
                        outputTextures[connection] = std::make_pair(it->first, it->second);
                        //On output change, lets check if any of the outputs is multi layered
                        for(auto setOutput = outputTextures.begin(); setOutput != outputTextures.end(); setOutput++) {
                            this->anyOutputMultiLayered = false;
                            if(setOutput->second.second->getType() == GraphicsInterface::TextureTypes::T2D_ARRAY ||
                               setOutput->second.second->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP ||
                               setOutput->second.second->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY
                            ) {
                                this->anyOutputMultiLayered = true;
                                break;
                            }
                        }
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
        if (ImGui::RadioButton("No Change##CullModeFromNodeExtension", cullMode == GraphicsInterface::CullModes::NO_CHANGE)) { cullMode = GraphicsInterface::CullModes::NO_CHANGE; }
        ImGui::SameLine();
        if (ImGui::RadioButton("None##CullModeFromNodeExtension", cullMode == GraphicsInterface::CullModes::NONE)) { cullMode = GraphicsInterface::CullModes::NONE; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Front##CullModeFromNodeExtension", cullMode == GraphicsInterface::CullModes::FRONT)) { cullMode = GraphicsInterface::CullModes::FRONT; }
        ImGui::SameLine();
        if (ImGui::RadioButton("Back##CullModeFromNodeExtension", cullMode == GraphicsInterface::CullModes::BACK)) { cullMode = GraphicsInterface::CullModes::BACK; }

        ImGui::Checkbox("Blend", &blendEnabled);

        ImGui::Checkbox("Clear", &blendEnabled);
        if (ImGui::BeginCombo("Render Method##RenderMethodCombo", currentMethodName.c_str())) {
            static const std::vector<std::string> &methodNames = GraphicsPipeline::getRenderMethodNames();
            for (size_t i = 0; i < methodNames.size(); ++i) {
                const std::string &methodName = methodNames.at(i);
                if (ImGui::Selectable(methodName.c_str())) {
                    currentMethodName = methodName;
                }
                if (currentMethodName == methodName) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if(anyOutputMultiLayered) {
            ImGui::Text("The output is layered, what Limon should iterate over? ");
            if (ImGui::BeginCombo("Iterate of##RenderMethodCombo", LIGHT_TYPES[iterateOverLightType].c_str())) {
                for (size_t i = 0; i < sizeof(LIGHT_TYPES) / sizeof(LIGHT_TYPES[0]); ++i) {
                    const std::string &methodName = LIGHT_TYPES[i];
                    if (ImGui::Selectable(methodName.c_str())) {
                        iterateOverLightType = i;
                    }
                    if (iterateOverLightType == i) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
        }

    }

    ImGui::Unindent( 16.0f );
}
