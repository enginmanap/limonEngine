//
// Created by engin on 15.04.2019.
//

#include <ImGui/imgui.h>
#include <memory>
#include <nodeGraph/src/Node.h>
#include <nodeGraph/src/NodeGraph.h>
#include <Graphics/GraphicsPipeline.h>
#include <GameObjects/Light.h>
#include "PipelineExtension.h"
#include "Graphics/Texture.h"
#include "PipelineStageExtension.h"
#include "API/Graphics/GraphicsProgram.h"


PipelineExtension::PipelineExtension(GraphicsInterface *graphicsWrapper, std::shared_ptr<GraphicsPipeline> currentGraphicsPipeline, std::shared_ptr<AssetManager> assetManager, Options* options,
                                     const std::vector<std::string> &renderMethodNames, GraphicsPipeline::RenderMethods renderMethods)
        : graphicsWrapper(graphicsWrapper), assetManager(assetManager), options(options), renderMethodNames(renderMethodNames), renderMethods(renderMethods) {
    {

        for(std::shared_ptr<Texture> texture:currentGraphicsPipeline->getTextures()) {
            usedTextures[texture->getName()] = texture;
        }

        //Add a texture to the list as place holder for screen
        auto texture = std::make_shared<Texture>(graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, 1, 1);
        usedTextures["Screen"] = texture;
        auto texture2 = std::make_shared<Texture>(graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH, GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::UNSIGNED_BYTE, 1, 1);
        usedTextures["Screen Depth"];
    }
}

//This method is used only for ImGui texture name generation
bool PipelineExtension::getNameOfTexture(void* data, int index, const char** outText) {
    auto& textures = *static_cast<std::map<std::string, std::shared_ptr<Texture>>*>(data);
    if(index < 0 || (uint32_t)index >= textures.size()) {
        return false;
    }
    auto it = textures.begin();
    for (int i = 0; i < index; ++i) {
        it++;
    }

    *outText = it->first.c_str();
    return true;

}


