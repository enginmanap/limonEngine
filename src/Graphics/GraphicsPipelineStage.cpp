//
// Created by engin on 5.03.2019.
//

#include "GraphicsPipelineStage.h"

void GraphicsPipelineStage::activate(bool clear) {
    glHelper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, clear && colorAttachment, clear && depthAttachment, cullMode, inputs);
}

void GraphicsPipelineStage::activate(const std::map<std::shared_ptr<Texture>, std::pair<GLHelper::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear) {
    glHelper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, clear && colorAttachment, clear && depthAttachment, cullMode, inputs, attachmentLayerMap);
}

bool GraphicsPipelineStage::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options) {
    tinyxml2::XMLElement *stageNode = document.NewElement("GraphicsPipelineStage");
    parentNode->InsertEndChild(stageNode);
    tinyxml2::XMLElement *currentElement = nullptr;

    currentElement = document.NewElement("RenderHeight");
    currentElement->SetText(std::to_string(this->renderHeight).c_str());
    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("RenderWidth");
    currentElement->SetText(std::to_string(this->renderWidth).c_str());
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

    currentElement = document.NewElement("ToScreen");
    if(depthAttachment && colorAttachment && frameBufferID == 0) {
        currentElement->SetText("True");
    } else {
        currentElement->SetText("False");
    }

    stageNode->InsertEndChild(currentElement);

    currentElement = document.NewElement("CullMode");
    switch (cullMode) {
        case GLHelper::CullModes::NONE: currentElement->SetText("NONE"); break;
        case GLHelper::CullModes::BACK: currentElement->SetText("BACK"); break;
        case GLHelper::CullModes::FRONT: currentElement->SetText("FRONT"); break;
        case GLHelper::CullModes::NO_CHANGE: currentElement->SetText("NO_CHANGE"); break;
    }
    stageNode->InsertEndChild(currentElement);

    //now serialize inputs
    currentElement = document.NewElement("Inputs");
    stageNode->InsertEndChild(currentElement);
    for(auto input:inputs) {
        tinyxml2::XMLElement *inputElement = document.NewElement("Input");
        inputElement->SetAttribute("Index", input.first);
        input.second->serialize(document, inputElement, options);
        currentElement->InsertEndChild(inputElement);
    }

    //now serialize outputs
    currentElement = document.NewElement("Outputs");
    stageNode->InsertEndChild(currentElement);
    for(auto output:outputs) {
        tinyxml2::XMLElement *outputElement = document.NewElement("Output");
        switch(output.first) {
            case GLHelper::FrameBufferAttachPoints::NONE : outputElement->SetAttribute("Attachment", "NONE"); break;
            case GLHelper::FrameBufferAttachPoints::DEPTH : outputElement->SetAttribute("Attachment", "DEPTH"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR0 : outputElement->SetAttribute("Attachment", "COLOR0"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR1 : outputElement->SetAttribute("Attachment", "COLOR1"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR2 : outputElement->SetAttribute("Attachment", "COLOR2"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR3 : outputElement->SetAttribute("Attachment", "COLOR3"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR4 : outputElement->SetAttribute("Attachment", "COLOR4"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR5 : outputElement->SetAttribute("Attachment", "COLOR5"); break;
            case GLHelper::FrameBufferAttachPoints::COLOR6 : outputElement->SetAttribute("Attachment", "COLOR6"); break;
        }
        output.second->serialize(document, outputElement, options);
        currentElement->InsertEndChild(outputElement);
    }
    return true;
}

GraphicsPipelineStage *GraphicsPipelineStage::deserialize(tinyxml2::XMLElement *stageNode, GLHelper *glHelper, Options *options) {
    tinyxml2::XMLElement* stageNodeAttribute = nullptr;

    uint32_t renderHeight, renderWidth;
    bool blendEnabled = false;
    bool toScreen = false;
    GLHelper::CullModes cullMode = GLHelper::CullModes::NO_CHANGE;

    stageNodeAttribute = stageNode->FirstChildElement("RenderHeight");
    if (stageNodeAttribute == nullptr) {
        std::cerr << "Pipeline stage must have Render Height. Skipping" << std::endl;
        return nullptr;
    }
    if(stageNodeAttribute->GetText() == nullptr) {
        std::cerr << "Pipeline stage Render Height has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string heightString = stageNodeAttribute->GetText();
    renderHeight = std::stoi(heightString);

    stageNodeAttribute = stageNode->FirstChildElement("renderWidth");
    if (stageNodeAttribute == nullptr) {
        std::cerr << "Pipeline stage must have Render Width. Skipping" << std::endl;
        return nullptr;
    }
    if(stageNodeAttribute->GetText() == nullptr) {
        std::cerr << "Pipeline stage Render Width has no text, skipping! " << std::endl;
        return nullptr;
    }
    std::string widthString = stageNodeAttribute->GetText();
    renderWidth = std::stoi(widthString);

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
        std::cerr << "Pipeline Stage To screen setting couldn't be found, skippin!" << std::endl;
        return nullptr;
    }

    GraphicsPipelineStage* newStage = new GraphicsPipelineStage(glHelper, renderWidth, renderHeight, blendEnabled, toScreen);

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
                cullMode = GLHelper::CullModes::NONE;
            } else if (cullModeString == "BACK") {
                cullMode = GLHelper::CullModes::BACK;
            } else if (cullModeString == "FRONT") {
                cullMode = GLHelper::CullModes::FRONT;
            } else if (cullModeString == "NO_CHANGE") {
                cullMode = GLHelper::CullModes::NO_CHANGE;
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
            tinyxml2::XMLElement *textureElement = inputElement->FirstChildElement("Texture");
            Texture* inputTexture = Texture::deserialize(textureElement, glHelper, options);
            if(inputTexture == nullptr) {
                std::cerr << "For input " << index << " texture deserialize failed, skipping" << std::endl;
            } else {
                newStage->setInput(index, std::shared_ptr<Texture>(inputTexture));
            }
        }
        inputElement = stageNodeAttribute->NextSiblingElement("Input");
    }

    stageNodeAttribute = stageNode->FirstChildElement("Outputs");
    tinyxml2::XMLElement *outputElement = stageNodeAttribute->FirstChildElement("Output");
    while (outputElement != nullptr) {
        const char* attachmentRaw = outputElement->Attribute("Attachment");
        if(attachmentRaw == nullptr) {
            std::cerr << "Ouput attachment for Pipeline Stage can't be read, skipping" << std::endl;
        } else {
            bool fail = false;
            GLHelper::FrameBufferAttachPoints attachmentPoint;
            std::string attachmentString = attachmentRaw;
            if(attachmentString == "NONE") { attachmentPoint = GLHelper::FrameBufferAttachPoints::NONE;}
            else if(attachmentString == "DEPTH") { attachmentPoint = GLHelper::FrameBufferAttachPoints::DEPTH;}
            else if(attachmentString == "COLOR0") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR0;}
            else if(attachmentString == "COLOR1") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR1;}
            else if(attachmentString == "COLOR2") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR2;}
            else if(attachmentString == "COLOR3") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR3;}
            else if(attachmentString == "COLOR4") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR4;}
            else if(attachmentString == "COLOR5") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR5;}
            else if(attachmentString == "COLOR6") { attachmentPoint = GLHelper::FrameBufferAttachPoints::COLOR6;}
            else {
                std::cerr << "Attachment point read failed, skipping " << std::endl;
                fail = true;
            }
            if(!fail) {
                tinyxml2::XMLElement *textureElement = outputElement->FirstChildElement("Texture");
                Texture *outputTexture = Texture::deserialize(textureElement, glHelper, options);
                if (outputTexture == nullptr) {
                    std::cerr << "For output " << attachmentString << " texture deserialize failed, skipping" << std::endl;
                } else {
                    newStage->setOutput(attachmentPoint, std::shared_ptr<Texture>(outputTexture), false, 0);
                }
            }
        }
        outputElement = stageNodeAttribute->NextSiblingElement("Output");
    }

    return newStage;
}


