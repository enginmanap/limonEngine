//
// Created by engin on 31.03.2019.
//

#include "GraphicsPipeline.h"

//Static initialize of the vector
std::vector<std::string> GraphicsPipeline::renderMethodNames { "None", "All directional shadows", "All point Shadow", "Opaque objects", "Animated objects", "Transparent objects", "GUI Texts", "GUI Images", "Editor", "Sky", "Debug information", "Opaque player attachments", "Animated player attachments", "Transparent player attachments"};


void GraphicsPipeline::render() {
    for(auto stageInfo:pipelineStages) {
        if(stageInfo.attachmentLayerMap.size() > 0) {
            stageInfo.stage->activate(stageInfo.attachmentLayerMap, stageInfo.clear);
        } else {
            stageInfo.stage->activate(stageInfo.clear);
        }
        for(auto renderMethod:stageInfo.renderMethods) {
            renderMethod.first(renderMethod.second);
        }
    }
}
