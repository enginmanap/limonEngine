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
public:
    struct ProgramNameInfo {
        std::string vertexShaderName;
        std::string geometryShaderName;
        std::string fragmentShaderName;
    };
    struct OutputTextureInfo {
        std::string name;
        GraphicsInterface::FrameBufferAttachPoints attachPoint = GraphicsInterface::FrameBufferAttachPoints::NONE;
        std::shared_ptr<Texture> texture = nullptr;
    };
private:
    struct LightType {
        std::string name;
        std::string outputType;
    };
    PipelineExtension* pipelineExtension = nullptr;

    GraphicsInterface::CullModes cullMode = GraphicsInterface::CullModes::NO_CHANGE;
    bool clearBefore = false;
    bool blendEnabled = false;
    bool depthTestEnabled = false;
    bool scissorTestEnabled = false;
    bool anyOutputMultiLayered = false;
    bool toScreen = false;
    std::string currentMethodName = "";
    std::string originalOutputType;
    static const LightType LIGHT_TYPES[];
    uint32_t iterateOverLightType = 0;
    std::map<uint32_t, int> inputTextureIndexes;//connectionId to input texture index
    std::map<uint32_t, OutputTextureInfo> outputTextures; // connectionId to output information
    ProgramNameInfo programNameInfo;

public:
    void setProgramNameInfo(const ProgramNameInfo &programNameInfo) {
        PipelineStageExtension::programNameInfo = programNameInfo;
    }

public:

    PipelineStageExtension(PipelineExtension* pipelineExtension, ProgramNameInfo programName)  : pipelineExtension(pipelineExtension), programNameInfo(programName) {}
    explicit PipelineStageExtension(PipelineExtension* pipelineExtension)  : pipelineExtension(pipelineExtension) {}
    void drawDetailPane(Node *node) override;

    bool isClearBefore() const {
        return clearBefore;
    }

    bool isBlendEnabled() const {
        return blendEnabled;
    }

    int getInputTextureIndex(const Connection* connection) const;

    const std::string &getMethodName() const {
        return currentMethodName;
    }

    GraphicsInterface::FrameBufferAttachPoints getOutputTextureIndex(const Connection* connection) const;

    std::shared_ptr<Texture> getOutputTexture(const Connection* connection) const;
    const OutputTextureInfo* getOutputTextureInfo(const Connection* connection) const;

    GraphicsInterface::CullModes getCullmode() const {
        return cullMode;
    }

    std::string getName() override {
        return "PipelineStageExtension";
    }

    bool isDepthTestEnabled() const {
        return depthTestEnabled;
    }

    bool isScissorTestEnabled() const {
        return scissorTestEnabled;
    }

    const ProgramNameInfo &getProgramNameInfo() const {
        return programNameInfo;
    }

    void serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentElement) override;

    void deserialize(const std::string &fileName, tinyxml2::XMLElement *nodeExtensionElement);
};


#endif //LIMONENGINE_PIPELINESTAGEEXTENSION_H
