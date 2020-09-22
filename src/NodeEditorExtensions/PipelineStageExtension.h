//
// Created by engin on 18.04.2019.
//

#ifndef LIMONENGINE_PIPELINESTAGEEXTENSION_H
#define LIMONENGINE_PIPELINESTAGEEXTENSION_H

#include <memory>
#include <map>
#include <utility>

#include <nodeGraph/src/NodeExtension.h>

#include "API/GraphicsInterface.h"
#include "PipelineExtension.h"

class Connection;
class PipelineStageExtension : public NodeExtension {
    struct OutputTextureInfo {
        std::string name;
        GraphicsInterface::FrameBufferAttachPoints attachPoint = GraphicsInterface::FrameBufferAttachPoints::NONE;
        std::shared_ptr<Texture> texture = nullptr;
    };
    PipelineExtension* pipelineExtension = nullptr;

    GraphicsInterface::CullModes cullMode = GraphicsInterface::CullModes::NO_CHANGE;
    bool clearBefore = false;
    bool blendEnabled = false;
    bool anyOutputMultiLayered = false;
    bool toScreen = false;
    std::string currentMethodName = "";
    static const std::string LIGHT_TYPES[];
    unsigned int iterateOverLightType = 0;
    std::map<const Connection*, int> inputTextureIndexes;
    std::map<const Connection*, OutputTextureInfo> outputTextures;
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

    GraphicsInterface::FrameBufferAttachPoints getOutputTextureIndex(const Connection* connection) const {
        auto indexIt = outputTextures.find(connection);
        if(indexIt == outputTextures.end()) {
            return GraphicsInterface::FrameBufferAttachPoints::NONE;
        } else {
            return indexIt->second.attachPoint;
        }
    }

    std::shared_ptr<Texture> getOutputTexture(const Connection* connection) const {
        auto textureIt = outputTextures.find(connection);
        if(textureIt == outputTextures.end()) {
            return nullptr;
        } else {
            return textureIt->second.texture;
        }
    }

    GraphicsInterface::CullModes getCullmode() const {
        return cullMode;
    }

    std::string getName() override {
        return "PipelineStageExtension";
    }

    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement) override;

};


#endif //LIMONENGINE_PIPELINESTAGEEXTENSION_H
