//
// Created by engin on 22/09/2024.
//

#ifndef LIMONENGINE_VISIBILITYREQUEST_H
#define LIMONENGINE_VISIBILITYREQUEST_H
#include "Occlusion/RenderList.h"


class VisibilityRequest {
public:
    static bool vectorComparator(const std::vector<uint64_t>& a, const std::vector<HashUtil::HashedString>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        bool found = false;
        for (uint64_t aValue : a) {
            for (const HashUtil::HashedString& bValue : b) {
                if (aValue == bValue.hash) {
                    found = true;
                }
            }
        }
        if (found == false) {
            return false;
        }
        found = false;
        for (const HashUtil::HashedString& bValue : b) {
            for (uint64_t aValue : a) {
                if (bValue.hash == aValue) {
                    found = true;
                }
            }
        }
        if (found == false) {
            return false;
        }
        return true;
    }

        class uint64_vector_hasher {
        public:
            std::size_t operator()(std::vector<uint64_t> const &vec) const {
                std::size_t seed = vec.size();
                for (auto x: vec) {
                    x = ((x >> 16) ^ x) * 0x45d9f3b;
                    x = ((x >> 16) ^ x) * 0x45d9f3b;
                    x = (x >> 16) ^ x;
                    seed ^= x + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                }
                return seed;
            }
        };

        static SDL2MultiThreading::Condition condition;
        const Camera* const camera;
        glm::vec3 playerPosition;
        const OptionsUtil::Options* options;
        const OptionsUtil::Options::Option<std::vector<long>> lodDistancesOption;
        const OptionsUtil::Options::Option<double> skipRenderDistanceOption;
        const OptionsUtil::Options::Option<double> skipRenderSizeOption;
        const OptionsUtil::Options::Option<double> maxSkipRenderSizeOption;
        const std::unordered_map<uint32_t, PhysicalRenderable *>* const objects;
        std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher>* visibility;
        bool running = true;
        std::atomic<uint32_t> frameCount;
        SDL2MultiThreading::SpinLock inProgressLock;
        SDL_mutex* blockMutex = SDL_CreateMutex();
        VisibilityRequest(Camera* camera, std::unordered_map<uint32_t, PhysicalRenderable *>* objects, std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher> * visibility, const glm::vec3& playerPosition, const OptionsUtil::Options* options) :
                camera(camera), playerPosition(playerPosition), options(options),
                lodDistancesOption(options->getOption<std::vector<long>>(HASH("LodDistanceList"))),
                skipRenderDistanceOption(options->getOption<double>(HASH("SkipRenderDistance"))),
                skipRenderSizeOption(options->getOption<double>(HASH("SkipRenderSize"))),
                maxSkipRenderSizeOption(options->getOption<double>(HASH("MaxSkipRenderSize"))),
                objects(objects), visibility(visibility), frameCount(0) {

        };

        std::vector<RenderList> getRenderListsForHashList(const std::vector<HashUtil::HashedString>& hashList) const {
            std::vector<RenderList> renderLists;
            for (const auto& visibilityEntry:(*visibility)) {
                if (vectorComparator(visibilityEntry.first, hashList)) {
                    renderLists.emplace_back(visibilityEntry.second);
                }
            }
            return renderLists;
        }

    static bool isAnyTagMatch(const std::vector<HashUtil::HashedString>& renderTags, const std::list<HashUtil::HashedString> & objectTags) {
            for (const auto& renderTag:renderTags) {
                for (const auto& objectTag:objectTags) {
                    if (renderTag.hash == objectTag.hash) {
                        return true;
                    }
                }
            }
            return false;
        }

    static bool isAnyTagMatch(const std::vector<uint64_t>& renderTags, const std::list<HashUtil::HashedString> & objectTags) {
            for (const auto& renderTag:renderTags) {
                for (const auto& objectTag:objectTags) {
                    if (renderTag == objectTag.hash) {
                        return true;
                    }
                }
            }
            return false;
        }
};


#endif //LIMONENGINE_VISIBILITYREQUEST_H
