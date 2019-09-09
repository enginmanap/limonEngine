//
// Created by engin on 31.03.2019.
//

#include "GraphicsPipeline.h"

void GraphicsPipeline::render() {
    for(auto stageInfo:pipelineStages) {
        stageInfo.stage->activate(stageInfo.attachmentLayerMap, stageInfo.clear);
        for(auto renderMethod:stageInfo.renderMethods) {
            renderMethod.first(renderMethod.second);
        }
    }
}
