//
// Created by engin on 18.11.2026.
//

#ifndef LIMONENGINE_STAGECOVERAGE_H
#define LIMONENGINE_STAGECOVERAGE_H

#include <cstdint>
#include <string>
#include <vector>
#include "Utils/HashUtil.h"

//one object drawing stage, taken from the pipeline by Editor::extractPipelineStages when it becomes the world's
struct PipelineStageTags {
    std::string name;
    std::vector<HashUtil::HashedString> tags;
};

class StageCoverage {
public:
    //names of the stages that would draw something with these tags, each name once
    static void collectStageLabels(const std::vector<uint64_t> &tagHashes, const std::vector<PipelineStageTags> &stages, std::vector<std::string> &labels);

    //the "renders in" line. Only ever about the current pipeline, another one may draw what this one doesn't
    static void drawLine(const std::vector<std::string> &labels, const std::string &tooltip);

    static std::string joinLabels(const std::vector<std::string> &labels);
};

#endif //LIMONENGINE_STAGECOVERAGE_H
