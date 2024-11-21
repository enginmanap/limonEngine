//
// Created by engin on 15.04.2019.
//

#include <ImGui/imgui.h>
#include <memory>
#include <nodeGraph/src/Node.h>
#include <nodeGraph/src/NodeGraph.h>
#include <Graphics/GraphicsPipeline.h>
#include <GameObjects/Light.h>
#include <algorithm>
#include "PipelineExtension.h"
#include "Graphics/Texture.h"
#include "PipelineStageExtension.h"
#include "API/Graphics/GraphicsProgram.h"
#include "Utils/StringUtils.hpp"


PipelineExtension::PipelineExtension(GraphicsInterface *graphicsWrapper, std::shared_ptr<GraphicsPipeline> currentGraphicsPipeline, std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options* options,
                                     const std::vector<std::string> &renderMethodNames, RenderMethods renderMethods)
        : graphicsWrapper(graphicsWrapper), assetManager(assetManager), options(options), renderMethodNames(renderMethodNames), renderMethods(renderMethods) {
    {

        for(std::shared_ptr<Texture> texture:currentGraphicsPipeline->getTextures()) {
            usedTextures[texture->getName()] = texture;
        }

        //Add a texture to the list as place holder for screen
        if(usedTextures.find("ScreenPlaceHolder") == usedTextures.end()) {
            auto texture = std::make_shared<Texture>(graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA,
                                                     GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, 1, 1);
            texture->setName("ScreenPlaceHolder");
            usedTextures["Screen"] = texture;
        }
        if(usedTextures.find("ScreenPlaceHolder") == usedTextures.end()) {
            auto texture2 = std::make_shared<Texture>(graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH,
                                                      GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::UNSIGNED_BYTE, 1, 1);
            texture2->setName("ScreenDepthPlaceHolder");
            usedTextures["Screen Depth"] = texture2;
        }
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

    if(ImGui::CollapsingHeader("Textures")) {
        ImGui::Text("Current Textures");
        if(ImGui::ListBox("##CurrentTextures", &selectedTexture, PipelineExtension::getNameOfTexture,
                       static_cast<void *>(&this->usedTextures), this->usedTextures.size(), 10)) {
            //in case the selected texture is changed
            if(selectedTexture != -1) {
                //find the selected texture
                const char* selectedName;
                PipelineExtension::getNameOfTexture(static_cast<void *>(&this->usedTextures), selectedTexture, &selectedName);
                std::string selectedNameString(selectedName);
                std::map<std::string, std::shared_ptr<Texture>>::iterator it = usedTextures.find(selectedNameString);
                if(it == usedTextures.end()) {
                    std::cerr << "Selected texture not found. This seems like an error." << std::endl;
                } else {
                    //found texture case
                    this->currentTextureInfo = it->second->getTextureInfo();
                    //now set the ImGui char buffers with strings
                    strncpy(this->tempName,currentTextureInfo.name.c_str(), sizeof(tempName)/ sizeof(tempName[0]));
                    strncpy(this->tempHeightOption,currentTextureInfo.heightOption.c_str(), sizeof(tempHeightOption)/ sizeof(tempHeightOption[0]));
                    strncpy(this->tempWidthOption,currentTextureInfo.widthOption.c_str(), sizeof(tempWidthOption)/ sizeof(tempWidthOption[0]));
                }
            }
        }
        if(selectedTexture== -1) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            ImGui::Button("View Texture");
            ImGui::PopStyleVar();

        } else {
            if (ImGui::Button("View Texture")) {
                ImGui::OpenPopup("create_texture_popup");
            }
        }
        if(ImGui::Button("Create Texture")) {
            currentTextureInfo = Texture::TextureInfo();
            //now set the ImGui char buffers with strings
            strncpy(this->tempName,currentTextureInfo.name.c_str(), sizeof(tempName)/ sizeof(tempName[0]));
            strncpy(this->tempHeightOption,currentTextureInfo.heightOption.c_str(), sizeof(tempHeightOption)/ sizeof(tempHeightOption[0]));
            strncpy(this->tempWidthOption,currentTextureInfo.widthOption.c_str(), sizeof(tempWidthOption)/ sizeof(tempWidthOption[0]));
            selectedTexture = -1;
            ImGui::OpenPopup("create_texture_popup");
        }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    if (ImGui::BeginPopup("create_texture_popup")) {
        drawTextureSettings();
    }
    if(!isNodeGraphValid()) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        ImGui::Button("Build Pipeline");
        ImGui::PopStyleVar();
    } else {
        static bool showVerify = false;
        if (ImGui::Button("Build Pipeline")) {
            orderedStages.clear();
            if(!buildRenderPipelineStages(nodes, orderedStages)) {
                addError("Build failed");
            } else {
                showVerify = true;
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
            }
        }
        if(showVerify) {
            ImGui::OpenPopup("Verify");
            if (ImGui::BeginPopupModal("Verify", nullptr, ImGuiWindowFlags_Modal)) {
                ImGui::Text("Limon engine builds the pipeline based on render method priorities and dependencies.");
                ImGui::Text("\n");
                ImGui::Text("Since it can't detect run time behaviour, this order might not be optimal for your case.");
                ImGui::Text("You can reorder this list based on your knowledge of the intentions of each shader.");
                ImGui::Text("Please note, because it is possible to intentionally reverse dependency order to use ");
                ImGui::Text("previous frame results, your reorders are not verified for dependencies or any other sanity checks.");
                ImGui::Text("\n");
                std::vector<std::string> tempStringList;
                std::string tempString;

                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec4 backgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);

                backgroundColor.x +=0.5f;
                backgroundColor.y +=0.5f;
                backgroundColor.z +=0.5f;
                ImU32 newColor = ImGui::ColorConvertFloat4ToU32(backgroundColor);
                size_t fromNode, toNode;
                for (size_t n = 0; n < orderedStages.size(); n++) {
                    tempStringList.clear();
                    std::for_each(orderedStages[n].first.begin(), orderedStages[n].first.end(), [&tempStringList](const Node* node) {tempStringList.emplace_back(node->getDisplayName());});
                    tempString = StringUtils::join(tempStringList, ",");
                    draw_list->ChannelsSplit(2);
                    draw_list->ChannelsSetCurrent(1);

                    ImGui::Selectable(tempString.c_str());

                    draw_list->ChannelsSetCurrent(0);
                    ImVec2 p_min = ImGui::GetItemRectMin();
                    ImVec2 p_max = ImGui::GetItemRectMax();
                    ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, newColor);
                    draw_list->ChannelsMerge();

                    if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
                    {
                        fromNode = n;
                        toNode = n + (ImGui::GetMouseDragDelta(0).y < 0.f ? -1 : 1);
                    }
                }
                if (toNode > 0 && toNode < orderedStages.size())
                {
                    //we need to re order the ordered stages now
                    auto backup = orderedStages[fromNode];
                    orderedStages[fromNode] = orderedStages[toNode];
                    orderedStages[toNode] = backup;
                    ImGui::ResetMouseDragDelta();
                }
                ImGui::InputText("File name:##fileNameToSavePipeline", tempFileName, sizeof (tempFileName)/sizeof (tempFileName[0], ImGuiInputTextFlags_CharsNoBlank));
                ImGui::SameLine();
                if(ImGui::Button("Build with this order")) {
                    std::shared_ptr<GraphicsPipeline> builtPipelineNew = combineStagesToPipeline();
                    builtPipeline = builtPipelineNew;//old one is auto removed
                    builtPipeline->serialize(tempFileName, options);
                    showVerify = false;
                }
                if(ImGui::Button("Cancel##BuildingPipelineCance")){
                    showVerify = false;
                    //we don't actually need to do anything in this case
                }

                ImGui::EndPopup();
            }
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

void PipelineExtension::drawTextureSettings() {

    ImGui::Text("Texture type:");
    if(ImGui::RadioButton("2D##Texture_type_PipelineExtension", currentTextureInfo.textureType == GraphicsInterface::TextureTypes::T2D)) { currentTextureInfo.textureType = GraphicsInterface::TextureTypes::T2D; }
    ImGui::SameLine();
    if(ImGui::RadioButton("2D Array##Texture_type_PipelineExtension", currentTextureInfo.textureType == GraphicsInterface::TextureTypes::T2D_ARRAY)) { currentTextureInfo.textureType = GraphicsInterface::TextureTypes::T2D_ARRAY; }
    ImGui::SameLine();
    if(ImGui::RadioButton("Cubemap##Texture_type_PipelineExtension", currentTextureInfo.textureType == GraphicsInterface::TextureTypes::TCUBE_MAP)) { currentTextureInfo.textureType = GraphicsInterface::TextureTypes::TCUBE_MAP; }
    ImGui::SameLine();
    if(ImGui::RadioButton("Cubemap Array##Texture_type_PipelineExtension", currentTextureInfo.textureType == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY)) { currentTextureInfo.textureType = GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY; }


    ImGui::Text("Internal Format type:");
    if(ImGui::RadioButton("RED##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::RED)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::RED; }
    ImGui::SameLine();
    if(ImGui::RadioButton("R32F##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::R32F)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::R32F; }
    ImGui::SameLine();
    if(ImGui::RadioButton("RGB##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::RGB)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::RGB; }
    ImGui::SameLine();
    if(ImGui::RadioButton("RGBA##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::RGBA)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::RGBA; }
    ImGui::SameLine();
    if(ImGui::RadioButton("RGB16F##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::RGB16F)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::RGB16F; }
    ImGui::SameLine();
    if(ImGui::RadioButton("RGB32F##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::RGB32F)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::RGB32F; }
    ImGui::SameLine();
    if(ImGui::RadioButton("DEPTH##internalFormat_type_PipelineExtension", currentTextureInfo.internalFormatType == GraphicsInterface::InternalFormatTypes::DEPTH)) { currentTextureInfo.internalFormatType = GraphicsInterface::InternalFormatTypes::DEPTH; }


    ImGui::Text("Format type:");
    if(ImGui::RadioButton("RED##format_type_PipelineExtension", currentTextureInfo.formatType == GraphicsInterface::FormatTypes::RED)) { currentTextureInfo.formatType = GraphicsInterface::FormatTypes::RED; }
    ImGui::SameLine();
    if(ImGui::RadioButton("RGB##format_type_PipelineExtension", currentTextureInfo.formatType == GraphicsInterface::FormatTypes::RGB)) { currentTextureInfo.formatType = GraphicsInterface::FormatTypes::RGB; }
    ImGui::SameLine();
    if(ImGui::RadioButton("RGBA##format_type_PipelineExtension", currentTextureInfo.formatType == GraphicsInterface::FormatTypes::RGBA)) { currentTextureInfo.formatType = GraphicsInterface::FormatTypes::RGBA; }
    ImGui::SameLine();
    if(ImGui::RadioButton("DEPTH##format_type_PipelineExtension", currentTextureInfo.formatType == GraphicsInterface::FormatTypes::DEPTH)) { currentTextureInfo.formatType = GraphicsInterface::FormatTypes::DEPTH; }


    ImGui::Text("Data type:");
    if(ImGui::RadioButton("UNSIGNED_BYTE##data_type_PipelineExtension", currentTextureInfo.dataType == GraphicsInterface::DataTypes::UNSIGNED_BYTE)) { currentTextureInfo.dataType = GraphicsInterface::DataTypes::UNSIGNED_BYTE; }
    ImGui::SameLine();
    if(ImGui::RadioButton("UNSIGNED_SHORT##data_type_PipelineExtension", currentTextureInfo.dataType == GraphicsInterface::DataTypes::UNSIGNED_SHORT)) { currentTextureInfo.dataType = GraphicsInterface::DataTypes::FLOAT; }
    ImGui::SameLine();
    if(ImGui::RadioButton("UNSIGNED_INT##data_type_PipelineExtension", currentTextureInfo.dataType == GraphicsInterface::DataTypes::UNSIGNED_INT)) { currentTextureInfo.dataType = GraphicsInterface::DataTypes::FLOAT; }
    ImGui::SameLine();
    if(ImGui::RadioButton("FLOAT##data_type_PipelineExtension", currentTextureInfo.dataType == GraphicsInterface::DataTypes::FLOAT)) { currentTextureInfo.dataType = GraphicsInterface::DataTypes::FLOAT; }


    ImGui::Text("Wrap Mode S:");
    if(ImGui::RadioButton("NONE##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeS == GraphicsInterface::TextureWrapModes::NONE)) { currentTextureInfo.textureWrapModeS = GraphicsInterface::TextureWrapModes::NONE; }
    ImGui::SameLine();
    if(ImGui::RadioButton("REPEAT##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeS == GraphicsInterface::TextureWrapModes::REPEAT)) { currentTextureInfo.textureWrapModeS = GraphicsInterface::TextureWrapModes::REPEAT; }
    ImGui::SameLine();
    if(ImGui::RadioButton("BORDER##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeS == GraphicsInterface::TextureWrapModes::BORDER)) { currentTextureInfo.textureWrapModeS = GraphicsInterface::TextureWrapModes::BORDER; }
    ImGui::SameLine();
    if(ImGui::RadioButton("EDGE##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeS == GraphicsInterface::TextureWrapModes::EDGE)) { currentTextureInfo.textureWrapModeS = GraphicsInterface::TextureWrapModes::EDGE; }

    ImGui::Text("Wrap Mode T:");
    if(ImGui::RadioButton("NONE##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeT == GraphicsInterface::TextureWrapModes::NONE)) { currentTextureInfo.textureWrapModeT = GraphicsInterface::TextureWrapModes::NONE; }
    ImGui::SameLine();
    if(ImGui::RadioButton("REPEAT##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeT == GraphicsInterface::TextureWrapModes::REPEAT)) { currentTextureInfo.textureWrapModeT = GraphicsInterface::TextureWrapModes::REPEAT; }
    ImGui::SameLine();
    if(ImGui::RadioButton("BORDER##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeT == GraphicsInterface::TextureWrapModes::BORDER)) { currentTextureInfo.textureWrapModeT = GraphicsInterface::TextureWrapModes::BORDER; }
    ImGui::SameLine();
    if(ImGui::RadioButton("EDGE##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeT == GraphicsInterface::TextureWrapModes::EDGE)) { currentTextureInfo.textureWrapModeT = GraphicsInterface::TextureWrapModes::EDGE; }

    ImGui::Text("Wrap Mode R:");
    if(ImGui::RadioButton("NONE##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeR == GraphicsInterface::TextureWrapModes::NONE)) { currentTextureInfo.textureWrapModeR = GraphicsInterface::TextureWrapModes::NONE; }
    ImGui::SameLine();
    if(ImGui::RadioButton("REPEAT##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeR == GraphicsInterface::TextureWrapModes::REPEAT)) { currentTextureInfo.textureWrapModeR = GraphicsInterface::TextureWrapModes::REPEAT; }
    ImGui::SameLine();
    if(ImGui::RadioButton("BORDER##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeR == GraphicsInterface::TextureWrapModes::BORDER)) { currentTextureInfo.textureWrapModeR = GraphicsInterface::TextureWrapModes::BORDER; }
    ImGui::SameLine();
    if(ImGui::RadioButton("EDGE##wrap_mode_PipelineExtension", currentTextureInfo.textureWrapModeR == GraphicsInterface::TextureWrapModes::EDGE)) { currentTextureInfo.textureWrapModeR = GraphicsInterface::TextureWrapModes::EDGE; }

    ImGui::Text("Filter Mode:");
    if(ImGui::RadioButton("NEAREST##wrap_mode_PipelineExtension", currentTextureInfo.filterMode == GraphicsInterface::FilterModes::NEAREST)) { currentTextureInfo.filterMode = GraphicsInterface::FilterModes::NEAREST; }
    ImGui::SameLine();
    if(ImGui::RadioButton("LINEAR##wrap_mode_PipelineExtension", currentTextureInfo.filterMode == GraphicsInterface::FilterModes::LINEAR)) { currentTextureInfo.filterMode = GraphicsInterface::FilterModes::LINEAR; }
    ImGui::SameLine();
    if(ImGui::RadioButton("TRILINEAR##wrap_mode_PipelineExtension", currentTextureInfo.filterMode == GraphicsInterface::FilterModes::TRILINEAR)) { currentTextureInfo.filterMode = GraphicsInterface::FilterModes::TRILINEAR; }


    if(ImGui::InputInt2("Size##texture_size_PipelineExtension", currentTextureInfo.defaultSize)) {
        for (int i = 0; i < 2; ++i) {
            if (currentTextureInfo.defaultSize[i] < 1) {
                currentTextureInfo.defaultSize[i] = 1;
            }
            if (currentTextureInfo.defaultSize[i] > 8192) {
                currentTextureInfo.defaultSize[i] = 8192;
            }
        }
    }

    if(currentTextureInfo.textureType == GraphicsInterface::TextureTypes::T2D_ARRAY || currentTextureInfo.textureType == GraphicsInterface::TextureTypes::TCUBE_MAP_ARRAY) {
        if(ImGui::InputInt("Depth##texture_depth_PipelineExtension", &currentTextureInfo.depth)) {
            if(currentTextureInfo.depth < 0 ) {
                currentTextureInfo.depth = 0;
            }
            if(currentTextureInfo.depth > 1024) {
                currentTextureInfo.depth = 1024;
            }
        }
    }

    ImGui::InputText("##texture_Height_Option_PipelineExtension",tempHeightOption, sizeof(tempHeightOption)-1, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    ImGui::Text("TextureHeightOption");
    ImGui::InputText("##texture_Width_Option_PipelineExtension",tempWidthOption, sizeof(tempWidthOption)-1, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    ImGui::Text("TextureWidthOption");

    if(ImGui::InputFloat4("Border Color##texture_borderColor_PipelineExtension", currentTextureInfo.borderColor)) {
        for (int i = 0; i < 4; ++i) {
            if (currentTextureInfo.borderColor[i] < 0) {
                currentTextureInfo.borderColor[i] = 0;
            }
            if (currentTextureInfo.borderColor[i] > 1.0) {
                currentTextureInfo.borderColor[i] = 1;
            }
        }
    }
    ImGui::InputText("##texture_name_PipelineExtension",tempName, sizeof(tempName)-1, ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    if(currentTextureInfo.name.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::Text("TextureName");
        ImGui::PopStyleColor();
    } else {
        ImGui::Text("TextureName");
    }
    if(selectedTexture == -1 ) {
        if (ImGui::Button("Create Texture##create_button_PipelineExtension")) {
            if (strnlen(tempName, sizeof(tempName) / sizeof tempName[0]) != 0) {
                currentTextureInfo.name = std::string(tempName);
                currentTextureInfo.heightOption = std::string(tempHeightOption);
                currentTextureInfo.widthOption = std::string(tempWidthOption);
                std::shared_ptr<Texture> texture = std::make_shared<Texture>(graphicsWrapper, currentTextureInfo);
                usedTextures[currentTextureInfo.name] = texture;
                memset(tempName, 0, sizeof(currentTextureInfo.name));
                currentTextureInfo = Texture::TextureInfo();
                ImGui::CloseCurrentPopup();
            }
        }
    }
    ImGui::EndPopup();
}

bool PipelineExtension::buildRenderPipelineStages(const std::vector<const Node *> &nodes, std::vector<
        std::pair<std::set<const Node *>, std::shared_ptr<GraphicsPipeline::StageInfo>>> & orderedStages) {
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
        return false;
    } else {
        std::unordered_map<const Node*, std::set<const Node*>> dependencies;
        buildDependencyInfoRecursive(rootNode, dependencies);
        for(const auto& node:dependencies) {
            std::cerr << "Node: " <<  node.first->getName() << std::endl;
            for(auto dependency: node.second) {
                std::cerr << "\t\tfor depends: " <<  dependency->getName() << std::endl;
            }
        }
        //first element of the pair is what is required(dependencies, recursively filled, contains indirect), second is the group that can be rendered with them
        std::vector<std::pair<std::set<const Node*>, std::set<const Node*>>> dependencyGroups = buildGroupsByDependency(dependencies);
        std::map<const Node*, std::shared_ptr<GraphicsPipeline::StageInfo>> nodeStages;
        std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node*>> builtStages;//A stage can contain more than one node, so the nodes used to build it is also here.
        if(buildRenderPipelineRecursive(rootNode, renderMethods, nodeStages, dependencyGroups, builtStages)) {
            //we have dependency info, and the stage info. Stage info contains highest priority. Order based on that
            for(const auto& builtStageInfo:builtStages) {
                // Build stages are individual, but they have dependencies, and if a high priority node needs a low priority node, low priority should become high priority.
                // Also if a high priority node is rendered to frame buffer but not finished processing because a stage after is low priority, that one should get updated too
                // To figure this chain, we need to track them back to forward.
                recursiveUpdatePriorityForDependencies(dependencyGroups, builtStages, builtStageInfo);
                recursiveUpdatePriorityForDependents(dependencyGroups, builtStages,builtStageInfo);
            }
            //now move the elements to ordered
            while(!builtStages.empty()) {
                std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node*>>::iterator builtStageInfo = builtStages.begin();
                //Special case, first one always gets inserted
                if(orderedStages.empty()) {
                    orderedStages.emplace_back(builtStageInfo->second, builtStageInfo->first);
                    builtStages.erase(builtStageInfo);
                    continue;
                }
                //a stage can only be inserted in between things it is depending on, and things that depend on it.
                size_t maxDependencyIndex = 0; //last entry that this node is depending on
                size_t minDependentIndex = 999; //first entry that this node is depending on

                //lets find which dependency group this stage  was in.
                for (const auto &dependencyGroup: dependencyGroups) {
                    if (dependencyGroup.second.find(*(builtStageInfo->second.begin())) != dependencyGroup.second.end()) {
                        //this is the dependency group this stage belongs to, any stage that is required needs to be before
                        for (const auto &dependency: dependencyGroup.first) {
                            for (size_t i = 0; i < orderedStages.size(); ++i) {
                                if (orderedStages[i].first.find(dependency) != orderedStages[i].first.end()) {
                                    maxDependencyIndex = i;
                                }
                            }
                        }
                    }
                }
                //we found what we are depending on in the current state. Lets find what depends on us
                std::set<const Node*> dependentNodes;
                for (const auto &dependencyGroup: dependencyGroups) {
                    if (dependencyGroup.first.find(*(builtStageInfo->second.begin())) != dependencyGroup.first.end()) {
                        //This dependency group was depending on us. collect all nodes
                        dependentNodes.insert(dependencyGroup.second.begin(), dependencyGroup.second.end());
                    }
                }
                std::vector<const Node*> tempNodes;
                for (size_t i = 0; i < orderedStages.size(); ++i) {
                    std::set_intersection(dependentNodes.begin(), dependentNodes.end(), orderedStages[i].first.begin(), orderedStages[i].first.end(), std::back_inserter(tempNodes));
                    if(!tempNodes.empty()) {
                        minDependentIndex = i;
                        break;//since we are iterating forward, finding once is enough.
                    }
                }
                //now we need to insert this new stage in order of its priority, after the last dependency index
                bool inserted = false;
                size_t itStart = maxDependencyIndex == 0 ? 0 : maxDependencyIndex;
                size_t itEnd = minDependentIndex == 999 ? orderedStages.size() : minDependentIndex;
                for (size_t i = itStart; i < itEnd; ++i) {
                    uint32_t minPriorityOfStage = 999;
                    minPriorityOfStage = std::min(orderedStages[i].second->getHighestPriority(), minPriorityOfStage);
                    if(minPriorityOfStage > builtStageInfo->first->getHighestPriority()) {
                        //insert here
                        orderedStages.insert(orderedStages.begin() + i, {builtStageInfo->second, builtStageInfo->first});
                        inserted = true;
                        break;
                    }
                }
                if(!inserted) {
                    //so we are the last
                    orderedStages.insert(orderedStages.begin() + itEnd, {builtStageInfo->second, builtStageInfo->first});
                }
                builtStages.erase(builtStageInfo);
            }
        }//error message provided by recursive
        return true;
    }
}

void PipelineExtension::recursiveUpdatePriorityForDependencies(std::vector<std::pair<std::set<const Node *>, std::set<const Node *>>> &dependencyGroups,
                                                               std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStages,
                                                               const std::pair<const std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStageInfo) const {
    for (const auto &stageNode: builtStageInfo.second) {
        //a build stage might be combination of multiple nodes, for each of those nodes, update the priority of its dependencies
        for (const auto &dependencyGroup: dependencyGroups) {
            if (dependencyGroup.second.find(stageNode) != dependencyGroup.second.end()) {
                //this is the dependency group, and all dependencies are at the first entry of the pair
                for (const auto &nodeDependency: dependencyGroup.first) {
                    //search for them in the builtStages to update the requirement
                    for (const auto &builtStage2: builtStages) {
                        if (builtStage2.second.find(nodeDependency) != builtStage2.second.end()) {
                            //this stage is a dependency for the one we were iterating over, update the priority
                            builtStage2.first->setHighestPriority(std::min(builtStage2.first->getHighestPriority(), builtStageInfo.first->getHighestPriority()));
                            //what about the nodes that were depending on this stage? we should update them too
                            recursiveUpdatePriorityForDependencies(dependencyGroups, builtStages, builtStage2);
                        }
                    }
                }
            }
        }
    }
}

void PipelineExtension::recursiveUpdatePriorityForDependents(std::vector<std::pair<std::set<const Node *>, std::set<const Node *>>> &dependencyGroups,
                                                               std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStages,
                                                               const std::pair<const std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStageInfo) const {
    for (const auto &stageNode: builtStageInfo.second) {
        //a build stage might be combination of multiple nodes, for each of those nodes, update the dependents so the output will reach the screen before others
        for (const auto &dependencyGroup: dependencyGroups) {
            if (dependencyGroup.first.find(stageNode) != dependencyGroup.first.end()) {
                //this is the dependency group, and all dependencies are at the first entry of the pair
                for (const auto &nodeDependent: dependencyGroup.second) {
                    //search for them in the builtStages to update the requirement
                    if (nodeDependent->getName() != "Screen") {
                        for (const auto &builtStage2: builtStages) {
                            if (builtStage2.second.find(nodeDependent) != builtStage2.second.end()) {
                                //this stage is a dependency for the one we were iterating over, update the priority
                                builtStage2.first->setHighestPriority(std::min(builtStage2.first->getHighestPriority(), builtStageInfo.first->getHighestPriority()));
                                //what about the nodes that are dependents  on this stage? we should update them too
                                recursiveUpdatePriorityForDependents(dependencyGroups, builtStages, builtStage2);
                            }
                        }
                    }
                }
            }
        }
    }
}

void PipelineExtension::buildDependencyInfoRecursive(const Node *node, std::unordered_map<const Node*, std::set<const Node*>>& dependencies) {
    std::set<const Node*> currentNodeDependencies;
    for(auto inputConnection:node->getInputConnections()) {
        for(auto inputNode:inputConnection->getConnectedNodes()){
            if(dependencies.find(inputNode) == dependencies.end()) {
                buildDependencyInfoRecursive(inputNode, dependencies);
            }
            currentNodeDependencies.insert(dependencies.at(inputNode).begin(), dependencies.at(inputNode).end());
            currentNodeDependencies.insert(inputNode);
        }
    }
    dependencies[node] = currentNodeDependencies;
}

bool PipelineExtension::canBeJoined(const std::set<const Node*>& existingNodes, const std::set<const Node*>& existingDependencies, const Node* currentNode, const std::set<const Node*>& currentDependencies) {
    std::string existingNodeName = "emptyList";
    if(!existingNodes.empty()) {
        existingNodeName = (*existingNodes.begin())->getDisplayName();
    }
    if(existingDependencies.find(currentNode) != existingDependencies.end()) {
        std::cerr << "Failed to join because existing set "<< existingNodeName <<"depends to current " << std::endl;
        return false;//existing nodes depend on this node
    }
    std::set<const Node*> intersection;
    std::set_intersection(currentDependencies.begin(), currentDependencies.end(), existingNodes.begin(), existingNodes.end(), std::inserter(intersection, intersection.begin()));
    if(!intersection.empty()) {
        std::cerr << "Failed to join because current node depends to existing set "<< existingNodeName <<" " << std::endl;
        return false;//current node is depending on one of the nodes. can't merge
    }
    //old and new don't depend each other.

    //now get if this node outputs any depth map, and if it does, what is it.
    auto currentStageExtension = dynamic_cast<PipelineStageExtension*>(currentNode->getExtension());
    if(currentStageExtension == nullptr) {
        //stage extension not found, can't take the chance. fail
        std::cerr << "Failed to join because current node have no extension " << std::endl;
        return false;
    }
    std::shared_ptr<Texture> newDepthMap;

    for(auto outputConnection:currentNode->getOutputConnections()) {
        auto outputTextureInfo = currentStageExtension->getOutputTextureInfo(outputConnection);
        if(outputTextureInfo == nullptr ) {
            //there is an output that is not set. We allow it if it is depth, and depth read/write are disabled
            if(outputConnection->getName() !="Depth") {
                std::cerr << "Failed to join because current node have not set output  " << std::endl;
                return false;
            }
            //now we know it is depth, but we don't know if depth read write is enabled, lets check
            currentStageExtension = dynamic_cast<PipelineStageExtension*>(currentNode->getExtension());
            if(currentStageExtension == nullptr ) {
                std::cerr << "ERROR: Limon should not have any other extension type then PipelineStageExtension!" << std::endl;
                return false;
            }
            if(currentStageExtension->isDepthTestEnabled() || currentStageExtension->isDepthWriteEnabled()) {
                std::cerr << "Failed to join because current node have not set Depth output but uses Depth." << std::endl;
                return false;
            }

        } else {
            if (outputTextureInfo->texture != nullptr && outputTextureInfo->texture->getFormat() == GraphicsInterface::FormatTypes::DEPTH) {
                newDepthMap = outputTextureInfo->texture;
                break;
            }
        }
    }

    //now find what depthmap is used by existing set. Current Depth map might be null.
    std::shared_ptr<Texture> existingDepthMap = nullptr;
    int32_t existingRenderResolution[2];
    GraphicsInterface::CullModes existingCullMode = GraphicsInterface::CullModes::NO_CHANGE;
    bool existingScissorTestState = false;
    bool existingDepthReadState = false;
    bool existingDepthWriteState = false;
    bool existingBlendingState = false;
    std::vector<std::string> existingCameraTags;
    std::vector<std::string> existingObjectTags;
    for(auto existingNode:existingNodes) {
        auto stageExtension = dynamic_cast<PipelineStageExtension*>(existingNode->getExtension());
        if(stageExtension == nullptr) {
            //stage extension not found, can't take the chance. fail
            std::cerr << "Failed to join because existing node have no extension " << std::endl;
            return false;
        }
        existingRenderResolution[0] = stageExtension->getDefaultRenderResolution()[0];
        existingRenderResolution[1] = stageExtension->getDefaultRenderResolution()[1];
        if(existingCullMode == GraphicsInterface::CullModes::NO_CHANGE) {//no change can be ignored
            existingCullMode = stageExtension->getCullmode();
        }
        existingScissorTestState = stageExtension->isScissorTestEnabled();
        existingDepthReadState = stageExtension->isDepthTestEnabled();
        existingDepthWriteState = stageExtension->isDepthWriteEnabled();
        existingBlendingState = stageExtension->isBlendEnabled();
        existingCameraTags = stageExtension->getCameraTags();
        existingObjectTags = stageExtension->getObjectTags();
        for (auto outputConnection:existingNode->getOutputConnections()) {
            auto outputTextureInfo = stageExtension->getOutputTextureInfo(outputConnection);
            if (outputTextureInfo == nullptr) {
                //there is an output that is not set. Return false
                std::cerr << "Failed to join because existing set "<< existingNodeName <<" have not set outputs" << std::endl;
                return false;
            }
            if(outputTextureInfo->texture != nullptr && outputTextureInfo->texture->getFormat() == GraphicsInterface::FormatTypes::DEPTH) {
                existingDepthMap = outputTextureInfo->texture;
                break;
            }
        }
        if(existingDepthMap != nullptr) {
            //already found the output, break
            break;
        }
    }
    if(currentStageExtension->getCameraTags().size() != existingCameraTags.size()) {
        std::cerr << "Failed to join because existing camera tags "<< StringUtils::join(existingCameraTags,",") << " is not same as " << StringUtils::join(currentStageExtension->getCameraTags(),",") << std::endl;
        return false;
    }
    if(currentStageExtension->getObjectTags().size() != existingObjectTags.size()) {
        std::cerr << "Failed to join because existing object tags "<< StringUtils::join(existingObjectTags,",") << " is not same as " << StringUtils::join(currentStageExtension->getObjectTags(),",") << std::endl;
        return false;
    }

    for (const std::string &tag: currentStageExtension->getCameraTags()) {
        if(std::find(existingCameraTags.begin(), existingCameraTags.end(),tag) == existingCameraTags.end()) {
            std::cerr << "Failed to join because existing camera tags "<< StringUtils::join(existingCameraTags,",") << " is not same as " << StringUtils::join(currentStageExtension->getCameraTags(),",") << std::endl;
            return false;
        }
    }

    for (const std::string &tag: currentStageExtension->getObjectTags()) {
        if(std::find(existingObjectTags.begin(), existingObjectTags.end(),tag) == existingObjectTags.end()) {
            std::cerr << "Failed to join because existing object tags "<< StringUtils::join(existingObjectTags,",") << " is not same as " << StringUtils::join(currentStageExtension->getObjectTags(),",") << std::endl;
            return false;
        }
    }

    /* left for debug */
    if(existingDepthMap == nullptr) {
        std::cerr << "Success to join because existing set "<< existingNodeName <<" has no depth map" << std::endl;
    }
    if(newDepthMap == nullptr) {
        std::cerr << "Success to join because current node "<< currentNode->getDisplayName() <<" has no depth map" << std::endl;
    }
    if(existingDepthMap == newDepthMap) {
        std::cerr << "Success to join because current node and existing "<< existingNodeName <<" has same depth map" << std::endl;
    }
    /* left for debug */
    if(existingDepthMap != nullptr && newDepthMap != nullptr && existingDepthMap != newDepthMap) {
        std::cerr << "Failed to join because existing set " << existingNodeName << " have other depth map" << std::endl;
        return false;
    }
    if(existingRenderResolution[0] != currentStageExtension->getDefaultRenderResolution()[0] ||
            existingRenderResolution[1] != currentStageExtension->getDefaultRenderResolution()[1] ) {
        std::cerr << "Failed because Render Resolution is different" << std::endl;
        return false;
    }

    //they should have the same cull mode for joining.
    if(existingCullMode != GraphicsInterface::CullModes::NO_CHANGE &&
        currentStageExtension->getCullmode() != GraphicsInterface::CullModes::NO_CHANGE &&
        existingCullMode != currentStageExtension->getCullmode()) {
        std::cerr << "Failed because Cull Mode is different" << std::endl;
        return false;
    }

    if(existingScissorTestState != currentStageExtension->isScissorTestEnabled()) {
        std::cerr << "Failed because Scissor Mode is different" << std::endl;
        return false;
    }
    if(existingDepthReadState != currentStageExtension->isDepthTestEnabled()) {
        std::cerr << "Failed because DepthReadState is different" << std::endl;
        return false;
    }
    if(existingDepthWriteState != currentStageExtension->isDepthWriteEnabled()) {
        std::cerr << "Failed because DepthWriteState is different" << std::endl;
        return false;
    }
    if(existingDepthWriteState != currentStageExtension->isDepthWriteEnabled()) {
        std::cerr << "Failed because DepthWriteState is different" << std::endl;
        return false;
    }
    if(existingBlendingState != currentStageExtension->isBlendEnabled()) {
        std::cerr << "Failed because BlendState is different" << std::endl;
        return false;
    }


    return true;
}

/**
 * groups node so they can be rendered in without expensive GPU state changes. Rules:
 *
 * 1) they can't be outputting different depth maps
 * 2) they can't be depending one another, neither directly or indirectly
 *
 * This method might take some time to finish
 *
 */
std::vector<std::pair<std::set<const Node*>, std::set<const Node*>>> PipelineExtension::buildGroupsByDependency(std::unordered_map<const Node*, std::set<const Node*>> dependencies) {
    /**
     * result is an array of dependency[],node[]
     *
     */
    std::vector<std::pair<std::set<const Node*>, std::set<const Node*>>> resultGroup;//dependencies, nodes
    uint32_t selectingDependencySize = 0;
    while(true) {
        std::set<const Node*> nodesToRemove;
        std::cerr << "searching for dependency size" << selectingDependencySize << " while waiting dependency size " << dependencies.size() << std::endl;
        for (auto nodeDependencyIt = dependencies.begin(); nodeDependencyIt != dependencies.end();) {
            bool nodeMerged = false;
            if(nodeDependencyIt->second.size() == selectingDependencySize) {
                std::cerr << "processing node " << nodeDependencyIt->first->getDisplayName() << std::endl;
                //these are the ones we want to try to join
                for (auto resultGroupIt = resultGroup.begin(); resultGroupIt != resultGroup.end(); ++resultGroupIt) {
                    //searching for a result that can be merged
                    if(canBeJoined(resultGroupIt->second, resultGroupIt->first, nodeDependencyIt->first, nodeDependencyIt->second)) {
                        std::cerr << "Joining " << nodeDependencyIt->first->getDisplayName() << " with: ";
                        for(auto oldNodes:resultGroupIt->second) {
                            std::cerr <<  oldNodes->getDisplayName() << ", ";
                        }
                        std::cerr << std::endl;
                        std::set<const Node*> joinedDependencies;
                        joinedDependencies.insert(resultGroupIt->first.begin(), resultGroupIt->first.end());
                        joinedDependencies.insert(nodeDependencyIt->second.begin(), nodeDependencyIt->second.end());
                        std::set<const Node*> joinedNodes;
                        joinedNodes.insert(resultGroupIt->second.begin(), resultGroupIt->second.end());
                        joinedNodes.insert(nodeDependencyIt->first);
                        resultGroup.erase(resultGroupIt);
                        resultGroup.emplace_back(std::make_pair(joinedDependencies,joinedNodes));
                        nodeDependencyIt = dependencies.erase(nodeDependencyIt);
                        nodeMerged = true;
                        break;
                    }
                }

                if(!nodeMerged) {
                    std::cerr << "Adding itself " << nodeDependencyIt->first->getDisplayName() << std::endl;
                    std::set<const Node*> tempDependencies;
                    tempDependencies.insert(nodeDependencyIt->second.begin(), nodeDependencyIt->second.end());
                    std::set<const Node*> tempNodes;
                    tempNodes.insert(nodeDependencyIt->first);
                    resultGroup.emplace_back(std::make_pair(tempDependencies, tempNodes));
                    nodeDependencyIt = dependencies.erase(nodeDependencyIt);
                }
                if(nodeDependencyIt == dependencies.end()) {
                    break;//if we removed elements, it is possible we are already in the end
                }
            } else {
                ++nodeDependencyIt;
            }
        }
        if(dependencies.empty()) {
            break;
        }
        if(selectingDependencySize > 30) {
            break;
        }
        selectingDependencySize++;
    }
    for(auto result: resultGroup) {
        std::cerr << "Nodes: ";
        for(auto node:result.second) {
            std::cerr << node->getDisplayName() << ", ";
        }
        std::cerr << "\n\t\t depends: ";
        for(auto depends:result.first) {
            std::cerr << depends->getDisplayName() << ", ";
        }
        std::cerr << std::endl;
    }
    return resultGroup;
}


bool PipelineExtension::buildRenderPipelineRecursive(const Node *node,
                                                     RenderMethods &renderMethods,
                                                     std::map<const Node*, std::shared_ptr<GraphicsPipeline::StageInfo>>& nodeStages,
                                                     const std::vector<std::pair<std::set<const Node*>, std::set<const Node*>>>& groupsByDependency,
                                                     std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStages) {

    if(nodeStages.find(node) != nodeStages.end()) {
        return true;
    }
    for(const Connection* connection:node->getInputConnections()) {
        //each connection can also have multiple inputs.
        for(Connection* connectionInputs:connection->getInputConnections()) {
            Node *inputNode = connectionInputs->getParent();
            if(nodeStages.find(inputNode) == nodeStages.end()) {
                if(!buildRenderPipelineRecursive(inputNode, renderMethods, nodeStages, groupsByDependency, builtStages)) {
                    return false;//if failed in stack, fail.
                }
            }
        }
    }

    //after all inputs are put in graphics pipeline, or no input case
    auto stageExtension = dynamic_cast<PipelineStageExtension*>(node->getExtension());

    if(node->getExtension() != nullptr && stageExtension == nullptr) {
        std::cerr << " extension type of node [" << node->getDisplayName() << "] is " << node->getName() << std::endl;
    }

    if(stageExtension != nullptr) {
        bool toScreen = false;
        if (!node->getOutputConnections().empty()) {
            for (auto connection:node->getOutputConnections()) {
                for (auto connectedNodes:connection->getConnectedNodes()) {
                    if (connectedNodes->getName() == "Screen") {
                        toScreen = true;
                        break;
                    }
                }
                if (toScreen) {
                    break;
                }
            }
        }
        std::shared_ptr<GraphicsProgram> stageProgram;
        if (stageExtension->getProgramNameInfo().geometryShaderName.empty()) {
            stageProgram = std::make_shared<GraphicsProgram>(assetManager.get(),
                                                             stageExtension->getProgramNameInfo().vertexShaderName,
                                                             stageExtension->getProgramNameInfo().fragmentShaderName,
                                                             true);//FIXME: is material required should be part of program info
        } else {
            stageProgram = std::make_shared<GraphicsProgram>(assetManager.get(),
                                                             stageExtension->getProgramNameInfo().vertexShaderName,
                                                             stageExtension->getProgramNameInfo().geometryShaderName,
                                                             stageExtension->getProgramNameInfo().fragmentShaderName,
                                                             true);//FIXME: is material required should be part of program info
        }

        std::shared_ptr<GraphicsPipeline::StageInfo> stageInfo;
        stageInfo = findSharedStage(node, nodeStages, groupsByDependency);
        if (stageInfo == nullptr) {
            stageInfo = std::make_shared<GraphicsPipeline::StageInfo>();
            stageInfo->clear = stageExtension->isClearBefore();
            stageInfo->stage = std::make_shared<GraphicsPipelineStage>(graphicsWrapper,
                                                                       stageExtension->getDefaultRenderResolution()[0],
                                                                       stageExtension->getDefaultRenderResolution()[1],
                                                               stageExtension->getRenderWidthOption(),
                                                               stageExtension->getRenderHeightOption(),
                                                               stageExtension->isBlendEnabled(),
                                                               stageExtension->isDepthTestEnabled(),
                                                               stageExtension->isDepthWriteEnabled(),
                                                               stageExtension->isScissorTestEnabled(),
                                                               toScreen);
            stageInfo->renderTags = stageExtension->getObjectTags();
            stageInfo->stage->setObjectTags(stageExtension->getObjectTags());

            stageInfo->cameraTags = stageExtension->getCameraTags();
            stageInfo->stage->setCameraTags(stageExtension->getCameraTags());
            stageInfo->stage->setCullMode(stageExtension->getCullmode());
            std::set<const Node*> nodeList{node};
            builtStages[stageInfo] = nodeList;//only add the stage at the first time. Because this method works from back to front, the order in the vector will be correct.
        } else {
            builtStages[stageInfo].insert(node);
            stageInfo->clear = stageInfo->clear || stageExtension->isClearBefore();
            stageInfo->stage->setBlendEnabled(stageInfo->stage->isBlendEnabled() || stageExtension->isBlendEnabled());
            stageInfo->stage->setDepthTestEnabled(stageInfo->stage->isDepthTestEnabled() || stageExtension->isDepthTestEnabled());
            stageInfo->stage->setScissorEnabled(stageInfo->stage->isScissorEnabled() || stageExtension->isScissorTestEnabled());
            if(stageInfo->stage->getCullMode() == GraphicsInterface::CullModes::NO_CHANGE) {//no change can be ignored. Otherwise they are guaranteed to have the same by canJoin
                stageInfo->stage->setCullMode(stageExtension->getCullmode());
            }
        }

        uint32_t location = stageInfo->stage->getLastPresetIndex();
        for (const Connection *connection:node->getInputConnections()) { //connect the inputs to current stage, since all of them now have a Stage build.
            std::shared_ptr<Texture> inputTexture = nullptr;
            for (Connection *inputConnection:connection->getInputConnections()) {

                Node *inputNode = inputConnection->getParent();
                if (inputNode->getExtension() != nullptr) {
                    //TODO: Extension should force all connections to use the same texture.
                    PipelineStageExtension *inputNodeExtension = dynamic_cast<PipelineStageExtension *>(inputNode->getExtension());
                    if (inputTexture == nullptr) {
                        inputTexture = inputNodeExtension->getOutputTexture(inputConnection);
                    } else {
                        if (inputTexture->getTextureID() !=
                            inputNodeExtension->getOutputTexture(inputConnection)->getTextureID()) {
                            std::cerr << "Different textures are set for same connection. This is illegal."
                                      << std::endl;
                        }
                    }
                } else {
                    std::cerr << "Input node extension is not PipelineStageExtension, this is not handled!"
                              << std::endl;
                }
            }
            if (inputTexture != nullptr) {
                auto stageProgramUniforms = stageProgram->getUniformMap();
                if (stageProgramUniforms.find(connection->getName()) != stageProgramUniforms.end()) {
                    //FIXME these should not be hard coded, but they are because of missing material editor.
                    if (connection->getName() == "pre_shadowDirectional") {
                        stageInfo->stage->setInput(graphicsWrapper->getMaxTextureImageUnits() - 1, inputTexture);
                        stageProgram->addPresetValue(connection->getName(),
                                                     std::to_string(graphicsWrapper->getMaxTextureImageUnits() - 1));
                    } else if (connection->getName() == "pre_shadowPoint") {
                        stageInfo->stage->setInput(graphicsWrapper->getMaxTextureImageUnits() - 2, inputTexture);
                        stageProgram->addPresetValue(connection->getName(),
                                                     std::to_string(graphicsWrapper->getMaxTextureImageUnits() - 2));
                    } else {
                        stageInfo->stage->setInput(location, inputTexture);
                        stageProgram->addPresetValue(connection->getName(), std::to_string(location));
                        location++;
                    }
                }
            }
            stageInfo->stage->setLastPresetIndex(location);
        }

        //now handle outputs
        // Directional depth map requires layer settings therefore it is not set by us.
        std::shared_ptr<Texture> depthMapDirectional = nullptr;

        for (const Connection *connection:node->getOutputConnections()) {
            auto programOutputsMap = stageProgram->getOutputMap();
            if (programOutputsMap.find(connection->getName()) != programOutputsMap.end()) {
                auto frameBufferAttachmentPoint = programOutputsMap.find(connection->getName())->second.second;
                if (stageExtension->getOutputTextureInfo(connection) == nullptr) {
                    //We add depth automatically to all programs, but it is possible that depth is not written. We can check it with the flag
                    if(connection->getName() == "Depth" && !stageExtension->isDepthWriteEnabled() && !stageExtension->isDepthTestEnabled()) {
                        continue;
                    }
                    addError("Output [" + connection->getName() + "] of node " + node->getName() + " is not set.");
                    return false;
                }
                if (stageExtension->getOutputTextureInfo(connection)->name != "Screen" &&
                    stageExtension->getOutputTextureInfo(connection)->name !=
                    "Screen Depth") {//for screen we don't need to attach anything
                    if (stageExtension->getOutputTexture(connection)->getFormat() ==
                        GraphicsInterface::FormatTypes::DEPTH &&
                        stageExtension->getOutputTexture(connection)->getType() ==
                        GraphicsInterface::TextureTypes::T2D_ARRAY) {
                        depthMapDirectional = stageExtension->getOutputTexture(connection);
                    }
                    // at this point, we have a problem. All programs have depth output mapped, because it is not possible at program level whether it should or not.
                    // but for some programs, it should not be, and it should be skipped.
                    if (frameBufferAttachmentPoint == GraphicsInterface::FrameBufferAttachPoints::DEPTH &&
                            (!stageInfo->stage->isDepthWriteEnabled() && !stageInfo->stage->isDepthTestEnabled())) {
                        //means depth is not used, don't set it
                        continue;
                    }
                    stageInfo->stage->setOutput(frameBufferAttachmentPoint, stageExtension->getOutputTexture(connection));

                }
            }
        }
        if(stageExtension->getMethodName() == "All directional shadows") {
            RenderMethods::RenderMethod functionToCall = renderMethods.getRenderMethodAllDirectionalLights(stageInfo->stage, depthMapDirectional, stageProgram, options);
            stageInfo->addRenderMethod(functionToCall);
        } else if(stageExtension->getMethodName() == "All point shadows") {
            RenderMethods::RenderMethod functionToCall = renderMethods.getRenderMethodAllPointLights(stageProgram);
            stageInfo->addRenderMethod(functionToCall);
        } else {
            bool isFound = true;
            RenderMethods::RenderMethod functionToCall = renderMethods.getRenderMethod(
                    graphicsWrapper, stageExtension->getMethodName(), stageProgram, isFound);
            if(isFound) {
                functionToCall.setCameraName(StringUtils::join(stageExtension->getCameraTags(), ","));
                stageInfo->cameraTags = stageExtension->getCameraTags();
                std::vector<HashUtil::HashedString> renderTags;
                for (const auto &item: stageExtension->getObjectTags()){
                    renderTags.emplace_back(item);
                }
                stageInfo->renderTags = stageExtension->getObjectTags();
                functionToCall.setRenderTags(renderTags);
                stageInfo->addRenderMethod(functionToCall);
            } else {
                std::cerr << "Selected method name is invalid!" << std::endl;
                addError("Selected method name in node " + node->getDisplayName()+ " is invalid!");
                return false;
            }
        }
        if(stageInfo->stage->isBlendEnabled()) {
            stageInfo->setHighestPriority(stageInfo->getHighestPriority() + 1);//if 2 stage with same priority is present, we want blending one later
        }
        nodeStages[node] = stageInfo;//contains newStage
    } else {
        if(node->getName() != "Screen") {//Screen is a special case
            std::cerr << "Extension of the node [" << node->getDisplayName() << "] is not PipelineStageExtension, this is not handled! " << std::endl;
            return false;
        }
    }
    return true;
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
    uint32_t textureSerializeID = 1;
    for(auto textureIt:usedTextures) {
        std::shared_ptr<Texture>& usedTexture = textureIt.second;
        if(usedTexture == nullptr) {
            std::cerr << "There is a texture entry with name " << textureIt.first << " without a texture, skipping" << std::endl;
            continue;
        }
        textureSerializeID++;
        if(usedTexture->getSerializeID() == 0 ) {
            usedTexture->setSerializeID(textureSerializeID);
        }
        usedTexture->serialize(document, usedTexturesElement, options);
        std::cout << "Texture entry with name " << textureIt.first << " is serialized with id " << usedTexture->getSerializeID() << std::endl;
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
        std::cout << "read texture with name [" << texture->getName() << "] and id " << texture->getSerializeID() << std::endl;
        textureElement = textureElement->NextSiblingElement("Texture");
    }
}

std::shared_ptr<GraphicsPipeline::StageInfo> PipelineExtension::findSharedStage(const Node *currentNode,
                                                                          std::map<const Node *, std::shared_ptr<GraphicsPipeline::StageInfo>> &builtStages,
                                                                          const std::vector<std::pair<std::set<const Node *>, std::set<const Node *>>> &dependencyGroups) {
    for (auto dependencyGroup:dependencyGroups) {
        if(dependencyGroup.second.find(currentNode) != dependencyGroup.second.end()) {
            //this is the group we should process, lets search for a stage that this node can share
            for(auto groupNode:dependencyGroup.second) {
                auto foundElementIt = builtStages.find(groupNode);
                if(foundElementIt != builtStages.end()) {
                    //we found a match. return the match
                    std::cerr << "found sharable node " << currentNode->getDisplayName() << " will re use " << groupNode->getDisplayName() << std::endl;
                    return foundElementIt->second;
                }
            }
        }
    }
    std::cerr << "not found sharable node " << currentNode->getDisplayName() << std::endl;
    return nullptr;
}