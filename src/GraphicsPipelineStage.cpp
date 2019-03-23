//
// Created by engin on 5.03.2019.
//

#include "GraphicsPipelineStage.h"

void GraphicsPipelineStage::activate(bool clear) {
    glHelper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, clear && colorAttachment, clear && depthAttachment, inputs);
}

void GraphicsPipelineStage::activate(const std::map<std::shared_ptr<GLHelper::Texture>, std::pair<GLHelper::FrameBufferAttachPoints, int>> &attachmentLayerMap, bool clear) {
    glHelper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, clear && colorAttachment, clear && depthAttachment, cullMode, inputs, attachmentLayerMap);
}