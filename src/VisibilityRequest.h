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
#include "CullingEntry.h"
#include "limonAPI/Graphics/GraphicsInterface.h"//NR_MAX_MODELS
#include "Camera/Camera.h"
#include "Utils/HardCodedTags.h"
#include <limits>


class PhysicalRenderable;
class Attachable;

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
        //read once, a camera's tags are added before addCamera builds its request
        const bool isPlayerCamera;
        const bool isDirectionalLightCamera;
        glm::vec3 playerPosition;
        float playerPixelSize = 0.0f;//world size of one player screen pixel, per meter of distance unless orthographic. 0 is unknown
        bool playerPixelOrthographic = false;
        glm::vec3 playerCameraPosition = glm::vec3(0.0f);//shadows are judged from here, with or without the player pixel
        const OptionsUtil::Options* options;
        const OptionsUtil::Options::Option<double> skipRenderDistanceOption;
        const OptionsUtil::Options::Option<double> skipRenderSizeOption;
        const OptionsUtil::Options::Option<double> maxSkipRenderSizeOption;

        const OptionsUtil::Options::Option<bool> occlusionRenderDumpOption;
        const OptionsUtil::Options::Option<long> occlusionRenderDumpFrequencyOption;
        const OptionsUtil::Options::Option<double> occlusionOccluderSizePerspectiveOption;
        const OptionsUtil::Options::Option<double> occlusionOccluderSizeOrthographicOption;
        const OptionsUtil::Options::Option<bool> occlusionEnabledOption;
        const OptionsUtil::Options::Option<bool> occlusionDebugReverseOption;
        const OptionsUtil::Options::Option<bool> occlusionOccluderSizeByGroupOption;
        const OptionsUtil::Options::Option<double> occlusionOccluderGroupMemberMinFractionOption;
        const OptionsUtil::Options::Option<long> occlusionBakeLodLevelOption;
        const OptionsUtil::Options::Option<double> occlusionLodFilledPixelsOption;
        const OptionsUtil::Options::Option<double> lodSwitchHysteresisOption;
        const OptionsUtil::Options::Option<long> lodForceLevelOption;
        const OptionsUtil::Options::Option<bool> lodPixelScalePlayerOption;
        const OptionsUtil::Options::Option<bool> lodPixelScaleLightsOption;

        const std::vector<CullingEntry>* const cullingEntries;
        const Attachable* const playerObject;
        std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher>* visibility;
        //occlusion_debugReverse parks the meshes that survived the occlusion test here, World draws it translucent. Empty unless that option is on
        mutable RenderList debugOverlayRenderList;
        mutable OcclusionCullerHelper occlusionCuller;
        mutable std::unordered_map<uint32_t, const std::vector<glm::mat4>*> changedBoneTransforms;
        mutable std::vector<uint8_t> previousLodLevels;//indexed like cullingEntries, which never pass NR_MAX_MODELS
        bool running = true; //non atomic because only used in between latch/barrier. But, must be checked before doing anything (because false means dangling pointers to camera and objects)
        bool cameraIsDirty = true; // cached by main thread before each signal; avoids Python GIL call from background thread
        bool playerDead = false; // same reason, isDead() reaches the player and we cannot touch that from here

        VisibilityRequest(Camera* camera, const std::vector<CullingEntry>* cullingEntries, const Attachable* playerObject, std::unordered_map<std::vector<uint64_t>, RenderList, uint64_vector_hasher> * visibility, const glm::vec3& playerPosition, const OptionsUtil::Options* options, SDL2MultiThreading::Barrier* frameBarrier, const std::string& cameraName) :
                visibilityLatch(cameraName), frameBarrier(frameBarrier), camera(camera),
                isPlayerCamera(camera->hasTag(HashUtil::hashString(HardCodedTags::CAMERA_PLAYER))),
                isDirectionalLightCamera(camera->hasTag(HashUtil::hashString(HardCodedTags::CAMERA_LIGHT_DIRECTIONAL))),
                playerPosition(playerPosition), options(options),
                skipRenderDistanceOption(options->getOption<double>(HASH("LOD_skipRenderDistance"))),
                skipRenderSizeOption(options->getOption<double>(HASH("LOD_skipRenderSize"))),
                maxSkipRenderSizeOption(options->getOption<double>(HASH("LOD_maxSkipRenderSize"))),
                occlusionRenderDumpOption(options->getOption<bool>(HASH("occlusion_renderDump"))),
                occlusionRenderDumpFrequencyOption(options->getOption<long>(HASH("occlusion_renderDumpFrequency"))),
                occlusionOccluderSizePerspectiveOption(options->getOption<double>(HASH("occlusion_occluderSizePerspective"))),
                occlusionOccluderSizeOrthographicOption(options->getOption<double>(HASH("occlusion_occluderSizeOrthographic"))),
                occlusionEnabledOption(options->getOption<bool>(HASH("occlusion_enabled"))),
                occlusionDebugReverseOption(options->getOption<bool>(HASH("occlusion_debugReverse"))),
                occlusionOccluderSizeByGroupOption(options->getOption<bool>(HASH("occlusion_occluderSizeByGroup"))),
                occlusionOccluderGroupMemberMinFractionOption(options->getOption<double>(HASH("occlusion_occluderGroupMemberMinFraction"))),
                occlusionBakeLodLevelOption(options->getOption<long>(HASH("occlusion_bakeLodLevel"))),
                occlusionLodFilledPixelsOption(options->getOption<double>(HASH("occlusion_lodFilledPixels"))),
                lodSwitchHysteresisOption(options->getOption<double>(HASH("LOD_switchHysteresis"))),
                lodForceLevelOption(options->getOption<long>(HASH("LOD_forceLevel"))),
                lodPixelScalePlayerOption(options->getOption<bool>(HASH("LOD_pixelScalePlayer"))),
                lodPixelScaleLightsOption(options->getOption<bool>(HASH("LOD_pixelScaleLights"))),
                cullingEntries(cullingEntries), playerObject(playerObject), visibility(visibility),
                occlusionCuller(options->getOption<long>(HASH("occlusion_renderWidth")),
                options->getOption<long>(HASH("occlusion_renderHeight"))),
                previousLodLevels(NR_MAX_MODELS, NO_PREVIOUS_LOD_LEVEL) {
        }

        static constexpr uint8_t NO_PREVIOUS_LOD_LEVEL = std::numeric_limits<uint8_t>::max();

        //last level this camera chose per culling entry, so a model sitting on a switch point does not flip every frame.
        //Owned by this camera's culling thread. NO_PREVIOUS_LOD_LEVEL until it is picked, World::deleteModel must move it with the entry
        uint32_t getPreviousLodLevel(size_t entryIndex) const {
            return previousLodLevels[entryIndex];
        }

        void setPreviousLodLevel(size_t entryIndex, uint32_t level) const {
            previousLodLevels[entryIndex] = (uint8_t)level;//the level fits, VisibilityManager.cpp checks LOD_MAX_LEVEL_COUNT
        }

    static bool isAnyTagMatch(const std::vector<HashUtil::HashedString>& renderTags, const std::vector<HashUtil::HashedString> & objectTags) {
            for (const auto& renderTag:renderTags) {
                for (const auto& objectTag:objectTags) {
                    if (renderTag.hash == objectTag.hash) {
                        return true;
                    }
                }
            }
            return false;
        }

    static bool isAnyTagMatch(const std::vector<uint64_t>& renderTags, const std::vector<HashUtil::HashedString> & objectTags) {
            for (const auto& renderTag:renderTags) {
                for (const auto& objectTag:objectTags) {
                    if (renderTag == objectTag.hash) {
                        return true;
                    }
                }
            }
            return false;
        }

    static bool isAnyTagMatch(const std::vector<uint64_t>& renderTags, const std::vector<uint64_t> & meshTags) {
            for (uint64_t renderTag : renderTags) {
                for (uint64_t meshTag : meshTags) {
                    if (renderTag == meshTag) {
                        return true;
                    }
                }
            }
            return false;
        }
};


#endif //LIMONENGINE_VISIBILITYREQUEST_H
