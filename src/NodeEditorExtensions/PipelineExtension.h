//
// Created by engin on 15.04.2019.
//

#ifndef LIMONENGINE_PIPELINEEXTENSION_H
#define LIMONENGINE_PIPELINEEXTENSION_H


#include <nodeGraph/src/EditorExtension.h>
#include "API/GraphicsInterface.h"
#include "Graphics/GraphicsPipeline.h"

#include <functional>
#include <vector>

class GraphicsPipelineStage;

class PipelineExtension : public EditorExtension {
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
    GraphicsInterface* graphicsWrapper = nullptr;
    Options* options;
    const std::vector<std::string>& renderMethodNames;
    GraphicsPipeline::RenderMethods& renderMethods;

    static bool getNameOfTexture(void* data, int index, const char** outText);

public:
    PipelineExtension(GraphicsInterface *graphicsWrapper, std::shared_ptr<GraphicsPipeline> currentGraphicsPipeline, Options* options,
                      const std::vector<std::string> &renderMethodNames, GraphicsPipeline::RenderMethods &renderMethods);

    void drawDetailPane(NodeGraph* nodeGraph, const std::vector<const Node *>& nodes, const Node* selectedNode) override;

    const std::map<std::string, std::shared_ptr<Texture>> &getUsedTextures() const {
        return usedTextures;
    }

    std::string getName() override {
        return "PipelineExtension";
    }

    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement) override;

    void buildRenderPipelineRecursive(const Node *node, GraphicsPipeline *graphicsPipeline);
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
