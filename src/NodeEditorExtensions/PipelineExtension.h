//
// Created by engin on 15.04.2019.
//

#ifndef LIMONENGINE_PIPELINEEXTENSION_H
#define LIMONENGINE_PIPELINEEXTENSION_H


#include <nodeGraph/src/EditorExtension.h>
#include "Graphics/GLHelper.h"

class GraphicsPipeline;

class PipelineExtension : public EditorExtension {
    std::map<std::string, std::shared_ptr<Texture>> usedTextures;
    GLHelper* glHelper = nullptr;
    static bool getNameOfTexture(void* data, int index, const char** outText);
public:

    PipelineExtension(GLHelper* glHelper);

    void drawDetailPane() override;

    const std::map<std::string, std::shared_ptr<Texture>> &getUsedTextures() const {
        return usedTextures;
    }

    void buildRenderPipelineRecursive(Node *node, GraphicsPipeline *graphicsPipeline);
};


#endif //LIMONENGINE_PIPELINEEXTENSION_H
