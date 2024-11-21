//
// Created by engin on 15.04.2019.
//

#ifndef LIMONENGINE_PIPELINEEXTENSION_H
#define LIMONENGINE_PIPELINEEXTENSION_H


#include <nodeGraph/src/EditorExtension.h>
#include "API/Graphics/GraphicsInterface.h"
#include "Graphics/GraphicsPipeline.h"

#include <functional>
#include <vector>
#include <unordered_set>

class GraphicsPipelineStage;

class PipelineExtension : public EditorExtension {

    Texture::TextureInfo currentTextureInfo;
    char tempName[256] = {0};           //
    char tempHeightOption[256] = {0};   // These 4 are used for ImGui strings.
    char tempWidthOption[256] = {0};    //
    char tempFileName[512] = {"./Data/renderPipelineBuilt.xml"};       //
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
    std::map<std::string, std::shared_ptr<Camera>> usedCameras;
    GraphicsInterface* graphicsWrapper = nullptr;
    std::shared_ptr<AssetManager> assetManager; //TODO: used for deserialize textures, maybe it would be possible to avoid.
    OptionsUtil::Options* options;//TODO: used for texture de/serialize, maybe it would be possible to avoid.
    const std::vector<std::string>& renderMethodNames;
    std::vector<std::string> messages;
    std::vector<std::string> errorMessages;
    RenderMethods renderMethods;

    std::vector<std::pair<std::set<const Node*>, std::shared_ptr<GraphicsPipeline::StageInfo>>> orderedStages; // used to keep the node order for stage so we can show it to the user
    std::shared_ptr<GraphicsPipeline> builtPipeline = nullptr;
    bool nodeGraphValid = true; //if there are nodes that are unknown, then we can't build.
    int32_t selectedTexture = -1;//-1 means it is not selected, there for we are building a new one
    int32_t selectedCamera = -1;//-1 means it is not selected, there for we are building a new one

    static bool getNameOfTexture(void* data, int index, const char** outText);
    static bool getNameOfCamera(void* data, int index, const char** outText);

    bool buildRenderPipelineRecursive(const Node *node, RenderMethods &renderMethods, std::map<const Node*, std::shared_ptr<GraphicsPipeline::StageInfo>>& nodeStages,
                                      const std::vector<std::pair<std::set<const Node*>, std::set<const Node*>>>& groupsByDependency,
                                      std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStages);//A stage can contain more than one node, so the nodes used to build it is also here.
    void buildDependencyInfoRecursive(const Node *node, std::unordered_map<const Node*, std::set<const Node*>>& dependencies);
    std::vector<std::pair<std::set<const Node*>, std::set<const Node*>>> buildGroupsByDependency(std::unordered_map<const Node*, std::set<const Node*>>);
    bool canBeJoined(const std::set<const Node*>& existingNodes, const std::set<const Node*>& existingDependencies, const Node* currentNode, const std::set<const Node*>& currentDependencies);

    static std::shared_ptr<GraphicsPipeline::StageInfo>
    findSharedStage(const Node *currentNode, std::map<const Node *, std::shared_ptr<GraphicsPipeline::StageInfo>> &builtStages,
                    const std::vector<std::pair<std::set<const Node *>, std::set<const Node *>>> &dependencyGroups);

    void drawTextureSettings();

public:
    PipelineExtension(GraphicsInterface *graphicsWrapper, std::shared_ptr<GraphicsPipeline> currentGraphicsPipeline, std::shared_ptr<AssetManager> assetManager, OptionsUtil::Options* options,
                      const std::vector<std::string> &renderMethodNames, RenderMethods renderMethods);

    void drawDetailPane(NodeGraph* nodeGraph, const std::vector<const Node *>& nodes, const Node* selectedNode) override;

    const std::map<std::string, std::shared_ptr<Texture>> &getUsedTextures() const {
        return usedTextures;
    }

    std::string getName() override {
        return "PipelineExtension";
    }

    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement) override;

    void deserialize(const std::string &fileName, tinyxml2::XMLElement *editorExtensionElement) override;

    bool buildRenderPipelineStages(const std::vector<const Node *> &nodes, std::vector<
            std::pair<std::set<const Node *>, std::shared_ptr<GraphicsPipeline::StageInfo>>> & orderedStages);

    std::shared_ptr<GraphicsPipeline> combineStagesToPipeline() {
        std::shared_ptr<GraphicsPipeline> builtGraphicsPipeline = std::make_shared<GraphicsPipeline>(renderMethods);
        for(const auto& usedTexture: usedTextures) {
            if(usedTexture.second != nullptr) {
                builtGraphicsPipeline->addTexture(usedTexture.second);
            }
        }
        for (size_t i = 0; i < orderedStages.size(); ++i) {
            builtGraphicsPipeline->addNewStage(*(orderedStages[i].second));
        }
        addMessage("Built new Pipeline");
        return builtGraphicsPipeline;
    }

    void addError(const std::string& errorMessage) {
        this->errorMessages.emplace_back(errorMessage);
    }

    void addMessage(const std::string& message) {
        this->messages.emplace_back(message);
    }

    bool isPipelineBuilt() {
        return builtPipeline != nullptr;
    }

    std::shared_ptr<GraphicsPipeline> handOverBuiltPipeline() {
        std::shared_ptr<GraphicsPipeline> temp = builtPipeline;
        builtPipeline = nullptr;
        return temp;
    }

    bool isNodeGraphValid() const {
        return nodeGraphValid;
    }

    void setNodeGraphValid(bool nodeGraphValid) {
        PipelineExtension::nodeGraphValid = nodeGraphValid;
        if(!nodeGraphValid) {
            addError("Node Graph is not valid, can't build pipeline!");
        }
    }

    void recursiveUpdatePriorityForDependencies(std::vector<std::pair<std::set<const Node *>, std::set<const Node *>>> &dependencyGroups,
                                                std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStages,
                                                const std::pair<const std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStageInfo) const;
    void recursiveUpdatePriorityForDependents(std::vector<std::pair<std::set<const Node *>, std::set<const Node *>>> &dependencyGroups,
                                                std::map<std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStages,
                                                const std::pair<const std::shared_ptr<GraphicsPipeline::StageInfo>, std::set<const Node *>> &builtStageInfo) const;
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
