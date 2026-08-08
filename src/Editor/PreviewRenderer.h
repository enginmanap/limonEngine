//
// Extracted from Editor: owns the asset-browser preview and the bone-exposure preview. They share the box-fit
// camera and the renderModelIMGUI bracket, so one class rather than two.
//

#ifndef LIMONENGINE_PREVIEWRENDERER_H
#define LIMONENGINE_PREVIEWRENDERER_H

#include "ImGui/imgui.h"
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#define MAX_PRELOAD_MODEL_COUNT_EDITOR 10

class World;
class Model;
class ModelAsset;
class Texture;
class GraphicsPipelineStage;
class GraphicsProgram;
class ImGuiHelper;
class ImGuiImageWrapper;

class PreviewRenderer {
    World* world;
    ImGuiHelper* imgGuiHelper;

    std::shared_ptr<Texture> colorTexture;
    std::shared_ptr<Texture> depthTexture;
    std::unique_ptr<GraphicsPipelineStage> backgroundRenderStage;
    ImGuiImageWrapper* wrapper = nullptr;

    std::vector<Model*> modelQueue;
    std::set<uint32_t> modelIdSet;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelAssetsWaitingCPULoad;
    std::unordered_map<std::string, std::shared_ptr<ModelAsset>> modelAssetsPreloaded;
    //so we know when the browsed asset changed and can reset its orbit
    std::string lastPreviewedAssetPath;

    //plain yaw/pitch, not a running quaternion, so resetting to zero on target change is trivial
    struct OrbitState {
        float yaw = 0.0f;   // radians, around world up
        float pitch = 0.0f; // radians, around the yawed local right axis, clamped to avoid pole flip
    };
    OrbitState assetPreviewOrbit;

    //bone preview's base is already tilted ~11 degrees up, so the pitch clamp needs the real base, not flat
    static const glm::vec3 BONE_PREVIEW_BASE_DIRECTION;
    static const glm::vec3 ASSET_PREVIEW_BASE_DIRECTION;

    Model* getModelAndMoveToEnd(const std::string& modelFilePath);
    Model* createRenderAndAddModelToLRU(const std::string &modelFileName, const glm::vec3 &newObjectPosition, std::shared_ptr<GraphicsProgram> graphicsProgram);
    void setTransformToModel(Model *model, const glm::vec3 &newObjectPosition);
    void renderSelectedObject(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram);

    //single source of truth so the render target, the overlay bake space, and the aspect can't drift apart
    static constexpr uint32_t BONE_PREVIEW_WIDTH = 640;
    static constexpr uint32_t BONE_PREVIEW_HEIGHT = 480;

    // We can't re-use the background renderer state for animation/bone preview, as editor might have both visible at the same time.
    struct BonePreviewState {
        // We can't use the main context, as it is mid frame, so we create a separate one for this.
        // We do reuse the font atlas, so it is a very lightweight thingy.
        ImGuiContext* imGuiContext = nullptr;

        std::shared_ptr<Texture> colorTexture;
        std::shared_ptr<Texture> depthTexture;
        std::unique_ptr<GraphicsPipelineStage> renderStage;
        ImGuiImageWrapper* wrapper = nullptr;
        // We can't use the actual rig-id, because that would corrupt the models state, so we will reserve another ID and use that.
        uint32_t rigId = 0;
        uint64_t startWallTime = 0;//wall time when the currently-previewed model/animation was first shown, for looping playback
        uint32_t modelObjectID = 0xFFFFFFFF;//tracks which model+animation startWallTime belongs to
        std::string animationName;
        //Refreshed every bakeSkeletonOverlay() call (same frame the preview image is shown), so click hit-testing
        //always matches the exact pose currently on screen. Local pixel space, same convention as the overlay itself.
        std::vector<std::pair<uint32_t, ImVec2>> boneScreenPositions;
        OrbitState orbit;
    };
    BonePreviewState bonePreview;

    // We might or might not render add object preview, same with animation/bone preview. If neither rendered, we don't need to
    // clean up/restore so we flag if any of them did render.
    bool offscreenPreviewRenderedThisFrame = false;
    void beginOffscreenModelPreview(GraphicsPipelineStage* targetStage, std::shared_ptr<GraphicsProgram> graphicsProgram);

    void bakeSkeletonOverlay(Model* model, const std::vector<glm::mat4> &jointTransforms,
                             const glm::mat4 &previewCameraMatrix, const glm::mat4 &previewProjectionMatrix,
                             std::shared_ptr<GraphicsProgram> graphicsProgram);

    //needs baseDirection for the pitch clamp, see the .cpp for why
    void applyOrbitDragToState(OrbitState &orbit, const glm::vec3 &baseDirection, float dragDeltaX, float dragDeltaY);

public:
    PreviewRenderer(World* world, ImGuiHelper* imgGuiHelper);
    ~PreviewRenderer();

    ImGuiImageWrapper* renderBonePreview(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram);

    //nearest bone within hitRadius pixels, -1 if none. Kept here so boneScreenPositions stays private
    int32_t findClosestBoneAtPixel(float pixelX, float pixelY, float hitRadius) const;

    //dragDeltaX/Y are this frame's raw ImGui mouse delta
    void applyBonePreviewOrbitDrag(float dragDeltaX, float dragDeltaY);
    void applyAssetPreviewOrbitDrag(float dragDeltaX, float dragDeltaY);

    //always valid once constructed, shown unconditionally every frame like before extraction
    ImGuiImageWrapper* getAssetPreviewWrapper() const {
        return wrapper;
    }

    //runs the load/cache state machine, renders into the wrapper above. True means still waiting on CPU load
    bool updateAssetPreview(const std::string &assetFullPath, const glm::vec3 &previewPosition, std::shared_ptr<GraphicsProgram> graphicsProgram);

    void finalizeOffscreenModelPreviews(std::shared_ptr<GraphicsProgram> graphicsProgram);

    //editor's own preview targets aren't part of the render pipeline, so the debug listbox needs them explicitly
    [[nodiscard]] std::shared_ptr<Texture> getBackgroundColorTexture() const {
        return colorTexture;
    }
    [[nodiscard]] std::shared_ptr<Texture> getBackgroundDepthTexture() const {
        return depthTexture;
    }
    [[nodiscard]] std::shared_ptr<Texture> getBonePreviewColorTexture() const {
        return bonePreview.colorTexture;
    }
    [[nodiscard]] std::shared_ptr<Texture> getBonePreviewDepthTexture() const {
        return bonePreview.depthTexture;
    }
};


#endif //LIMONENGINE_PREVIEWRENDERER_H
