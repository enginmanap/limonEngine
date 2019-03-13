//
// Created by engin on 5.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINESTAGE_H
#define LIMONENGINE_GRAPHICSPIPELINESTAGE_H


#include <cstdint>
#include <map>
#include "GLHelper.h"

class GraphicsPipelineStage {
    GLHelper* glHelper = nullptr;
    uint32_t renderWidth;
    uint32_t renderHeight;
    uint32_t frameBufferID;
    bool blendEnabled;

    std::map<uint32_t, std::shared_ptr<GLHelper::Texture>> inputs;
    std::map<GLHelper::FrameBufferAttachPoints, std::shared_ptr<GLHelper::Texture>> outputs;

public:

    GraphicsPipelineStage(GLHelper *glHelper, uint32_t renderWidth, uint32_t renderHeight, bool blendEnabled) :
    glHelper(glHelper), renderWidth(renderWidth), renderHeight(renderHeight), blendEnabled(blendEnabled) {
        frameBufferID = glHelper->createFrameBuffer(renderWidth, renderHeight);
    }

    ~GraphicsPipelineStage() {
        glHelper->deleteFrameBuffer(frameBufferID);
    }

    void setInput(uint32_t textureAttachmentPoint, std::shared_ptr<GLHelper::Texture> texture) {
        this->inputs[textureAttachmentPoint] = texture;
    }
    void setOutput(GLHelper::FrameBufferAttachPoints attachmentPoint, std::shared_ptr<GLHelper::Texture> texture, uint32_t layer = 0) {
        glHelper->attachDrawTextureToFrameBuffer(this->frameBufferID, texture->getType(), texture->getTextureID(), attachmentPoint, layer);
        this->outputs[attachmentPoint] = texture;
    }

    void activate(bool clear = false);


};


#endif //LIMONENGINE_GRAPHICSPIPELINESTAGE_H
