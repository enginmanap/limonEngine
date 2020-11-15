//
// Created by engin on 5.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINESTAGE_H
#define LIMONENGINE_GRAPHICSPIPELINESTAGE_H


#include <cstdint>
#include <map>
#include "API/Graphics/GraphicsInterface.h"
#include "Texture.h"

class GraphicsPipelineStage {
    GraphicsInterface* graphicsWrapper = nullptr;
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t frameBufferID;
    uint32_t nextPresetIndex = 1;//used by pipeline builder. If multiple programs are set per stage, we might want to attach n to first program, then when building second, we should start from n+1.
    bool blendEnabled = false;
    bool depthTestEnabled = false;
    bool scissorEnabled = false;
    bool colorAttachment = false;
    bool depthAttachment = false;
    GraphicsInterface::CullModes cullMode = GraphicsInterface::CullModes::NO_CHANGE;

    std::map<uint32_t, std::shared_ptr<Texture>> inputs;
    std::map<GraphicsInterface::FrameBufferAttachPoints, std::shared_ptr<Texture>> outputs;

public:

    GraphicsPipelineStage(GraphicsInterface* graphicsWrapper, uint32_t renderWidth, uint32_t renderHeight, bool blendEnabled, bool depthTestEnabled, bool scissorEnabled, bool toScreen = false) :
            graphicsWrapper(graphicsWrapper), renderWidth(renderWidth), renderHeight(renderHeight), blendEnabled(blendEnabled), depthTestEnabled(depthTestEnabled), scissorEnabled(scissorEnabled) {
        if(toScreen) {
            frameBufferID = 0;
            //since this is directly to screen, we should clear both color and depth, if clear is requested, because we will not get outputs set.
            colorAttachment = true;
            depthAttachment = true;
        } else {
            frameBufferID = graphicsWrapper->createFrameBuffer(renderWidth, renderHeight);
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

    bool isScissorEnabled() const {
        return scissorEnabled;
    }

    void setScissorEnabled(bool scissorEnabled) {
        GraphicsPipelineStage::scissorEnabled = scissorEnabled;
    }

    void activate(bool clear = false);

    void activate(const std::map<std::shared_ptr<Texture>, std::pair<GraphicsInterface::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear = false);



    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static std::shared_ptr<GraphicsPipelineStage> deserialize(tinyxml2::XMLElement *stageNode, GraphicsInterface* graphicsWrapper, const std::vector<std::shared_ptr<Texture>>& textures, Options *options);

};


#endif //LIMONENGINE_GRAPHICSPIPELINESTAGE_H
