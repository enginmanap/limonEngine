//
// Created by engin on 5.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINESTAGE_H
#define LIMONENGINE_GRAPHICSPIPELINESTAGE_H


#include <cstdint>
#include <map>
#include "OpenGLGraphics.h"
#include "Texture.h"

class GraphicsPipelineStage {
    OpenGLGraphics* glHelper = nullptr;
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t frameBufferID;
    bool blendEnabled = false;
    bool colorAttachment = false;
    bool depthAttachment = false;
    OpenGLGraphics::CullModes cullMode = OpenGLGraphics::CullModes::NO_CHANGE;

    std::map<uint32_t, std::shared_ptr<Texture>> inputs;
    std::map<OpenGLGraphics::FrameBufferAttachPoints, std::shared_ptr<Texture>> outputs;

public:

    GraphicsPipelineStage(OpenGLGraphics *glHelper, uint32_t renderWidth, uint32_t renderHeight, bool blendEnabled, bool toScreen = false) :
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
    setOutput(OpenGLGraphics::FrameBufferAttachPoints attachmentPoint, std::shared_ptr<Texture> texture, bool clear = false, uint32_t layer = -1) {
        glHelper->attachDrawTextureToFrameBuffer(this->frameBufferID, texture->getType(), texture->getTextureID(),
                                                 attachmentPoint, layer, clear);
        switch (attachmentPoint) {
            case OpenGLGraphics::FrameBufferAttachPoints::DEPTH: depthAttachment = true; break;
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR0:/*fallthrough*/
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR1:/*fallthrough*/
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR2:/*fallthrough*/
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR3:/*fallthrough*/
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR4:/*fallthrough*/
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR5:/*fallthrough*/
            case OpenGLGraphics::FrameBufferAttachPoints::COLOR6: colorAttachment = true; break;
            case OpenGLGraphics::FrameBufferAttachPoints::NONE: break;
        }
        this->outputs[attachmentPoint] = texture;
    }

    OpenGLGraphics::CullModes getCullMode() const {
        return cullMode;
    }

    void setCullMode(OpenGLGraphics::CullModes cullMode) {
        GraphicsPipelineStage::cullMode = cullMode;
    }

    void activate(bool clear = false);

    void activate(const std::map<std::shared_ptr<Texture>, std::pair<OpenGLGraphics::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear = false);

    bool serialize(tinyxml2::XMLDocument &document, tinyxml2::XMLElement *parentNode, Options *options);

    static GraphicsPipelineStage *deserialize(tinyxml2::XMLElement *stageNode, OpenGLGraphics *glHelper, Options *options);

};


#endif //LIMONENGINE_GRAPHICSPIPELINESTAGE_H
