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
            if (inputTextureIndexes.find(connection->getId()) == inputTextureIndexes.end()) {
                inputTextureIndexes[connection->getId()] = 0;
            }
            ImGui::InputInt((connection->getName() + " texture Attachment Index").c_str(), &(inputTextureIndexes[connection->getId()]));
            if (inputTextureIndexes[connection->getId()] < 0) {
                inputTextureIndexes[connection->getId()] = 0;//Don't allow negative values
            }
        }
        ImGui::EndChild();
    }
    if (ImGui::CollapsingHeader("Output Textures##PipelineStageExtension")) {

        ImGui::BeginChild("Output Textures##PipelineStageExtensionOutputRegion", ImVec2(0, 120), true, ImGuiWindowFlags_HorizontalScrollbar);
        auto usedTextures = this->pipelineExtension->getUsedTextures();
        for (const Connection *connection:node->getOutputConnections()) {
            std::string currentTextureName = "None";
            if (outputTextures.find(connection->getId()) != outputTextures.end()) {
                currentTextureName = outputTextures[connection->getId()].name;
            }
            if (ImGui::BeginCombo(connection->getName().c_str(), currentTextureName.c_str())) {
                for (auto & usedTexture : usedTextures) {
                    if (ImGui::Selectable(usedTexture.first.c_str())) {
                        OutputTextureInfo textureInfo;
                        textureInfo.name = usedTexture.first;
                        textureInfo.texture = usedTexture.second;
                        outputTextures[connection->getId()] = textureInfo;
                        //On output change, lets check if any of the outputs is multi layered
                        for(auto & outputTexture : outputTextures) {
                            this->anyOutputMultiLayered = false;
                            if(outputTexture.second.texture->getType() == GraphicsInterface::TextureTypes::T2D_ARRAY ||
                               outputTexture.second.texture->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP ||
                               outputTexture.second.texture->getType() == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY
                            ) {
                                this->anyOutputMultiLayered = true;
                                break;
                            }
                        }
                    }
                    if (currentTextureName == usedTexture.first) {
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
    toScreenElement->SetText(toScreen ? "True" : "False");
    stageExtensionElement->InsertEndChild(toScreenElement);

    tinyxml2::XMLElement *iterateOverLightTypeElement = document.NewElement("IterateLightType");
    iterateOverLightTypeElement->SetText(iterateOverLightType);
    stageExtensionElement->InsertEndChild(iterateOverLightTypeElement);

    tinyxml2::XMLElement *renderMethodNameElement = document.NewElement("RenderMethodName");
    renderMethodNameElement->SetText(currentMethodName.c_str());
    stageExtensionElement->InsertEndChild(renderMethodNameElement);

    tinyxml2::XMLElement *outputTexturesElements = document.NewElement("OutputTextures");
    stageExtensionElement->InsertEndChild(outputTexturesElements);

    for(const auto& outputIt:outputTextures) {
        OutputTextureInfo outputInfo = outputIt.second;
        tinyxml2::XMLElement *outputInfoElement = document.NewElement("OutputInfo");
        outputTexturesElements->InsertEndChild(outputInfoElement);

        tinyxml2::XMLElement *connectionElement = document.NewElement("Connection");
        connectionElement->SetText(outputIt.first);
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

void PipelineStageExtension::deserialize(const std::string &fileName[[gnu::unused]], tinyxml2::XMLElement *nodeExtensionElement) {
    tinyxml2::XMLElement *cullModeElement = nodeExtensionElement->FirstChildElement("CullMode");
    this->cullMode = GraphicsInterface::CullModes::NONE;
    if (cullModeElement == nullptr || cullModeElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have CullMode. Defaulting to none" << std::endl;
    } else {
        std::string cullModeString = cullModeElement->GetText();
        if(cullModeString == "Front") {
            this->cullMode = GraphicsInterface::CullModes::FRONT;
        } else if(cullModeString == "Back") {
            this->cullMode = GraphicsInterface::CullModes::BACK;
        } else if(cullModeString == "None") {
            this->cullMode = GraphicsInterface::CullModes::NONE;
        } else if(cullModeString == "NoChange") {
            this->cullMode = GraphicsInterface::CullModes::NO_CHANGE;
        } else {
            std::cerr << "Pipeline stage extension cullMode is unknown. Defaulting to none" << std::endl;
        }
    }

    tinyxml2::XMLElement *clearBeforeElement = nodeExtensionElement->FirstChildElement("ClearBefore");
    this->clearBefore = true;
    if(clearBeforeElement == nullptr || clearBeforeElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have ClearBefore flag. Defaulting to true" << std::endl;
    } else {
        std::string clearBeforeString = clearBeforeElement->GetText();
        if(clearBeforeString == "True") {
            clearBefore = true;
        } else if(clearBeforeString == "False") {
            clearBefore =false;
        } else {
            std::cerr << "Pipeline stage extension doesn't Clear ClearBefore value is unknown. Defaulting to true" << std::endl;
        }
    }
    
    tinyxml2::XMLElement *blendEnabledElement = nodeExtensionElement->FirstChildElement("BlendEnabled");
    this->blendEnabled = true;
    if(blendEnabledElement == nullptr || blendEnabledElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have BlendEnabled flag. Defaulting to true" << std::endl;
    } else {
        std::string blendEnabledString = blendEnabledElement->GetText();
        if(blendEnabledString == "True") {
            blendEnabled = true;
        } else if(blendEnabledString == "False") {
            blendEnabled =false;
        } else {
            std::cerr << "Pipeline stage extension doesn't BlendEnabled flag value is unknown. Defaulting to true" << std::endl;    
        }
    }
    
    tinyxml2::XMLElement *anyOutputMultilayeredElement = nodeExtensionElement->FirstChildElement("AnyOutputMultiLayered");
    this->anyOutputMultiLayered = true;
    if(anyOutputMultilayeredElement == nullptr || anyOutputMultilayeredElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have AnyOutputMultiLayered flag. Defaulting to true" << std::endl;
    } else {
        std::string anyOutputMultilayeredString = anyOutputMultilayeredElement->GetText();
        if(anyOutputMultilayeredString == "True") {
            anyOutputMultiLayered = true;
        } else if(anyOutputMultilayeredString == "False") {
            anyOutputMultiLayered =false;
        } else {
            std::cerr << "Pipeline stage extension doesn't AnyOutputMultiLayered flag value is unknown. Defaulting to true" << std::endl;
        }
    }

    tinyxml2::XMLElement *renderMethodNameElement = nodeExtensionElement->FirstChildElement("RenderMethodName");
    if(renderMethodNameElement == nullptr || renderMethodNameElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have Render method name. " << std::endl;
    } else {
        this->currentMethodName = renderMethodNameElement->GetText();
    }

    tinyxml2::XMLElement *toScreenElement = nodeExtensionElement->FirstChildElement("ToScreen");
    this->toScreen = true;
    if(toScreenElement == nullptr || toScreenElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have ToScreen flag. Defaulting to true" << std::endl;
    } else {
        std::string toScreenString = toScreenElement->GetText();
        if(toScreenString == "True") {
            toScreen = true;
        } else if(toScreenString == "False") {
            toScreen =false;
        } else {
            std::cerr << "Pipeline stage extension doesn't Clear ToScreen value is unknown. Defaulting to true" << std::endl;
        }
    }

    tinyxml2::XMLElement *iterateLightTypeElement = nodeExtensionElement->FirstChildElement("IterateLightType");
    this->iterateOverLightType = 0;
    if(iterateLightTypeElement == nullptr || iterateLightTypeElement->GetText() == nullptr) {
        std::cerr << "Pipeline stage extension doesn't have IterateLightType. Defaulting to 0. This might cause issues." << std::endl;
    } else {
        std::string iterateLightTypeString = iterateLightTypeElement->GetText();
        this->iterateOverLightType = std::stoul(iterateLightTypeString);
    }

    //deserialize the outputs.
    tinyxml2::XMLElement *outputTexturesElement = nodeExtensionElement->FirstChildElement("OutputTextures");
    if(outputTexturesElement == nullptr ) {
        std::cerr << "Pipeline stage extension doesn't have OutputTextures. This is most likely an error." << std::endl;
    } else {
        tinyxml2::XMLElement *outputInfoElement = outputTexturesElement->FirstChildElement("OutputInfo"); {
            while (outputInfoElement != nullptr) {
                OutputTextureInfo info;

                tinyxml2::XMLElement *nameElement = outputInfoElement->FirstChildElement("Name");
                if(nameElement == nullptr || nameElement->GetText() == nullptr) {
                    std::cerr << "Pipeline stage extension Output Info doesn't have name. This is most likely an error." << std::endl;
                } else {
                    info.name = nameElement->GetText();
                }

                tinyxml2::XMLElement *textureNameElement = outputInfoElement->FirstChildElement("TextureName");
                if(textureNameElement == nullptr || textureNameElement->GetText() == nullptr) {
                    std::cerr << "Pipeline stage extension Output Info doesn't have TextureName. This is most likely an error." << std::endl;
                } else {
                    std::string textureNameString = textureNameElement->GetText();
                    auto& usedTextures = this->pipelineExtension->getUsedTextures();
                    if(usedTextures.find(textureNameString) == usedTextures.end()) {
                        std::cerr << "Pipeline stage extension Output Info TextureName not found. This is most likely an error." << std::endl;
                    }  else {
                        info.texture = usedTextures.at(textureNameString);
                    }
                }

                tinyxml2::XMLElement *outputInfoNameElement = outputInfoElement->FirstChildElement("AttachPoint");
                if(outputInfoNameElement == nullptr || outputInfoNameElement->GetText() == nullptr) {
                    std::cerr << "Pipeline stage extension Output Info doesn't have AttachPoint. This is most likely an error." << std::endl;
                } else {
                    std::string attachPointString = outputInfoNameElement->GetText();
                    if (attachPointString == "NONE") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::NONE; }
                    else if (attachPointString == "COLOR0") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR0; }
                    else if (attachPointString == "COLOR1") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR1; }
                    else if (attachPointString == "COLOR2") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR2; }
                    else if (attachPointString == "COLOR3") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR3; }
                    else if (attachPointString == "COLOR4") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR4; }
                    else if (attachPointString == "COLOR5") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR5; }
                    else if (attachPointString == "COLOR6") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR6; }
                    else if (attachPointString == "DEPTH") { info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::DEPTH; }
                    else {
                        std::cerr << "Pipeline stage extension Output Info AttachPoint unknown. This is most likely an error." << std::endl;
                        info.attachPoint = GraphicsInterface::FrameBufferAttachPoints::NONE;
                    }
                }

                tinyxml2::XMLElement *connectionElement = outputInfoElement->FirstChildElement("Connection");
                if(connectionElement == nullptr || connectionElement->GetText() == nullptr) {
                    std::cerr << "Pipeline stage extension Output Info doesn't have ConnectionId. This is most likely an error. Skipping" << std::endl;
                } else {
                     uint32_t connectionId = std::stoul(connectionElement->GetText());
                     this->outputTextures[connectionId] = info;
                }
                outputInfoElement = outputInfoElement->NextSiblingElement("OutputInfo");
            }
        }
    }
}

int PipelineStageExtension::getInputTextureIndex(const Connection *connection) const {
    auto indexIt = inputTextureIndexes.find(connection->getId());
    if(indexIt == inputTextureIndexes.end()) {
        return 0;
    } else {
        return indexIt->second;
    }
}

GraphicsInterface::FrameBufferAttachPoints PipelineStageExtension::getOutputTextureIndex(const Connection* connection) const {
    auto indexIt = outputTextures.find(connection->getId());
    if(indexIt == outputTextures.end()) {
        return GraphicsInterface::FrameBufferAttachPoints::NONE;
    } else {
        return indexIt->second.attachPoint;
    }
}
std::shared_ptr<Texture> PipelineStageExtension::getOutputTexture(const Connection* connection) const {
    auto textureIt = outputTextures.find(connection->getId());
    if(textureIt == outputTextures.end()) {
        return nullptr;
    } else {
        return textureIt->second.texture;
    }
}