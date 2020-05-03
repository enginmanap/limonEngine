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
public:

private:
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
    GraphicsInterface* graphicsWrapper = nullptr;
    static bool getNameOfTexture(void* data, int index, const char** outText);
    const std::vector<std::string>& renderMethodNames;
    GraphicsPipeline::RenderMethods& renderMethods;
public:

    PipelineExtension(GraphicsInterface* graphicsWrapper, const std::vector<std::string>& renderMethodNames, GraphicsPipeline::RenderMethods& renderMethods);

    void drawDetailPane(const std::vector<const Node *>& nodes, const Node* selectedNode = nullptr) override;

    const std::map<std::string, std::shared_ptr<Texture>> &getUsedTextures() const {
        return usedTextures;
    }

    void buildRenderPipelineRecursive(const Node *node, GraphicsPipeline *graphicsPipeline);
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
