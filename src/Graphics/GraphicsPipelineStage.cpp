//
// Created by engin on 5.03.2019.
//

#include "GraphicsPipelineStage.h"

#include <Utils/StringUtils.hpp>

void GraphicsPipelineStage::activate(bool clear) {
    graphicsWrapper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, depthTestEnabled, depthWriteEnabled, scissorEnabled, clear && colorAttachment,
                                       clear && depthAttachment, cullMode, inputs, foundName);
}

void GraphicsPipelineStage::activate(const std::map<std::shared_ptr<Texture>, std::pair<GraphicsInterface::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear) {
    graphicsWrapper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, depthTestEnabled, depthWriteEnabled, scissorEnabled, clear && colorAttachment,
                                       clear && depthAttachment, cullMode, inputs, attachmentLayerMap, foundName);
}

bool GraphicsPipelineStage::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options [[gnu::unused]]) {
    tinyxml2::XMLElement *stageNode = document.NewElement("GraphicsPipelineStage");
    parentNode->InsertEndChild(stageNode);
    tinyxml2::XMLElement *currentElement = nullptr;

    currentElement = document.NewElement("DefaultRenderHeight");
    currentElement->SetText(std::to_string(this->defaultRenderHeight).c_str());
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DefaultRenderWidth");
    currentElement->SetText(std::to_string(this->defaultRenderWidth).c_str());
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("RenderHeightOption");
    currentElement->SetText(this->renderHeightOption.c_str());
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("RenderWidthOption");
    currentElement->SetText(this->renderWidthOption.c_str());
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("BlendEnabled");
    if(blendEnabled) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ColorAttachment");
    if(colorAttachment) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DepthAttachment");
    if(depthAttachment) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DepthTestEnabled");
    if(depthTestEnabled) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("DepthWriteEnabled");
    if(depthWriteEnabled) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ScissorEnabled");
    if(scissorEnabled) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("ToScreen");
    if(depthAttachment && colorAttachment && frameBufferID == 0) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("CameraTags");
    currentElement->SetText(StringUtils::join(cameraTags, ",").c_str());
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("CullMode");
    switch (cullMode) {
        case GraphicsInterface::CullModes::NONE: currentElement->SetText("NONE"); break;
        case GraphicsInterface::CullModes::BACK: currentElement->SetText("BACK"); break;
        case GraphicsInterface::CullModes::FRONT: currentElement->SetText("FRONT"); break;
        case GraphicsInterface::CullModes::NO_CHANGE: currentElement->SetText("NO_CHANGE"); break;
    }
    stageNode->InsertEndChild(currentElement);

    //now serialize inputs
    currentElement = document.NewElement("Inputs");
    stageNode->InsertEndChild(currentElement);
    for(auto input:inputs) {
        tinyxml2::XMLElement *inputElement = document.NewElement("Input");
        inputElement->SetAttribute("Index", input.first);
        inputElement->SetAttribute("textureID", input.second->getSerializeID());
        currentElement->InsertEndChild(inputElement);
    }

    //now serialize outputs
    currentElement = document.NewElement("Outputs");
    stageNode->InsertEndChild(currentElement);
    for(auto output:outputs) {
        tinyxml2::XMLElement *outputElement = document.NewElement("Output");
        switch(output.first) {
            case GraphicsInterface::FrameBufferAttachPoints::NONE : outputElement->SetAttribute("Attachment", "NONE"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR0 : outputElement->SetAttribute("Attachment", "COLOR0"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR1 : outputElement->SetAttribute("Attachment", "COLOR1"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR2 : outputElement->SetAttribute("Attachment", "COLOR2"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR3 : outputElement->SetAttribute("Attachment", "COLOR3"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR4 : outputElement->SetAttribute("Attachment", "COLOR4"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR5 : outputElement->SetAttribute("Attachment", "COLOR5"); break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR6 : outputElement->SetAttribute("Attachment", "COLOR6"); break;
            case GraphicsInterface::FrameBufferAttachPoints::DEPTH :  outputElement->SetAttribute("Attachment", "DEPTH"); break;
        }
        outputElement->SetAttribute("textureID", output.second->getSerializeID());

        currentElement->InsertEndChild(outputElement);
    }
    return true;
}

std::shared_ptr<GraphicsPipelineStage> GraphicsPipelineStage::deserialize(tinyxml2::XMLElement *stageNode, GraphicsInterface* graphicsWrapper, const std::vector<std::shared_ptr<Texture>>& textures) {
    tinyxml2::XMLElement* stageNodeAttribute = nullptr;

    uint32_t defaultRenderHeight, defaultRenderWidth;
    bool blendEnabled = false;
    bool toScreen = false;
    std::vector<std::string> cameraTags;
    GraphicsInterface::CullModes cullMode = GraphicsInterface::CullModes::NO_CHANGE;

    stageNodeAttribute = stageNode->FirstChildElement("DefaultRenderHeight");
    if (stageNodeAttribute == nullptr) {
        std::cerr << "Pipeline stage must have Render Height. Skipping" << std::endl;
        return nullptr;
    }
    if(stageNodeAttribute->GetText() == nullptr) {
        std::cerr << "Pipeline stage Render Height has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string heightString = stageNodeAttribute->GetText();
    defaultRenderHeight = std::stoi(heightString);

    stageNodeAttribute = stageNode->FirstChildElement("DefaultRenderWidth");
    if (stageNodeAttribute == nullptr) {
        std::cerr << "Pipeline stage must have Render Width. Skipping" << std::endl;
        return nullptr;
    }
    if(stageNodeAttribute->GetText() == nullptr) {
        std::cerr << "Pipeline stage Render Width has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string widthString = stageNodeAttribute->GetText();
    defaultRenderWidth = std::stoi(widthString);

    std::string widthOption;
    stageNodeAttribute = stageNode->FirstChildElement("RenderWidthOption");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Stage RenderWidthOption has no text, skipping! " << std::endl;
        } else {
            widthOption = stageNodeAttribute->GetText();
        }
    }

    std::string heightOption;
    stageNodeAttribute = stageNode->FirstChildElement("RenderHeightOption");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Stage RenderHeightOption has no text, skipping! " << std::endl;
        } else {
            heightOption = stageNodeAttribute->GetText();
        }
    }


    stageNodeAttribute = stageNode->FirstChildElement("BlendEnabled");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage blend enabled setting has no text, assuming false!" << std::endl;
        } else {
            std::string blendEnabledString = stageNodeAttribute->GetText();
            if(blendEnabledString == "True") {
                blendEnabled = true;
            } else if(blendEnabledString == "False") {
                blendEnabled = false;
            } else {
                std::cerr << "Pipeline Stage blend enabled setting is unknown, assuming false!" << std::endl;
            }
        }
    }

    stageNodeAttribute = stageNode->FirstChildElement("CameraTags");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage CameraTags setting has no text, assuming empty!" << std::endl;
        } else {
            std::string cameraTagsString = stageNodeAttribute->GetText();
            cameraTags = StringUtils::split(cameraTagsString, ",");
        }
    }

    bool colorAttachmentEnabled = true;
    stageNodeAttribute = stageNode->FirstChildElement("ColorAttachment");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage Color attachment setting has no text, assuming true!" << std::endl;
        } else {
            std::string colorAttachmentString = stageNodeAttribute->GetText();
            if(colorAttachmentString == "True") {
                colorAttachmentEnabled = true;
            } else if(colorAttachmentString == "False") {
                colorAttachmentEnabled = false;
            } else {
                std::cerr << "Pipeline Stage depth test enabled setting is unknown, assuming true!" << std::endl;
            }
        }
    }

    bool depthAttachmentEnabled = true;
    stageNodeAttribute = stageNode->FirstChildElement("DepthAttachment");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage Depth attachment setting has no text, assuming true!" << std::endl;
        } else {
            std::string depthAttachmentString = stageNodeAttribute->GetText();
            if(depthAttachmentString == "True") {
                depthAttachmentEnabled = true;
            } else if(depthAttachmentString == "False") {
                depthAttachmentEnabled = false;
            } else {
                std::cerr << "Pipeline Stage depth test enabled setting is unknown, assuming true!" << std::endl;
            }
        }
    }

    bool depthTestEnabled = true;
    stageNodeAttribute = stageNode->FirstChildElement("DepthTestEnabled");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage depth test enabled setting has no text, assuming true!" << std::endl;
        } else {
            std::string depthTestEnabledString = stageNodeAttribute->GetText();
            if(depthTestEnabledString == "True") {
                depthTestEnabled = true;
            } else if(depthTestEnabledString == "False") {
                depthTestEnabled = false;
            } else {
                std::cerr << "Pipeline Stage depth test enabled setting is unknown, assuming true!" << std::endl;
            }
        }
    }

    bool depthWriteEnabled = true;
    stageNodeAttribute = stageNode->FirstChildElement("DepthWriteEnabled");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage depth write enabled setting has no text, assuming true!" << std::endl;
        } else {
            std::string depthWriteEnabledString = stageNodeAttribute->GetText();
            if(depthWriteEnabledString == "True") {
                depthWriteEnabled = true;
            } else if(depthWriteEnabledString == "False") {
                depthWriteEnabled = false;
            } else {
                std::cerr << "Pipeline Stage depth write enabled setting is unknown, assuming true!" << std::endl;
            }
        }
    }

    bool scissorEnabled = false;
    stageNodeAttribute = stageNode->FirstChildElement("ScissorEnabled");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage scissor enabled setting has no text, assuming false!" << std::endl;
        } else {
            std::string ScissorEnabledString = stageNodeAttribute->GetText();
            if(ScissorEnabledString == "True") {
                scissorEnabled = true;
            } else if(ScissorEnabledString == "False") {
                scissorEnabled = false;
            } else {
                std::cerr << "Pipeline Stage scissor enabled setting is unknown, assuming false!" << std::endl;
            }
        }
    }

    stageNodeAttribute = stageNode->FirstChildElement("ToScreen");
    if (stageNodeAttribute != nullptr) {
        if(stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline Stage to screen setting has no text, skipping!" << std::endl;
            return nullptr;
        } else {
            std::string toScreenString = stageNodeAttribute->GetText();
            if(toScreenString == "True") {
                toScreen = true;
            } else if(toScreenString == "False") {
                toScreen = false;
            } else {
                std::cerr << "Pipeline Stage to screen setting is unknown, skipping!" << std::endl;
                return nullptr;
            }
        }
    } else {
        std::cerr << "Pipeline Stage To screen setting couldn't be found, skipping!" << std::endl;
        return nullptr;
    }

    std::shared_ptr<GraphicsPipelineStage> newStage = std::make_shared<GraphicsPipelineStage>(graphicsWrapper, defaultRenderWidth, defaultRenderHeight, widthOption, heightOption, blendEnabled, depthTestEnabled, depthWriteEnabled, scissorEnabled, toScreen);
    newStage->depthAttachment = depthAttachmentEnabled;
    newStage->colorAttachment = colorAttachmentEnabled;
    newStage->cameraTags = cameraTags;

    stageNodeAttribute = stageNode->FirstChildElement("CullMode");

    if (stageNodeAttribute == nullptr) {
        std::cerr << "Pipeline stage cull mode not set. defaulting to no change" << std::endl;
    } else {
        bool fail = false;
        if (stageNodeAttribute->GetText() == nullptr) {
            std::cerr << "Pipeline stage cull mode has no text, defaulting to no change " << std::endl;
        } else {
            std::string cullModeString = stageNodeAttribute->GetText();
            if (cullModeString == "NONE") {
                cullMode = GraphicsInterface::CullModes::NONE;
            } else if (cullModeString == "BACK") {
                cullMode = GraphicsInterface::CullModes::BACK;
            } else if (cullModeString == "FRONT") {
                cullMode = GraphicsInterface::CullModes::FRONT;
            } else if (cullModeString == "NO_CHANGE") {
                cullMode = GraphicsInterface::CullModes::NO_CHANGE;
            } else {
                std::cerr << "Texture type is unknown, defaulting to no change" << std::endl;
                fail = true;
            }
            if(!fail) {
                newStage->setCullMode(cullMode);
            }
        }
    }

    //now read inputs and outputs
    stageNodeAttribute = stageNode->FirstChildElement("Inputs");
    tinyxml2::XMLElement *inputElement = stageNodeAttribute->FirstChildElement("Input");
    while (inputElement != nullptr) {
        const char* indexRaw = inputElement->Attribute("Index");
        if(indexRaw == nullptr) {
            std::cerr << "Input index for Pipeline Stage can't be read, skipping" << std::endl;
        } else {
            int index = std::stoi(indexRaw);
            std::string textureID = inputElement->Attribute("textureID");
            if(textureID.empty()) {
                std::cerr << "Texture ID for index " << index << " can't be read, skipping" << std::endl;
            } else {
                bool found = false;
                for (const auto& texture:textures) {
                    if (texture->getSerializeID() == (uint32_t)stoi(textureID)) {
                        newStage->setInput(index, texture);
                        found = true;
                        break;
                    }
                }
                if(!found) {
                    std::cerr << "Texture ID "<< std::stoi(textureID) << " for index " << index << " can't be found" << std::endl;
                }
            }
        }
        inputElement = inputElement->NextSiblingElement("Input");
    }

    stageNodeAttribute = stageNode->FirstChildElement("Outputs");
    tinyxml2::XMLElement *outputElement = stageNodeAttribute->FirstChildElement("Output");
    while (outputElement != nullptr) {
        const char* attachmentRaw = outputElement->Attribute("Attachment");
        if(attachmentRaw == nullptr) {
            std::cerr << "Output attachment for Pipeline Stage can't be read, skipping" << std::endl;
        } else {
            bool fail = false;
            GraphicsInterface::FrameBufferAttachPoints attachmentPoint;
            std::string attachmentString = attachmentRaw;
            if(attachmentString == "NONE") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::NONE;}
            else if(attachmentString == "DEPTH") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::DEPTH;}
            else if(attachmentString == "COLOR0") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR0;}
            else if(attachmentString == "COLOR1") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR1;}
            else if(attachmentString == "COLOR2") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR2;}
            else if(attachmentString == "COLOR3") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR3;}
            else if(attachmentString == "COLOR4") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR4;}
            else if(attachmentString == "COLOR5") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR5;}
            else if(attachmentString == "COLOR6") { attachmentPoint = GraphicsInterface::FrameBufferAttachPoints::COLOR6;}
            else {
                std::cerr << "Attachment point read failed, skipping " << std::endl;
                fail = true;
            }
            if(!fail) {
                std::string textureID = outputElement->Attribute("textureID");
                std::shared_ptr<Texture> outputTexture;
                if(textureID.empty()) {
                    std::cerr << "Texture ID for output attachment " << attachmentString << " can't be read, skipping" << std::endl;
                } else {
                    bool found = false;
                    for (const auto& texture:textures) {
                        if (texture->getSerializeID() == std::stoul(textureID)) {
                            outputTexture = texture;
                            found = true;
                            break;
                        }
                    }
                    if(!found) {
                        std::cerr << "Texture ID "<< std::stoi(textureID) << " for attachment point " << attachmentString << " can't be found" << std::endl;
                    }
                }
                if (outputTexture == nullptr) {
                    std::cerr << "For output " << attachmentString << " texture deserialize failed, skipping" << std::endl;
                } else {
                    if(attachmentPoint != GraphicsInterface::FrameBufferAttachPoints::DEPTH ||
                            (depthWriteEnabled || depthTestEnabled)) { //if depth is not read or written, then don't attach it.
                        newStage->setOutput(attachmentPoint, outputTexture, false, -1);
                    }
                }
            }
        }
        outputElement = outputElement->NextSiblingElement("Output");
    }

    return newStage;
}