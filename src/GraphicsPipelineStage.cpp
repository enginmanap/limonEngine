//
// Created by engin on 5.03.2019.
//

#include "GraphicsPipelineStage.h"

void GraphicsPipelineStage::activate(bool clear) {
    glHelper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, clear, inputs);
}