//
// Created by engin on 18/03/2026.
//

#ifndef LIMONENGINE_SKYSHOULDRENDERBEFOREBLENDRULE_H
#define LIMONENGINE_SKYSHOULDRENDERBEFOREBLENDRULE_H
#include <memory>
#include <set>
#include <vector>

#include "Graphics/GraphicsPipeline.h"

class Node;


class SkyShouldRenderBeforeBlendRule {
public:
    static void apply(std::vector<std::pair<std::set<const Node *>, std::shared_ptr<GraphicsPipeline::StageInfo>>> &orderedStages);
};


#endif //LIMONENGINE_SKYSHOULDRENDERBEFOREBLENDRULE_H