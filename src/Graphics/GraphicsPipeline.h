//
// Created by engin on 31.03.2019.
//

#ifndef LIMONENGINE_GRAPHICSPIPELINE_H
#define LIMONENGINE_GRAPHICSPIPELINE_H


#include <vector>
#include <functional>
#include <memory>
#include "GraphicsPipelineStage.h"

class World;
class GLSLProgram;

class GraphicsPipeline {
public:
    struct StageInfo {
        std::shared_ptr<GraphicsPipelineStage> stage;
        std::map<std::shared_ptr<Texture>, std::pair<GLHelper::FrameBufferAttachPoints, int>> attachmentLayerMap;
        std::vector<std::pair<std::function<void(const std::shared_ptr<GLSLProgram>&)>, std::shared_ptr<GLSLProgram>>> renderMethods;
        bool clear = false;
    };

    void addNewStage(const StageInfo& stageInformation) {
        pipelineStages.push_back(stageInformation);
    }
    void render();

private:
    std::vector<StageInfo> pipelineStages;
};


#endif //LIMONENGINE_GRAPHICSPIPELINE_H
