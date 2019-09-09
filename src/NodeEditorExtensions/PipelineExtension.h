//
// Created by engin on 15.04.2019.
//

#ifndef LIMONENGINE_PIPELINEEXTENSION_H
#define LIMONENGINE_PIPELINEEXTENSION_H


#include <nodeGraph/src/EditorExtension.h>
#include "Graphics/GLHelper.h"

#include <functional>
#include <vector>

class GraphicsPipeline;

class PipelineExtension : public EditorExtension {
public:
    static std::vector<std::string> renderMethodNames;//This is not array, because custom effects might be loaded on runtime as extensions.
    struct RenderMethods {
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderOpaqueObjects;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderAnimatedObjects;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderTransparentObjects;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderGUITexts;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderGUIImages;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderPlayerAttachment;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderSky;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderEditor;
        std::function<void(const std::shared_ptr<GLSLProgram>&)> renderDebug;
    };
private:
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
    GLHelper* glHelper = nullptr;
    static bool getNameOfTexture(void* data, int index, const char** outText);
    RenderMethods renderMethods;
public:

    PipelineExtension(GLHelper* glHelper, RenderMethods renderMethods);

    void drawDetailPane(const std::vector<const Node *>& nodes, const Node* selectedNode = nullptr) override;

    const std::map<std::string, std::shared_ptr<Texture>> &getUsedTextures() const {
        return usedTextures;
    }

    void buildRenderPipelineRecursive(const Node *node, GraphicsPipeline *graphicsPipeline);
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
