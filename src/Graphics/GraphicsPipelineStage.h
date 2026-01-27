//
// Created by engin on 5.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINESTAGE_H
#define LIMONENGINE_GRAPHICSPIPELINESTAGE_H


#include <cstdint>
#include <map>
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "Texture.h"

class GraphicsPipelineStage {
    GraphicsInterface* graphicsWrapper = nullptr;
    std::string renderWidthOption;
    std::string renderHeightOption;
    std::string foundName;
    uint32_t defaultRenderWidth;
    uint32_t defaultRenderHeight;
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t frameBufferID;
    uint32_t nextPresetIndex = 1;//used by pipeline builder. If multiple programs are set per stage, we might want to attach n to first program, then when building second, we should start from n+1.
    std::vector<std::string> cameraTags;//These tags are used to select which cameras are suppose to render in this stage.
    //TODO how multiple cameras will render to same input/output is not clear to me. For lights they can select different layers, but what else? Atlases etc. not clear yet.
    std::vector<std::string> objectTags;//These tags are used to select which cameras are suppose to render in this stage.
    bool blendEnabled = false;
    bool depthTestEnabled = false;
    bool depthWriteEnabled = true;
    bool scissorEnabled = false;
    bool colorAttachment = false;
    bool depthAttachment = false;
    GraphicsInterface::CullModes cullMode = GraphicsInterface::CullModes::NO_CHANGE;

    std::map<uint32_t, std::shared_ptr<Texture>> inputs;
    std::map<GraphicsInterface::FrameBufferAttachPoints, std::shared_ptr<Texture>> outputs;

public:

    GraphicsPipelineStage(GraphicsInterface* graphicsWrapper, uint32_t renderWidth, uint32_t renderHeight, const std::string& renderWidthOption, const std::string& renderHeightOption, bool blendEnabled, bool depthTestEnabled, bool depthWriteEnabled, bool scissorEnabled, bool toScreen = false) :
            graphicsWrapper(graphicsWrapper), renderWidthOption(renderWidthOption), renderHeightOption(renderHeightOption), defaultRenderWidth(renderWidth), defaultRenderHeight(renderHeight), renderWidth(renderWidth), renderHeight(renderHeight), blendEnabled(blendEnabled), depthTestEnabled(depthTestEnabled), depthWriteEnabled(depthWriteEnabled), scissorEnabled(scissorEnabled) {
        if(!renderHeightOption.empty()) {
            OptionsUtil::Options::Option<long> renderHeightOptionOption = graphicsWrapper->getOptions()->getOption<long>(hash(renderHeightOption));
            this->renderHeight = renderHeightOptionOption.getOrDefault(this->renderHeight);//if not found, it will not change, but we already set it to default
        }
        if(!renderWidthOption.empty()) {
            OptionsUtil::Options::Option<long> renderWidthOptionOption = graphicsWrapper->getOptions()->getOption<long>(hash(renderWidthOption));
            this->renderWidth = renderWidthOptionOption.getOrDefault(this->renderWidth);//if not found, it will not change, but we already set it to default
        }
        if(toScreen) {
            frameBufferID = 0;
            //since this is directly to screen, we should clear both color and depth, if clear is requested, because we will not get outputs set.
            colorAttachment = true;
            depthAttachment = true;
            foundName = "Screen";
        } else {
            frameBufferID = graphicsWrapper->createFrameBuffer(this->renderWidth, this->renderHeight);
        }
    }

    ~GraphicsPipelineStage() {
        graphicsWrapper->deleteFrameBuffer(frameBufferID);
    }

