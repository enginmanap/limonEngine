#include "RenderTagBits.h"

uint32_t RenderTagBits::countTags(const std::map<std::string, std::vector<std::set<std::string>>> &cameraTagToRenderTagSetMap) {
    std::set<uint64_t> tagHashes;
    for (const std::pair<const std::string, std::vector<std::set<std::string>>> &cameraEntry : cameraTagToRenderTagSetMap) {
        for (const std::set<std::string> &tagSet : cameraEntry.second) {
            for (const std::string &tag : tagSet) {
                tagHashes.insert(HashUtil::hashString(tag));
            }
        }
    }
    return (uint32_t) tagHashes.size();
}

bool RenderTagBits::rebuild(const std::map<std::string, std::vector<std::set<std::string>>> &cameraTagToRenderTagSetMap, uint32_t &tagCount) {
    std::unordered_map<uint64_t, uint32_t> newBitByTagHash;
    for (const std::pair<const std::string, std::vector<std::set<std::string>>> &cameraEntry : cameraTagToRenderTagSetMap) {
        for (const std::set<std::string> &tagSet : cameraEntry.second) {
            for (const std::string &tag : tagSet) {
                const uint64_t tagHash = HashUtil::hashString(tag);
                if (newBitByTagHash.find(tagHash) == newBitByTagHash.end()) {
                    const uint32_t newBit = (uint32_t) newBitByTagHash.size();
                    newBitByTagHash[tagHash] = newBit;
                }
            }
        }
    }
    tagCount = (uint32_t) newBitByTagHash.size();
    if (tagCount > MAX_TAG_COUNT) {
        return false;
    }
    bitByTagHash = newBitByTagHash;
    return true;
}

TagMask RenderTagBits::maskOf(const std::vector<uint64_t> &tagHashes) const {
    TagMask mask;
    for (uint64_t tagHash : tagHashes) {
        std::unordered_map<uint64_t, uint32_t>::const_iterator bitIterator = bitByTagHash.find(tagHash);
        if (bitIterator != bitByTagHash.end()) {
            mask.set(bitIterator->second);
        }
    }
    return mask;
}

TagMask RenderTagBits::maskOf(const std::vector<HashUtil::HashedString> &tags) const {
    TagMask mask;
    for (const HashUtil::HashedString &tag : tags) {
        std::unordered_map<uint64_t, uint32_t>::const_iterator bitIterator = bitByTagHash.find(tag.hash);
        if (bitIterator != bitByTagHash.end()) {
            mask.set(bitIterator->second);
        }
    }
    return mask;
}
