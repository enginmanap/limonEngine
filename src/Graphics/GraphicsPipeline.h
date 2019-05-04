//
// Created by engin on 31.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINE_H
#define LIMONENGINE_GRAPHICSPIPELINE_H


#include <vector>
#include <functional>
#include "GraphicsPipelineStage.h"

class World;

class GraphicsPipeline {
public:
    struct StageInfo {
        std::shared_ptr<GraphicsPipelineStage> stage;
        std::map<std::shared_ptr<Texture>, std::pair<GLHelper::FrameBufferAttachPoints, int>> attachmentLayerMap;
        bool clear = false;
    };

    void addNewStage(const StageInfo& stageInformation, std::function<void()> renderMethod) {
        pipelineStages.push_back(std::make_pair(stageInformation, renderMethod));
    }
    void render();

private:
    std::vector<std::pair<StageInfo, std::function<void()>>> pipelineStages;
};


#endif //LIMONENGINE_GRAPHICSPIPELINE_H
