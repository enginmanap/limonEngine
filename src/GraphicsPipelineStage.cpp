//
// Created by engin on 5.03.2019.
//

#include "GraphicsPipelineStage.h"

void GraphicsPipelineStage::activate() {
    glHelper->switchRenderStage(renderWidth, renderHeight, frameBufferID, blendEnabled, inputs);
}