    void setInput(uint32_t textureAttachmentPoint, std::shared_ptr<Texture> texture) {
        this->inputs[textureAttachmentPoint] = texture;
    }
    void
    setOutput(GraphicsInterface::FrameBufferAttachPoints attachmentPoint, std::shared_ptr<Texture> texture, bool clear = false, uint32_t layer = -1) {
        graphicsWrapper->attachDrawTextureToFrameBuffer(this->frameBufferID, texture->getType(), texture->getTextureID(),
                                                 attachmentPoint, layer, clear);
        switch (attachmentPoint) {
            case GraphicsInterface::FrameBufferAttachPoints::DEPTH: depthAttachment = true; break;
            case GraphicsInterface::FrameBufferAttachPoints::COLOR0:/*fallthrough*/
            case GraphicsInterface::FrameBufferAttachPoints::COLOR1:/*fallthrough*/
            case GraphicsInterface::FrameBufferAttachPoints::COLOR2:/*fallthrough*/
            case GraphicsInterface::FrameBufferAttachPoints::COLOR3:/*fallthrough*/
            case GraphicsInterface::FrameBufferAttachPoints::COLOR4:/*fallthrough*/
            case GraphicsInterface::FrameBufferAttachPoints::COLOR5:/*fallthrough*/
            case GraphicsInterface::FrameBufferAttachPoints::COLOR6: colorAttachment = true; break;
            case GraphicsInterface::FrameBufferAttachPoints::NONE: break;
        }
        this->outputs[attachmentPoint] = texture;
        if(this->foundName != "Screen" && texture->getName() != "") {
            this->foundName = texture->getName();
        }
    }

    GraphicsInterface::CullModes getCullMode() const {
        return cullMode;
    }

    void setCullMode(GraphicsInterface::CullModes cullMode) {
        GraphicsPipelineStage::cullMode = cullMode;
    }
    std::shared_ptr<Texture> getOutput(GraphicsInterface::FrameBufferAttachPoints attachPoint) {
        if(outputs.find(attachPoint) != outputs.end()) {
            return outputs[attachPoint];
        }
        std::cerr << "Requested output can't be found, exiting" << std::endl;
        exit(1);
    }

    uint32_t getLastPresetIndex() const {
        return nextPresetIndex;
    }

    void setLastPresetIndex(uint32_t lastPresetIndex) {
        GraphicsPipelineStage::nextPresetIndex = lastPresetIndex;
    }

    bool isBlendEnabled() const {
        return blendEnabled;
    }

    void setBlendEnabled(bool blendEnabled) {
        GraphicsPipelineStage::blendEnabled = blendEnabled;
    }

    bool isDepthTestEnabled() const {
        return depthTestEnabled;
    }

    void setDepthTestEnabled(bool depthTestEnabled) {
        GraphicsPipelineStage::depthTestEnabled = depthTestEnabled;
    }

    bool isDepthWriteEnabled() const {
        return depthWriteEnabled;
    }

    void setDepthWriteEnabled(bool depthWriteEnabled) {
        GraphicsPipelineStage::depthWriteEnabled = depthWriteEnabled;
    }

    bool isScissorEnabled() const {
        return scissorEnabled;
    }

    void setScissorEnabled(bool scissorEnabled) {
        GraphicsPipelineStage::scissorEnabled = scissorEnabled;
    }

    void activate(bool clear = false);

    void activate(const std::map<std::shared_ptr<Texture>, std::pair<GraphicsInterface::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear = false);


    std::vector<std::string> getCameraTags() const {
        return cameraTags;
    }

    void setCameraTags(const std::vector<std::string> &cameraTags) {
        GraphicsPipelineStage::cameraTags = cameraTags;
    }

    const std::vector<std::string> &getObjectTags() const {
        return objectTags;
    }

    void setObjectTags(const std::vector<std::string> &objectTags) {
        GraphicsPipelineStage::objectTags = objectTags;
    }

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, OptionsUtil::Options *options);

    void generateMipMaps(const std::shared_ptr<Texture> & shared) {
        graphicsWrapper->generateMipMapForTexture(shared->getTextureID());
    }

    static std::shared_ptr<GraphicsPipelineStage> deserialize(tinyxml2::XMLElement *stageNode, GraphicsInterface* graphicsWrapper, const std::vector<std::shared_ptr<Texture>>& textures);

};


#endif //LIMONENGINE_GRAPHICSPIPELINESTAGE_H
