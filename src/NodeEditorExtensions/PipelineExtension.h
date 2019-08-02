//
// Created by engin on 15.04.2019.
//

#ifndef LIMONENGINE_PIPELINEEXTENSION_H
#define LIMONENGINE_PIPELINEEXTENSION_H


#include <nodeGraph/src/EditorExtension.h>
#include "Graphics/GLHelper.h"

#include <functional>

class GraphicsPipeline;

class PipelineExtension : public EditorExtension {
public:
    struct RenderMethods {
        std::function<void(unsigned int, std::shared_ptr<GLSLProgram>)> renderLight;
        std::function<void()> renderWorld;
        std::function<void()> renderWorldTransparentObjects;
        std::function<void()> renderGUI;
        std::function<void()> ImGuiFrameSetup;
    };
private:
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
    GLHelper* glHelper = nullptr;
    static bool getNameOfTexture(void* data, int index, const char** outText);
    RenderMethods renderMethods;
public:


    void drawDetailPane() override;

    PipelineExtension(GLHelper* glHelper, RenderMethods renderMethods);
    const std::map<std::string, std::shared_ptr<Texture>> &getUsedTextures() const {
        return usedTextures;
    }

    void buildRenderPipelineRecursive(Node *node, GraphicsPipeline *graphicsPipeline);
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
