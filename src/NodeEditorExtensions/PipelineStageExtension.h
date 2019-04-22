//
// Created by engin on 18.04.2019.
//

#ifndef LIMONENGINE_PIPELINESTAGEEXTENSION_H
#define LIMONENGINE_PIPELINESTAGEEXTENSION_H

#include <memory>
#include <map>
#include <utility>


#include <nodeGraph/src/NodeExtension.h>

#include <GLHelper.h>

#include "PipelineExtension.h"

class Connection;
class PipelineStageExtension : public NodeExtension {
    PipelineExtension* pipelineExtension = nullptr;


    std::map<const Connection*, std::pair<std::string, std::shared_ptr<GLHelper::Texture>>> outputTextures;
public:
    PipelineStageExtension(PipelineExtension* pipelineExtension)  : pipelineExtension(pipelineExtension) {}
    void drawDetailPane(Node *node) override;
};


#endif //LIMONENGINE_PIPELINESTAGEEXTENSION_H
