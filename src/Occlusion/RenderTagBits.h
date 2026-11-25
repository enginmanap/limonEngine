#ifndef LIMONENGINE_RENDERTAGBITS_H
#define LIMONENGINE_RENDERTAGBITS_H

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "TagMask.h"
#include "../Utils/HashUtil.h"

//numbers the object tags of one pipeline's stages. Masks made before a rebuild mean nothing after it, World::refreshRenderTagBits redoes every model's
class RenderTagBits {
    std::unordered_map<uint64_t, uint32_t> bitByTagHash;

public:
    static constexpr uint32_t MAX_TAG_COUNT = 128;//TagMask's width

    static uint32_t countTags(const std::map<std::string, std::vector<std::set<std::string>>> &cameraTagToRenderTagSetMap);

    //false when the pipeline draws more than MAX_TAG_COUNT object tags, the numbering is left as it was then
    bool rebuild(const std::map<std::string, std::vector<std::set<std::string>>> &cameraTagToRenderTagSetMap, uint32_t &tagCount);

    TagMask maskOf(const std::vector<uint64_t> &tagHashes) const;
    TagMask maskOf(const std::vector<HashUtil::HashedString> &tags) const;
};

#endif //LIMONENGINE_RENDERTAGBITS_H
