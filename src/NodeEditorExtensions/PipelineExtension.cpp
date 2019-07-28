//
// Created by engin on 15.04.2019.
//

#include <ImGui/imgui.h>
#include <memory>
#include <nodeGraph/src/Node.h>
#include <Graphics/GraphicsPipeline.h>
#include "PipelineExtension.h"
#include "Graphics/Texture.h"
#include "PipelineStageExtension.h"


PipelineExtension::PipelineExtension(GLHelper *glHelper) : glHelper(glHelper) {
    {
        //Add a texture to the list as place holder for screen
        auto texture = std::make_shared<Texture>(glHelper, GLHelper::TextureTypes::T2D, GLHelper::InternalFormatTypes::RGBA, GLHelper::FormatTypes::RGBA, GLHelper::DataTypes::UNSIGNED_BYTE, 1,1);
        usedTextures["Screen"] = texture;
        auto texture2 = std::make_shared<Texture>(glHelper, GLHelper::TextureTypes::T2D, GLHelper::InternalFormatTypes::DEPTH, GLHelper::FormatTypes::DEPTH, GLHelper::DataTypes::UNSIGNED_BYTE, 1,1);
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


void PipelineExtension::drawDetailPane() {
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

        static GLHelper::TextureTypes  textureType = GLHelper::TextureTypes::T2D;
        ImGui::Text("Texture type:");
        if(ImGui::RadioButton("2D##Texture_type_PipelineExtension", textureType == GLHelper::TextureTypes::T2D)) { textureType = GLHelper::TextureTypes::T2D; }
        ImGui::SameLine();
        if(ImGui::RadioButton("2D Array##Texture_type_PipelineExtension", textureType == GLHelper::TextureTypes::T2D_ARRAY)) { textureType = GLHelper::TextureTypes::T2D_ARRAY; }
        ImGui::SameLine();
        if(ImGui::RadioButton("Cubemap##Texture_type_PipelineExtension", textureType == GLHelper::TextureTypes::TCUBE_MAP)) { textureType = GLHelper::TextureTypes::TCUBE_MAP; }
        ImGui::SameLine();
        if(ImGui::RadioButton("Cubemap Array##Texture_type_PipelineExtension", textureType == GLHelper::TextureTypes::TCUBE_MAP_ARRAY)) { textureType = GLHelper::TextureTypes::TCUBE_MAP_ARRAY; }

        static GLHelper::InternalFormatTypes internalFormatType = GLHelper::InternalFormatTypes::RED;
        ImGui::Text("Internal Format type:");
        if(ImGui::RadioButton("RED##internalFormat_type_PipelineExtension", internalFormatType == GLHelper::InternalFormatTypes::RED)) { internalFormatType = GLHelper::InternalFormatTypes::RED; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB##internalFormat_type_PipelineExtension", internalFormatType == GLHelper::InternalFormatTypes::RGB)) { internalFormatType = GLHelper::InternalFormatTypes::RGB; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGBA##internalFormat_type_PipelineExtension", internalFormatType == GLHelper::InternalFormatTypes::RGBA)) { internalFormatType = GLHelper::InternalFormatTypes::RGBA; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB16F##internalFormat_type_PipelineExtension", internalFormatType == GLHelper::InternalFormatTypes::RGB16F)) { internalFormatType = GLHelper::InternalFormatTypes::RGB16F; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB32F##internalFormat_type_PipelineExtension", internalFormatType == GLHelper::InternalFormatTypes::RGB32F)) { internalFormatType = GLHelper::InternalFormatTypes::RGB32F; }
        ImGui::SameLine();
        if(ImGui::RadioButton("DEPTH##internalFormat_type_PipelineExtension", internalFormatType == GLHelper::InternalFormatTypes::DEPTH)) { internalFormatType = GLHelper::InternalFormatTypes::DEPTH; }

        static GLHelper::FormatTypes formatType = GLHelper::FormatTypes::RGB;
        ImGui::Text("Format type:");
        if(ImGui::RadioButton("RED##format_type_PipelineExtension", formatType == GLHelper::FormatTypes::RED)) { formatType = GLHelper::FormatTypes::RED; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGB##format_type_PipelineExtension", formatType == GLHelper::FormatTypes::RGB)) { formatType = GLHelper::FormatTypes::RGB; }
        ImGui::SameLine();
        if(ImGui::RadioButton("RGBA##format_type_PipelineExtension", formatType == GLHelper::FormatTypes::RGBA)) { formatType = GLHelper::FormatTypes::RGBA; }
        ImGui::SameLine();
        if(ImGui::RadioButton("DEPTH##format_type_PipelineExtension", formatType == GLHelper::FormatTypes::DEPTH)) { formatType = GLHelper::FormatTypes::DEPTH; }

        static GLHelper::DataTypes dataType = GLHelper::DataTypes::UNSIGNED_BYTE;
        ImGui::Text("Data type:");
        if(ImGui::RadioButton("UNSIGNED_BYTE##data_type_PipelineExtension", dataType == GLHelper::DataTypes::UNSIGNED_BYTE)) { dataType = GLHelper::DataTypes::UNSIGNED_BYTE; }
        ImGui::SameLine();
        if(ImGui::RadioButton("FLOAT##data_type_PipelineExtension", dataType == GLHelper::DataTypes::FLOAT)) { dataType = GLHelper::DataTypes::FLOAT; }

        static GLHelper::TextureWrapModes  textureWrapMode = GLHelper::TextureWrapModes::NONE;
        ImGui::Text("Wrap Mode:");
        if(ImGui::RadioButton("NONE##wrap_mode_PipelineExtension", textureWrapMode == GLHelper::TextureWrapModes::NONE)) { textureWrapMode = GLHelper::TextureWrapModes::NONE; }
        ImGui::SameLine();
        if(ImGui::RadioButton("REPEAT##wrap_mode_PipelineExtension", textureWrapMode == GLHelper::TextureWrapModes::REPEAT)) { textureWrapMode = GLHelper::TextureWrapModes::REPEAT; }
        ImGui::SameLine();
        if(ImGui::RadioButton("BORDER##wrap_mode_PipelineExtension", textureWrapMode == GLHelper::TextureWrapModes::BORDER)) { textureWrapMode = GLHelper::TextureWrapModes::BORDER; }
        ImGui::SameLine();
        if(ImGui::RadioButton("EDGE##wrap_mode_PipelineExtension", textureWrapMode == GLHelper::TextureWrapModes::EDGE)) { textureWrapMode = GLHelper::TextureWrapModes::EDGE; }

        static GLHelper::FilterModes  filterMode = GLHelper::FilterModes::NEAREST;
        ImGui::Text("Filter Mode:");
        if(ImGui::RadioButton("NEAREST##wrap_mode_PipelineExtension", filterMode == GLHelper::FilterModes::NEAREST)) { filterMode = GLHelper::FilterModes::NEAREST; }
        ImGui::SameLine();
        if(ImGui::RadioButton("LINEAR##wrap_mode_PipelineExtension", filterMode == GLHelper::FilterModes::LINEAR)) { filterMode = GLHelper::FilterModes::LINEAR; }
        ImGui::SameLine();
        if(ImGui::RadioButton("TRILINEAR##wrap_mode_PipelineExtension", filterMode == GLHelper::FilterModes::TRILINEAR)) { filterMode = GLHelper::FilterModes::TRILINEAR; }

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
        if(textureType == GLHelper::TextureTypes::T2D_ARRAY || textureType == GLHelper::TextureTypes::TCUBE_MAP_ARRAY) {
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
                std::shared_ptr<Texture> texture = std::make_shared<Texture>(glHelper, textureType, internalFormatType, formatType, dataType, size[0], size[1],
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
        std::vector<Node *> nodes;//Assume we can get this somehow
        Node* rootNode = nullptr;
        for(Node* node: nodes) {
            /**
             * what to do?
             * find node that outputs to screen, iterate back to find all other nodes that needs rendering
             */

             if(node->getOutputConnections().size() == 0 && node->getOutputConnections()[0]->getName() == "Screen") {
                 rootNode = node;
                 break;
             }
        }
        if(rootNode == nullptr) {
            std::cout << "Screen output not found. cancelling." << std::endl;
        } else {
            GraphicsPipeline* graphicsPipeline = new GraphicsPipeline();
            buildRenderPipelineRecursive(rootNode, graphicsPipeline);
        }
    }
    ImGui::PopStyleVar();

}

void PipelineExtension::buildRenderPipelineRecursive(Node *node, GraphicsPipeline *graphicsPipeline) {
    for(const Connection* connection:node->getInputConnections()) {
        Node* inputNode = connection->getInput()->getParent();
        buildRenderPipelineRecursive(inputNode, graphicsPipeline);
    }
    //after all inputs are put in graphics pipeline, or no input case
    auto stageExtension = dynamic_cast<PipelineStageExtension*>(node->getExtension());

    if(stageExtension != nullptr) {
        std::shared_ptr<GraphicsPipelineStage> newStage;
        //FIXME this should be saved and used, not checked
        if(node->getOutputConnections().size() == 0 && node->getOutputConnections()[0]->getName() == "Screen") {
            newStage = std::make_shared<GraphicsPipelineStage>(glHelper, 1920, 1080, stageExtension->isBlendEnabled(), true);
        } else {
            newStage = std::make_shared<GraphicsPipelineStage>(glHelper, 1920, 1080, stageExtension->isBlendEnabled(), false);
        }
        for(const Connection *connection:node->getInputConnections()) {
            auto inputStageExtension = dynamic_cast<PipelineStageExtension*>(connection->getInput()->getParent()->getExtension());
            if(inputStageExtension != nullptr) {
                newStage->setInput(stageExtension->getInputTextureIndex(connection), inputStageExtension->getOutputTexture(connection->getInput()));
            } else {
                std::cerr << "Input node extension is not PipelineStageExtension, this is not handled!" << std::endl;
            };
        }
        //now handle outputs
        for(const Connection *connection:node->getOutputConnections()) {
            auto frameBufferAttachmentPoint = stageExtension->getOutputTextureIndex(connection);
            newStage->setOutput(frameBufferAttachmentPoint, stageExtension->getOutputTexture(connection));
        }

        newStage->setCullMode(stageExtension->getCullmode());
        GraphicsPipeline::StageInfo stageInfo;
        stageInfo.clear = stageExtension->isClearBefore();
        stageInfo.stage = newStage;
        //graphicsPipeline->addNewStage(stageInfo, )

    } else {
        std::cerr << "Extension of the node is not PipelineStageExtension, this is not handled! " << std::endl;
    }

}
