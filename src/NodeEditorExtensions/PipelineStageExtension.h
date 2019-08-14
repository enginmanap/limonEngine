//
// Created by engin on 18.04.2019.
//

#ifndef LIMONENGINE_PIPELINESTAGEEXTENSION_H
#define LIMONENGINE_PIPELINESTAGEEXTENSION_H

#include <memory>
#include <map>
#include <utility>

#include <nodeGraph/src/NodeExtension.h>

#include "Graphics/GLHelper.h"
#include "PipelineExtension.h"

class Connection;
class PipelineStageExtension : public NodeExtension {
    PipelineExtension* pipelineExtension = nullptr;

    GLHelper::CullModes cullMode = GLHelper::CullModes::NO_CHANGE;
    bool clearBefore = false;
    bool blendEnabled = false;
    bool toScreen = false;
    std::string currentMethodName = PipelineExtension::renderMethodNames[0];
    std::map<const Connection*, std::pair<std::string, std::shared_ptr<Texture>>> outputTextures;
    std::map<const Connection*, int> inputTextureIndexes;
    std::map<const Connection*, GLHelper::FrameBufferAttachPoints > outputTextureIndexes;
public:
    PipelineStageExtension(PipelineExtension* pipelineExtension)  : pipelineExtension(pipelineExtension) {}
    void drawDetailPane(Node *node) override;

    bool isClearBefore() const {
        return clearBefore;
    }

    bool isBlendEnabled() const {
        return blendEnabled;
    }

    int getInputTextureIndex(const Connection* connection) const {
        auto indexIt = inputTextureIndexes.find(connection);
        if(indexIt == inputTextureIndexes.end()) {
            return 0;
        } else {
            return indexIt->second;
        }
    }

    const std::string &getMethodName() const {
        return currentMethodName;
    }

    GLHelper::FrameBufferAttachPoints getOutputTextureIndex(const Connection* connection) const {
        auto indexIt = outputTextureIndexes.find(connection);
        if(indexIt == outputTextureIndexes.end()) {
            return GLHelper::FrameBufferAttachPoints::NONE;
        } else {
            return indexIt->second;
        }
    }

    std::shared_ptr<Texture> getOutputTexture(const Connection* connection) const {
        auto textureIt = outputTextures.find(connection);
        if(textureIt == outputTextures.end()) {
            return nullptr;
        } else {
            return textureIt->second.second;
        }
    }

    GLHelper::CullModes getCullmode() const {
        return cullMode;
    }
};


#endif //LIMONENGINE_PIPELINESTAGEEXTENSION_H
