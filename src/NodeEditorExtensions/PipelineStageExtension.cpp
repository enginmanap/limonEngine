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
                currentTextureName = outputTextures[connection].name;
            }
            if (ImGui::BeginCombo(connection->getName().c_str(), currentTextureName.c_str())) {
                for (auto it = usedTextures.begin(); it != usedTextures.end(); it++) {
                    if (ImGui::Selectable(it->first.c_str())) {
                        OutputTextureInfo textureInfo;
                        textureInfo.name = it->first;
                        textureInfo.texture = it->second;
                        outputTextures[connection] = textureInfo;
                        //On output change, lets check if any of the outputs is multi layered
                        for(auto setOutput = outputTextures.begin(); setOutput != outputTextures.end(); setOutput++) {
                            this->anyOutputMultiLayered = false;
                            if(setOutput->second.texture->getType() == GraphicsInterface::TextureTypes::T2D_ARRAY ||
                               setOutput->second.texture->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP ||
                               setOutput->second.texture->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY
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

        ImGui::Checkbox("Clear", &clearBefore);
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

void PipelineStageExtension::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement) {
    tinyxml2::XMLElement *stageExtensionElement = document.NewElement("NodeExtension");
    parentElement->InsertEndChild(stageExtensionElement);

    tinyxml2::XMLElement *nameElement = document.NewElement("Name");
    nameElement->SetText(getName().c_str());
    stageExtensionElement->InsertEndChild(nameElement);

    tinyxml2::XMLElement *cullModeElement = document.NewElement("CullMode");
    switch (this->cullMode) {
        case GraphicsInterface::CullModes::FRONT:       cullModeElement->SetText("Front"); break;
        case GraphicsInterface::CullModes::BACK:        cullModeElement->SetText("Back"); break;
        case GraphicsInterface::CullModes::NONE:        cullModeElement->SetText("None"); break;
        case GraphicsInterface::CullModes::NO_CHANGE:   cullModeElement->SetText("NoChange"); break;
    }
    stageExtensionElement->InsertEndChild(cullModeElement);

    tinyxml2::XMLElement *clearBeforeElement = document.NewElement("ClearBefore");
    clearBeforeElement->SetText(clearBefore ? "True" : "False");
    stageExtensionElement->InsertEndChild(clearBeforeElement);

    tinyxml2::XMLElement *blendEnabledElement = document.NewElement("BlendEnabled");
    blendEnabledElement->SetText(blendEnabled ? "True" : "False");
    stageExtensionElement->InsertEndChild(blendEnabledElement);

    tinyxml2::XMLElement *anyOutputMultilayeredElement = document.NewElement("AnyOutputMultiLayered");
    anyOutputMultilayeredElement->SetText(anyOutputMultiLayered ? "True" : "False");
    stageExtensionElement->InsertEndChild(anyOutputMultilayeredElement);

    tinyxml2::XMLElement *toScreenElement = document.NewElement("ToScreen");
    blendEnabledElement->SetText(toScreen ? "True" : "False");
    stageExtensionElement->InsertEndChild(toScreenElement);

    tinyxml2::XMLElement *iterateOverLightTypeElement = document.NewElement("IterateLightType");
    blendEnabledElement->SetText(iterateOverLightType);
    stageExtensionElement->InsertEndChild(iterateOverLightTypeElement);

    tinyxml2::XMLElement *outputTexturesElements = document.NewElement("OutputTextures");
    stageExtensionElement->InsertEndChild(outputTexturesElements);

    for(const auto& outputIt:outputTextures) {
        OutputTextureInfo outputInfo = outputIt.second;
        tinyxml2::XMLElement *outputInfoElement = document.NewElement("OutputInfo");
        outputTexturesElements->InsertEndChild(outputInfoElement);

        tinyxml2::XMLElement *connectionElement = document.NewElement("Connection");
        connectionElement->SetText(outputIt.first->getId());
        outputInfoElement->InsertEndChild(connectionElement);

        tinyxml2::XMLElement *outputInfoNameElement = document.NewElement("Name");
        outputInfoNameElement->SetText(outputInfo.name.c_str());
        outputInfoElement->InsertEndChild(outputInfoNameElement);

        tinyxml2::XMLElement *outputInfoAttachPointElement = document.NewElement("AttachPoint");
        std::string attachPointName;
        switch (outputInfo.attachPoint) {
            case GraphicsInterface::FrameBufferAttachPoints::NONE: attachPointName = "NONE"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR0: attachPointName = "COLOR0"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR1: attachPointName = "COLOR1"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR2: attachPointName = "COLOR2"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR3: attachPointName = "COLOR3"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR4: attachPointName = "COLOR4"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR5: attachPointName = "COLOR5"; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR6: attachPointName = "COLOR6"; break;
            case GraphicsInterface::FrameBufferAttachPoints::DEPTH: attachPointName = "DEPTH"; break;
        }
        outputInfoAttachPointElement->SetText(attachPointName.c_str());
        outputInfoElement->InsertEndChild(outputInfoAttachPointElement);

        tinyxml2::XMLElement *outputInfoTextureNameElement = document.NewElement("TextureName");
        outputInfoTextureNameElement->SetText(outputInfo.texture->getName().c_str());
        outputInfoElement->InsertEndChild(outputInfoTextureNameElement);
    }


}