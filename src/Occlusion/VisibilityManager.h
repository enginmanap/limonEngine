#ifndef LIMONENGINE_VISIBILITYMANAGER_H
#define LIMONENGINE_VISIBILITYMANAGER_H

#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include "SDL2Helper.h"
#include "VisibilityRequest.h"
#include "glm/glm.hpp"

class World;
class Camera;
class RenderList;
class GraphicsPipeline;
class VisibilityManager {
    World* world;

    /**
     * this variable is used as camera and tag list. Outer map is camera, inner map is tag list. Neither should be removed from this map,
     * unless a camera is removed, or render pipeline change, as tags come from the pipeline.
     *
     * This map is also used as a list of Cameras, and Hashes, so if a camera is removed, it should be removed from this map
     * In case of a clear, we should not clear the hashes, as it is basically meaningless.
     *
     * RenderList is a custom container of meshes to render for that camera + tag, materials and order of rendering.
     */
    std::unordered_map<Camera*, std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>*> cullingResults;
    bool multiThreadedCulling = true;

    void fillVisibleObjectsUsingTags();
    std::map<VisibilityRequest*, SDL_Thread *> occlusionThreadManager();
    void resetVisibilityBufferForRenderPipelineChange();
    void resetCameraTagsFromPipeline(const std::map<std::string, std::vector<std::set<std::string>>> &cameraRenderTagListMap);
    void resetTagsAndRefillCulling();

    static void fillVisibleObjectPerCamera(const void* visibilityRequestRaw);
    static int staticOcclusionThread(void* visibilityRequestRaw);
    static uint32_t getLodLevel(const std::vector<long>& lodDistances, float skipRenderDistance, float skipRenderSize, float maxSkipRenderSize, const glm::mat4 &viewMatrix, const glm::vec3& playerPosition, glm::vec3 minAABB, glm::vec3 maxAABB, float &objectAverageDepth, float &objectScreenSize);

public:
    std::map<VisibilityRequest*, SDL_Thread *> visibilityThreadPool;

    explicit VisibilityManager(World* world);
    ~VisibilityManager();

    void update();
    void onPipelineChange();
    std::unordered_map<Camera*, std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>*>& getCullingResults();
    void addCamera(Camera* camera);
    void removeCamera(Camera* camera);
};

#endif //LIMONENGINE_VISIBILITYMANAGER_H
