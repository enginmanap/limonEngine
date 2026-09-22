//
// Created by engin on 22/09/2024.
//

#ifndef LIMONENGINE_VISIBILITYREQUEST_H
#define LIMONENGINE_VISIBILITYREQUEST_H
#include "limonAPI/Options.h"
#include <SDL2MultiThreading.h>
#include <Utils/HashUtil.h>

#include "Occlusion/OcclusionCullerHelper.h"
#include "Occlusion/RenderList.h"


class PhysicalRenderable;
class Attachable;
class Camera;

class VisibilityRequest {
public:
    /**
     * Culling keys one render list per stage, by that stage's own tag list. This answers "is this list mine",
     * so every tag has to match, one tag in common would hand {basic,static} the {basic,animated} list as well.
     */
    static bool vectorComparator(const std::vector<uint64_t>& a, const std::vector<HashUtil::HashedString>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (uint64_t aValue : a) {
            bool found = false;
            for (const HashUtil::HashedString& bValue : b) {
                if (aValue == bValue.hash) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }
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

        SDL2MultiThreading::Latch visibilityLatch; //Means we start the processing.
        SDL2MultiThreading::Barrier* frameBarrier = nullptr; // Barrier that we have that we signal when we are done.
        const Camera* const camera;
        glm::vec3 playerPosition;
        const OptionsUtil::Options* options;
        const OptionsUtil::Options::Option<std::vector<long>> lodDistancesOption;
        const OptionsUtil::Options::Option<double> skipRenderDistanceOption;
        const OptionsUtil::Options::Option<double> skipRenderSizeOption;
        const OptionsUtil::Options::Option<double> maxSkipRenderSizeOption;
        const OptionsUtil::Options::Option<long> splitModelToMeshCountOption;

        const OptionsUtil::Options::Option<bool> occlusionRenderDumpOption;
        const OptionsUtil::Options::Option<long> occlusionRenderDumpFrequencyOption;
        const OptionsUtil::Options::Option<double> occlusionOccluderSizePerspectiveOption;
        const OptionsUtil::Options::Option<double> occlusionOccluderSizeOrthographicOption;
        const OptionsUtil::Options::Option<bool> occlusionEnabledOption;

        const std::unordered_map<uint32_t, Model *>* const objects;
        const Attachable* const playerObject;
        std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher>* visibility;
        mutable OcclusionCullerHelper occlusionCuller;
        mutable std::unordered_map<uint32_t, const std::vector<glm::mat4>*> changedBoneTransforms;
        bool running = true; //non atomic because only used in between latch/barrier. But, must be checked before doing anything (because false means dangling pointers to camera and objects)
        bool cameraIsDirty = true; // cached by main thread before each signal; avoids Python GIL call from background thread
        bool playerDead = false; // same reason, isDead() reaches the player and we cannot touch that from here

        VisibilityRequest(Camera* camera, std::unordered_map<uint32_t, Model *>* objects, const Attachable* playerObject, std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher> * visibility, const glm::vec3& playerPosition, const OptionsUtil::Options* options, SDL2MultiThreading::Barrier* frameBarrier, const std::string& cameraName) :
                visibilityLatch(cameraName), frameBarrier(frameBarrier), camera(camera), playerPosition(playerPosition), options(options),
                lodDistancesOption(options->getOption<std::vector<long>>(HASH("LOD_distanceList"))),
                skipRenderDistanceOption(options->getOption<double>(HASH("LOD_skipRenderDistance"))),
                skipRenderSizeOption(options->getOption<double>(HASH("LOD_skipRenderSize"))),
                maxSkipRenderSizeOption(options->getOption<double>(HASH("LOD_maxSkipRenderSize"))),
                splitModelToMeshCountOption(options->getOption<long>(HASH("SplitModelToMeshCount"))),
                occlusionRenderDumpOption(options->getOption<bool>(HASH("occlusion_renderDump"))),
                occlusionRenderDumpFrequencyOption(options->getOption<long>(HASH("occlusion_renderDumpFrequency"))),
                occlusionOccluderSizePerspectiveOption(options->getOption<double>(HASH("occlusion_occluderSizePerspective"))),
                occlusionOccluderSizeOrthographicOption(options->getOption<double>(HASH("occlusion_occluderSizeOrthographic"))),
                occlusionEnabledOption(options->getOption<bool>(HASH("occlusion_enabled"))),
                objects(objects), playerObject(playerObject), visibility(visibility),
                occlusionCuller(options->getOption<long>(HASH("occlusion_renderWidth")),
                options->getOption<long>(HASH("occlusion_renderHeight"))) {
        }

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
