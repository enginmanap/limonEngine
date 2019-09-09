//
// Created by engin on 31.03.2019.
//

#include "GraphicsPipeline.h"

void GraphicsPipeline::render() {
    for(auto stage:pipelineStages) {
        stage.first.stage->activate(stage.first.attachmentLayerMap, stage.first.clear);
        stage.second(nullptr);
    }
}
