//
// Created by engin on 15.04.2019.
//

#include <ImGui/imgui.h>
#include <memory>
#include <nodeGraph/src/Node.h>
#include <Graphics/GraphicsPipeline.h>
#include <GameObjects/Light.h>
#include "PipelineExtension.h"
#include "Graphics/Texture.h"
#include "PipelineStageExtension.h"
#include "API/GraphicsProgram.h"

PipelineExtension::PipelineExtension(GraphicsInterface* graphicsWrapper, const std::vector<std::string>& renderMethodNames, GraphicsPipeline::RenderMethods& renderMethods) : graphicsWrapper(graphicsWrapper), renderMethodNames(renderMethodNames), renderMethods(renderMethods) {
    {
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


void PipelineExtension::drawDetailPane(const std::vector<const Node *>& nodes, const Node* selectedNode [[gnu::unused]]) {
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
            buildRenderPipelineRecursive(rootNode, graphicsPipeline);
        }
    }
    ImGui::PopStyleVar();

}

void PipelineExtension::buildRenderPipelineRecursive(const Node *node, GraphicsPipeline *graphicsPipeline) {
    for(const Connection* connection:node->getInputConnections()) {
        if(connection->getInput() != nullptr) {
            Node *inputNode = connection->getInput()->getParent();
            buildRenderPipelineRecursive(inputNode, graphicsPipeline);
        }
    }
    //after all inputs are put in graphics pipeline, or no input case
    auto stageExtension = dynamic_cast<PipelineStageExtension*>(node->getExtension());

    if(stageExtension != nullptr) {
        std::shared_ptr<GraphicsPipelineStage> newStage;
        //FIXME this should be saved and used, not checked
        if(node->getOutputConnections().size() == 1 && node->getOutputConnections()[0]->getName() == "Screen") {
            newStage = std::make_shared<GraphicsPipelineStage>(graphicsWrapper, 1920, 1080, stageExtension->isBlendEnabled(), true);
        } else {
            newStage = std::make_shared<GraphicsPipelineStage>(graphicsWrapper, 1920, 1080, stageExtension->isBlendEnabled(), false);
        }
        for(const Connection *connection:node->getInputConnections()) {
            if(connection->getInput() != nullptr) {
                auto inputStageExtension = dynamic_cast<PipelineStageExtension *>(connection->getInput()->getParent()->getExtension());
                if (inputStageExtension != nullptr) {
                    newStage->setInput(stageExtension->getInputTextureIndex(connection), inputStageExtension->getOutputTexture(connection->getInput()));
                } else {
                    std::cerr << "Input node extension is not PipelineStageExtension, this is not handled!" << std::endl;
                };
            }
            std::cout << "Found input connection not feed for node " << node->getName() << " input " << connection->getName() << std::endl;
        }

        //TODO for directional lights, we need to determine which one of the outputs is the texture we want. For now, we assume the last one
        std::shared_ptr<Texture> depthMapDirectional = nullptr;
        //now handle outputs
        for(const Connection *connection:node->getOutputConnections()) {
            auto frameBufferAttachmentPoint = stageExtension->getOutputTextureIndex(connection);
            if(frameBufferAttachmentPoint != GraphicsInterface::FrameBufferAttachPoints::NONE) {
                newStage->setOutput(frameBufferAttachmentPoint, stageExtension->getOutputTexture(connection));
                depthMapDirectional = stageExtension->getOutputTexture(connection);
            }
        }

        //TODO: The program that was used too create the node should be accessible
        std::shared_ptr<GraphicsProgram> nodeProgram = nullptr;
        newStage->setCullMode(stageExtension->getCullmode());
        GraphicsPipeline::StageInfo stageInfo;
        stageInfo.clear = stageExtension->isClearBefore();
        stageInfo.stage = newStage;

        if(stageExtension->getMethodName() == "All directional shadows") {
            GraphicsPipeline::RenderMethod functionToCall = graphicsPipeline->getRenderMethods().getRenderMethodAllDirectionalLights(newStage, depthMapDirectional, nodeProgram);
            stageInfo.renderMethods.emplace_back(functionToCall);
            graphicsPipeline->addNewStage(stageInfo);
        } else if(stageExtension->getMethodName() == "All point shadows") {
            GraphicsPipeline::RenderMethod functionToCall = graphicsPipeline->getRenderMethods().getRenderMethodAllPointLights(nodeProgram);
            stageInfo.renderMethods.emplace_back(functionToCall);
            graphicsPipeline->addNewStage(stageInfo);
        } else {
            bool isFound = true;
            GraphicsPipeline::RenderMethod functionToCall = graphicsPipeline->getRenderMethods().getRenderMethod(stageExtension->getMethodName(), nodeProgram, isFound);
            if(isFound) {
                stageInfo.renderMethods.emplace_back(functionToCall);
                graphicsPipeline->addNewStage(stageInfo);
            } else {
                std::cerr << "Selected method name is invalid!" << std::endl;
            }
        }
    } else {
        std::cerr << "Extension of the node is not PipelineStageExtension, this is not handled! " << std::endl;
    }

}