void PipelineExtension::drawDetailPane(NodeGraph* nodeGraph, const std::vector<const Node *>& nodes, const Node* selectedNode [[gnu::unused]]) {
    ImGui::Text("Graphics Pipeline Details");
    int listbox_item_current = -1;//not static because I don't want user to select a item.

    if(ImGui::CollapsingHeader("Textures")) {
        ImGui::Text("Current Textures");
        ImGui::ListBox("##CurrentTextures", &listbox_item_current, PipelineExtension::getNameOfTexture,
                       static_cast<void *>(&this->usedTextures), this->usedTextures.size(), 10);
        if(ImGui::Button("Create Texture")) {
            ImGui::OpenPopup("create_texture_popup");
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("create_texture_popup")) {

        static GraphicsInterface::TextureTypes  textureType = GraphicsInterface::TextureTypes::T2D;
        ImGui::Text("Texture type:");
        if(ImGui::RadioButton("2D##Texture_type_PipelineExtension", textureType == GraphicsInterface::TextureTypes::T2D)) { textureType = GraphicsInterface::TextureTypes::T2D; }
        ImGui::SameLine();
        if(ImGui::RadioButton("2D Array##Texture_type_PipelineExtension", textureType == GraphicsInterface::TextureTypes::T2D_ARRAY)) { textureType = GraphicsInterface::TextureTypes::T2D_ARRAY; }
        ImGui::SameLine();
        if(ImGui::RadioButton("Cubemap##Texture_type_PipelineExtension", textureType == GraphicsInterface::TextureTypes::TCUBE_MAP)) { textureType = GraphicsInterface::TextureTypes::TCUBE_MAP; }
        ImGui::SameLine();
        if(ImGui::RadioButton("Cubemap Array##Texture_type_PipelineExtension", textureType == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY)) { textureType = GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY; }

        static GraphicsInterface::InternalFormatTypes internalFormatType = GraphicsInterface::InternalFormatTypes::RED;
        ImGui::Text("Internal Format type:");
        if(ImGui::RadioButton("RED##internalFormat_type_PipelineExtension", internalFormatType == GraphicsInterface::InternalFormatTypes::RED)) { internalFormatType = GraphicsInterface::InternalFormatTypes::RED; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB##internalFormat_type_PipelineExtension", internalFormatType == GraphicsInterface::InternalFormatTypes::RGB)) { internalFormatType = GraphicsInterface::InternalFormatTypes::RGB; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGBA##internalFormat_type_PipelineExtension", internalFormatType == GraphicsInterface::InternalFormatTypes::RGBA)) { internalFormatType = GraphicsInterface::InternalFormatTypes::RGBA; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB16F##internalFormat_type_PipelineExtension", internalFormatType == GraphicsInterface::InternalFormatTypes::RGB16F)) { internalFormatType = GraphicsInterface::InternalFormatTypes::RGB16F; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB32F##internalFormat_type_PipelineExtension", internalFormatType == GraphicsInterface::InternalFormatTypes::RGB32F)) { internalFormatType = GraphicsInterface::InternalFormatTypes::RGB32F; }
        ImGui::SameLine();
        if(ImGui::RadioButton("DEPTH##internalFormat_type_PipelineExtension", internalFormatType == GraphicsInterface::InternalFormatTypes::DEPTH)) { internalFormatType = GraphicsInterface::InternalFormatTypes::DEPTH; }

        static GraphicsInterface::FormatTypes formatType = GraphicsInterface::FormatTypes::RGB;
        ImGui::Text("Format type:");
        if(ImGui::RadioButton("RED##format_type_PipelineExtension", formatType == GraphicsInterface::FormatTypes::RED)) { formatType = GraphicsInterface::FormatTypes::RED; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB##format_type_PipelineExtension", formatType == GraphicsInterface::FormatTypes::RGB)) { formatType = GraphicsInterface::FormatTypes::RGB; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGBA##format_type_PipelineExtension", formatType == GraphicsInterface::FormatTypes::RGBA)) { formatType = GraphicsInterface::FormatTypes::RGBA; }
        ImGui::SameLine();
        if(ImGui::RadioButton("DEPTH##format_type_PipelineExtension", formatType == GraphicsInterface::FormatTypes::DEPTH)) { formatType = GraphicsInterface::FormatTypes::DEPTH; }

        static GraphicsInterface::DataTypes dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE;
        ImGui::Text("Data type:");
        if(ImGui::RadioButton("UNSIGNED_BYTE##data_type_PipelineExtension", dataType == GraphicsInterface::DataTypes::UNSIGNED_BYTE)) { dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE; }
        ImGui::SameLine();
        if(ImGui::RadioButton("FLOAT##data_type_PipelineExtension", dataType == GraphicsInterface::DataTypes::FLOAT)) { dataType = GraphicsInterface::DataTypes::FLOAT; }

        static GraphicsInterface::TextureWrapModes  textureWrapMode = GraphicsInterface::TextureWrapModes::NONE;
        ImGui::Text("Wrap Mode:");
        if(ImGui::RadioButton("NONE##wrap_mode_PipelineExtension", textureWrapMode == GraphicsInterface::TextureWrapModes::NONE)) { textureWrapMode = GraphicsInterface::TextureWrapModes::NONE; }
        ImGui::SameLine();
        if(ImGui::RadioButton("REPEAT##wrap_mode_PipelineExtension", textureWrapMode == GraphicsInterface::TextureWrapModes::REPEAT)) { textureWrapMode = GraphicsInterface::TextureWrapModes::REPEAT; }
        ImGui::SameLine();
        if(ImGui::RadioButton("BORDER##wrap_mode_PipelineExtension", textureWrapMode == GraphicsInterface::TextureWrapModes::BORDER)) { textureWrapMode = GraphicsInterface::TextureWrapModes::BORDER; }
        ImGui::SameLine();
        if(ImGui::RadioButton("EDGE##wrap_mode_PipelineExtension", textureWrapMode == GraphicsInterface::TextureWrapModes::EDGE)) { textureWrapMode = GraphicsInterface::TextureWrapModes::EDGE; }

        static GraphicsInterface::FilterModes  filterMode = GraphicsInterface::FilterModes::NEAREST;
        ImGui::Text("Filter Mode:");
        if(ImGui::RadioButton("NEAREST##wrap_mode_PipelineExtension", filterMode == GraphicsInterface::FilterModes::NEAREST)) { filterMode = GraphicsInterface::FilterModes::NEAREST; }
        ImGui::SameLine();
        if(ImGui::RadioButton("LINEAR##wrap_mode_PipelineExtension", filterMode == GraphicsInterface::FilterModes::LINEAR)) { filterMode = GraphicsInterface::FilterModes::LINEAR; }
        ImGui::SameLine();
        if(ImGui::RadioButton("TRILINEAR##wrap_mode_PipelineExtension", filterMode == GraphicsInterface::FilterModes::TRILINEAR)) { filterMode = GraphicsInterface::FilterModes::TRILINEAR; }

        static int size[2] = {1920, 1080};
        if(ImGui::InputInt2("Size##texture_size_PipelineExtension", size)) {
            for (int i = 0; i < 2; ++i) {
                if (size[i] < 1) {
                    size[i] = 1;
                }
                if (size[i] > 8192) {
                    size[i] = 8192;
                }
            }
        }
        static int depth = 0;
        if(textureType == GraphicsInterface::TextureTypes::T2D_ARRAY || textureType == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY) {
            if(ImGui::InputInt("Depth##texture_depth_PipelineExtension", &depth)) {
                if(depth < 0 ) {
                    depth = 0;
                }
                if(depth > 1024) {
                    depth = 1024;
                }
            }
        }
        static float borderColor[] = {1.0, 1.0, 1.0, 1.0};
        if(ImGui::InputFloat4("Border Color##texture_borderColor_PipelineExtension", borderColor)) {
            for (int i = 0; i < 4; ++i) {
                if (borderColor[i] < 0) {
                    borderColor[i] = 0;
                }
                if (borderColor[i] > 1.0) {
                    borderColor[i] = 1;
                }
            }
        }
        static char name[256] = {0};
        ImGui::InputText("##texture_name_PipelineExtension",name, sizeof(name)-1, ImGuiInputTextFlags_CharsNoBlank);
        ImGui::SameLine();
        if(std::strlen(name) == 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::Text("TextureName");
            ImGui::PopStyleColor();
        } else {
            ImGui::Text("TextureName");
        }
        if(ImGui::Button("Create Texture##create_button_PipelineExtension")) {
            if(std::strlen(name) != 0) {
                std::shared_ptr<Texture> texture = std::make_shared<Texture>(graphicsWrapper, textureType, internalFormatType, formatType, dataType, size[0], size[1],
                                                                                                 depth);
                texture->setWrapModes(textureWrapMode, textureWrapMode);
                texture->setBorderColor(borderColor[0], borderColor[1], borderColor[2], borderColor[3]);
                texture->setFilterMode(filterMode);
                texture->setName(name);
                this->usedTextures[name] = texture;
                memset(name, 0, sizeof(name));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    if(ImGui::Button("Build Pipeline")) {
        const Node* rootNode = nullptr;
        for(const Node* node: nodes) {
            /**
             * what to do?
             * find node that outputs to screen, iterate back to find all other nodes that needs rendering
             */

             if(node->getName() == "Screen") {
                 rootNode = node;
                 break;
             }
        }
        if(rootNode == nullptr) {
            std::cout << "Screen output not found. cancelling." << std::endl;
        } else {
            GraphicsPipeline* graphicsPipeline = new GraphicsPipeline(renderMethods);
            for(auto usedTexture:usedTextures) {
                if(usedTexture.second != nullptr) {
                    graphicsPipeline->addTexture(usedTexture.second);
                }
            }
            std::map<const Node*, std::shared_ptr<GraphicsPipelineStage>> nodeStages;
            buildRenderPipelineRecursive(rootNode, graphicsPipeline, nodeStages);
            graphicsPipeline->serialize("./Data/renderPipelineBuilt.xml", options);
            addMessage("Built new Pipeline");
        }
    }
    ImGui::PopStyleVar();

    for(auto message:errorMessages) {
        nodeGraph->addError(message);
    }
    errorMessages.clear();

    for(auto message:messages) {
        nodeGraph->addMessage(message);
    }
    messages.clear();
}

void PipelineExtension::buildRenderPipelineRecursive(const Node *node,
                                                     GraphicsPipeline *graphicsPipeline,
                                                     std::map<const Node*, std::shared_ptr<GraphicsPipelineStage>>& nodeStages) {

    if(nodeStages.find(node) != nodeStages.end()) {
        return;
    }
    for(const Connection* connection:node->getInputConnections()) {
        //each connection can also have multiple inputs.
        for(Connection* connectionInputs:connection->getInputConnections()) {
            Node *inputNode = connectionInputs->getParent();
            if(nodeStages.find(inputNode) == nodeStages.end()) {
                buildRenderPipelineRecursive(inputNode, graphicsPipeline, nodeStages);
            }
        }
    }

    //after all inputs are put in graphics pipeline, or no input case
    auto stageExtension = dynamic_cast<PipelineStageExtension*>(node->getExtension());

    if(stageExtension != nullptr) {
        //uint32_t nodeAttachmentPoints = 1;
        std::shared_ptr<GraphicsPipelineStage> newStage;
        bool toScreen = false;
        if(!node->getOutputConnections().empty() && !node->getOutputConnections()[0]->getConnectedNodes().empty()) {
            for(auto connection:node->getOutputConnections()) {
                for(auto connectedNodes:connection->getConnectedNodes()){
                    if(connectedNodes->getName() == "Screen") {
                        toScreen = true;
                        break;
                    }
                }
                if(toScreen) {
                    break;
                }
            }
        }
        std::shared_ptr<GraphicsProgram> stageProgram;
        if(stageExtension->getProgramNameInfo().geometryShaderName.empty()) {
            stageProgram = std::make_shared<GraphicsProgram>(assetManager.get(),stageExtension->getProgramNameInfo().vertexShaderName,
                                                                                                   stageExtension->getProgramNameInfo().fragmentShaderName,
                                                                                                   true);//FIXME: is material required should be part of program info
        } else {
            stageProgram = std::make_shared<GraphicsProgram>(assetManager.get(),stageExtension->getProgramNameInfo().vertexShaderName,
                                                                                                   stageExtension->getProgramNameInfo().geometryShaderName,
                                                                                                   stageExtension->getProgramNameInfo().fragmentShaderName,
                                                                                                   true);//FIXME: is material required should be part of program info
        }

        newStage = std::make_shared<GraphicsPipelineStage>(graphicsWrapper,
                                                           1920,
                                                           1080,
                                                           stageExtension->isBlendEnabled(),
                                                           stageExtension->isDepthTestEnabled(),
                                                           stageExtension->isScissorTestEnabled(),
                                                           toScreen);
        uint32_t location = 1;
        for(const Connection *connection:node->getInputConnections()) { //connect the inputs to current stage, since all of them now have a Stage build.
            std::shared_ptr<Texture> inputTexture = nullptr;
            for (Connection *inputConnection:connection->getInputConnections()) {

                Node *inputNode = inputConnection->getParent();
                if (inputNode->getExtension() != nullptr) {
                    //TODO: Extension should force all connections to use the same texture.
                    PipelineStageExtension *inputNodeExtension = dynamic_cast<PipelineStageExtension *>(inputNode->getExtension());
                    if(inputTexture == nullptr) {
                        inputTexture = inputNodeExtension->getOutputTexture(inputConnection);
                    } else {
                        if (inputTexture->getTextureID() != inputNodeExtension->getOutputTexture(inputConnection)->getTextureID()) {
                            std::cerr << "Different textures are set for same connection. This is illegal." << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Input node extension is not PipelineStageExtension, this is not handled!" << std::endl;
                }
            }
            if(inputTexture != nullptr) {
                auto stageProgramUniforms = stageProgram->getUniformMap();
                if (stageProgramUniforms.find(connection->getName()) != stageProgramUniforms.end()) {
                    //FIXME these should not be hard coded, but they are because of missing material editor.
                    if (connection->getName() == "pre_shadowDirectional") {
                        newStage->setInput(graphicsWrapper->getMaxTextureImageUnits() - 1, inputTexture);
                    } else if (connection->getName() == "pre_shadowPoint") {
                        newStage->setInput(graphicsWrapper->getMaxTextureImageUnits() - 2, inputTexture);
                    } else {
                        newStage->setInput(stageProgramUniforms[connection->getName()]->location, inputTexture);
                    }
                }
            }
        }

        //now handle outputs
        // Directional depth map requires layer settings therefore it is not set by us.
        std::shared_ptr<Texture> depthMapDirectional = nullptr;

        for(const Connection *connection:node->getOutputConnections()) {
            auto programOutputsMap = stageProgram->getOutputMap();
            if(programOutputsMap.find(connection->getName()) != programOutputsMap.end()) {
                auto frameBufferAttachmentPoint = programOutputsMap.find(connection->getName())->second.second;

                if(stageExtension->getOutputTextureInfo(connection)->name != "Screen" &&
                        stageExtension->getOutputTextureInfo(connection)->name != "Screen Depth" ) {//for screen we don't need to attach anything
                    if (stageExtension->getOutputTexture(connection)->getFormat() == GraphicsInterface::FormatTypes::DEPTH &&
                        stageExtension->getOutputTexture(connection)->getType() == GraphicsInterface::TextureTypes::T2D_ARRAY) {
                        depthMapDirectional = stageExtension->getOutputTexture(connection);
                    }
                    newStage->setOutput(frameBufferAttachmentPoint, stageExtension->getOutputTexture(connection));

                }
            }
        }
        newStage->setCullMode(stageExtension->getCullmode());
        GraphicsPipeline::StageInfo stageInfo;
        stageInfo.clear = stageExtension->isClearBefore();
        stageInfo.stage = newStage;
        if(stageExtension->getMethodName() == "All directional shadows") {
            GraphicsPipeline::RenderMethod functionToCall = graphicsPipeline->getRenderMethods().getRenderMethodAllDirectionalLights(newStage, depthMapDirectional, stageProgram);
            stageInfo.renderMethods.emplace_back(functionToCall);
            graphicsPipeline->addNewStage(stageInfo);
        } else if(stageExtension->getMethodName() == "All point shadows") {
            GraphicsPipeline::RenderMethod functionToCall = graphicsPipeline->getRenderMethods().getRenderMethodAllPointLights(stageProgram);
            stageInfo.renderMethods.emplace_back(functionToCall);
            graphicsPipeline->addNewStage(stageInfo);
        } else {
            bool isFound = true;
            GraphicsPipeline::RenderMethod functionToCall = graphicsPipeline->getRenderMethods().getRenderMethod(stageExtension->getMethodName(), stageProgram, isFound);
            if(isFound) {
                stageInfo.renderMethods.emplace_back(functionToCall);
                graphicsPipeline->addNewStage(stageInfo);
            } else {
                std::cerr << "Selected method name is invalid!" << std::endl;
            }
        }
        nodeStages[node] = newStage;
    } else {
        std::cerr << "Extension of the node is not PipelineStageExtension, this is not handled! " << std::endl;
    }
}

void PipelineExtension::serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement) {
    tinyxml2::XMLElement *graphicsExtensionElement = document.NewElement("EditorExtension");
    parentElement->InsertEndChild(graphicsExtensionElement);

    tinyxml2::XMLElement *nameElement = document.NewElement("Name");
    nameElement->SetText(getName().c_str());
    graphicsExtensionElement->InsertEndChild(nameElement);
/*
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
*/

    tinyxml2::XMLElement *usedTexturesElement = document.NewElement("UsedTextures");
    graphicsExtensionElement->InsertEndChild(usedTexturesElement);
    for(auto textureIt:usedTextures) {
        std::shared_ptr<Texture>& usedTexture = textureIt.second;
        if(usedTexture == nullptr) {
            std::cerr << "There is a texture entry with name " << textureIt.first << " without a texture, skipping" << std::endl;
            continue;
        }
        usedTexture->serialize(document, usedTexturesElement, options);
    }
}

void PipelineExtension::deserialize(const std::string &fileName[[gnu::unused]], tinyxml2::XMLElement *editorExtensionElement) {

    tinyxml2::XMLElement *usedTexturesElement = editorExtensionElement->FirstChildElement("UsedTextures");
    if (usedTexturesElement == nullptr) {
        std::cerr << "Pipeline extension doesn't have Used Textures. It is invalid" << std::endl;
        return;
    }
    tinyxml2::XMLElement *textureElement = usedTexturesElement->FirstChildElement("Texture");
    while(textureElement != nullptr) {
        std::shared_ptr<Texture> texture = Texture::deserialize(textureElement,this->graphicsWrapper, assetManager, options);
        usedTextures[texture->getName()] = texture;
        textureElement = textureElement->NextSiblingElement("Texture");
    }
}
