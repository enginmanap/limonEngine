//
// Created by engin on 5.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINESTAGE_H
#define LIMONENGINE_GRAPHICSPIPELINESTAGE_H


#include <cstdint>
#include <map>
#include "GraphicsInterface.h"
#include "Texture.h"

class GraphicsPipelineStage {
    GraphicsInterface* glHelper = nullptr;
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t frameBufferID;
    bool blendEnabled = false;
    bool colorAttachment = false;
    bool depthAttachment = false;
    GraphicsInterface::CullModes cullMode = GraphicsInterface::CullModes::NO_CHANGE;

    std::map<uint32_t, std::shared_ptr<Texture>> inputs;
    std::map<GraphicsInterface::FrameBufferAttachPoints, std::shared_ptr<Texture>> outputs;

public:

    GraphicsPipelineStage(GraphicsInterface *glHelper, uint32_t renderWidth, uint32_t renderHeight, bool blendEnabled, bool toScreen = false) :
            glHelper(glHelper), renderWidth(renderWidth), renderHeight(renderHeight), blendEnabled(blendEnabled) {
        if(toScreen) {
            frameBufferID = 0;
            //since this is directly to screen, we should clear both color and depth, if clear is requested, because we will not get outputs set.
            colorAttachment = true;
            depthAttachment = true;
        } else {
            frameBufferID = glHelper->createFrameBuffer(renderWidth, renderHeight);
        }
    }

    ~GraphicsPipelineStage() {
        glHelper->deleteFrameBuffer(frameBufferID);
    }

    void setInput(uint32_t textureAttachmentPoint, std::shared_ptr<Texture> texture) {
        this->inputs[textureAttachmentPoint] = texture;
    }
    void
    setOutput(GraphicsInterface::FrameBufferAttachPoints attachmentPoint, std::shared_ptr<Texture> texture, bool clear = false, uint32_t layer = -1) {
        glHelper->attachDrawTextureToFrameBuffer(this->frameBufferID, texture->getType(), texture->getTextureID(),
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

    void activate(bool clear = false);

    void activate(const std::map<std::shared_ptr<Texture>, std::pair<GraphicsInterface::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear = false);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GraphicsPipelineStage *deserialize(tinyxml2::XMLElement *stageNode, GraphicsInterface *glHelper, Options *options);

};


#endif //LIMONENGINE_GRAPHICSPIPELINESTAGE_H
