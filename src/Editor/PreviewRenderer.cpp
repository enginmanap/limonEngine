//
// Extracted from Editor.
//

#include <limits>
#include <cmath>
#include <algorithm>
#include <glm/gtc/quaternion.hpp>
#include "PreviewRenderer.h"
#include "World.h"
#include "GameObjects/Model.h"
#include "ImGuiHelper.h"
#include "Assets/AssetManager.h"
#include "limonAPI/Graphics/GraphicsInterface.h"
#include "Graphics/GraphicsPipelineStage.h"
#include "Graphics/Texture.h"

PreviewRenderer::PreviewRenderer(World* world, ImGuiHelper* imgGuiHelper) : world(world), imgGuiHelper(imgGuiHelper) {
    backgroundRenderStage = std::make_unique<GraphicsPipelineStage>(world->graphicsWrapper, 640,480,"","",true,true,true,false,false);
    colorTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, 640, 480);
    colorTexture->setName("EditorColorTexture");
    colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    depthTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH, GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::FLOAT, 640, 480);
    depthTexture->setName("EditorDepthTexture");
    colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    backgroundRenderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::COLOR0, colorTexture, true);
    backgroundRenderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, depthTexture, true);
    wrapper = new ImGuiImageWrapper();
    //set once, not after every render. colorTexture never changes identity, so this stays valid even before
    //the first render happens
    wrapper->texture = colorTexture;
    wrapper->layer = 0;

    modelPreview.renderStage = std::make_unique<GraphicsPipelineStage>(world->graphicsWrapper, DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT,"","",true,true,true,false,false);
    modelPreview.colorTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT);
    modelPreview.colorTexture->setName("EditorBonePreviewColorTexture");
    modelPreview.colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    modelPreview.depthTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH, GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::FLOAT, DEFAULT_PREVIEW_WIDTH, DEFAULT_PREVIEW_HEIGHT);
    modelPreview.depthTexture->setName("EditorBonePreviewDepthTexture");
    modelPreview.renderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::COLOR0, modelPreview.colorTexture, true);
    modelPreview.renderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, modelPreview.depthTexture, true);
    modelPreview.wrapper = new ImGuiImageWrapper();
    modelPreview.rigId = world->getNextRigId();
    assetPreviewPlayerBlockBuffer = world->graphicsWrapper->createUniformBuffer();
    modelPreview.playerBlockBuffer = world->graphicsWrapper->createUniformBuffer();

    // We wanna render the bone overlay to the preview texture. We can't do it clearly with main context, so we need
    // a secondary imgui context
    ImGuiContext* mainImGuiContext = ImGui::GetCurrentContext();
    ImFontAtlas* sharedFontAtlas = ImGui::GetIO().Fonts;
    modelPreview.imGuiContext = ImGui::CreateContext(sharedFontAtlas);
    ImGui::SetCurrentContext(mainImGuiContext);
}

PreviewRenderer::~PreviewRenderer() {
    ImGui::DestroyContext(modelPreview.imGuiContext);
    delete wrapper;
    delete modelPreview.wrapper;
    world->graphicsWrapper->deleteUniformBuffer(assetPreviewPlayerBlockBuffer);
    world->graphicsWrapper->deleteUniformBuffer(modelPreview.playerBlockBuffer);
    if (comparisonLevelTarget.playerBlockBuffer != 0) {
        world->graphicsWrapper->deleteUniformBuffer(comparisonLevelTarget.playerBlockBuffer);
    }
    if (comparisonOriginalTarget.playerBlockBuffer != 0) {
        world->graphicsWrapper->deleteUniformBuffer(comparisonOriginalTarget.playerBlockBuffer);
    }
    //LRU eviction only deletes the oldest entry when the cache is full, so anything still queued when the
    //editor goes away was never swept until now
    for (Model* queuedModel : modelQueue) {
        delete queuedModel;
    }
    modelQueueCullingEntries.clear();
}

Model* PreviewRenderer::getModelAndMoveToEnd(const std::string& modelFilePath) {
    for(auto iter = modelQueue.begin(); iter != modelQueue.end(); ++iter) {
        Model* model = *iter;
        if (model->getName() == modelFilePath + "_" + std::to_string(model->getWorldObjectID())) {
            modelQueue.erase(iter);
            modelQueue.emplace_back(model);
            return model;
        }
    }
    return nullptr;
}

Model* PreviewRenderer::createRenderAndAddModelToLRU(const std::string &modelFileName, const glm::vec3 &newObjectPosition, std::shared_ptr<GraphicsProgram> graphicsProgram) {
    uint32_t newWorldObjectId;
    if(modelQueue.size() >= MAX_PRELOAD_MODEL_COUNT_EDITOR) {
        newWorldObjectId = modelQueue[0]->getWorldObjectID();
        Model* evictedModel = modelQueue[0];
        delete evictedModel;
        modelQueueCullingEntries.erase(evictedModel);
        modelQueue.erase(modelQueue.begin());
    } else {
        newWorldObjectId = (*modelIdSet.begin());
        modelIdSet.erase(modelIdSet.begin());
    }

    std::unique_ptr<CullingEntry> cullingEntry = std::make_unique<CullingEntry>();
    Model* model = new Model(newWorldObjectId, world->assetManager, modelFileName, cullingEntry.get());// FIXME this will cause gaps, we should reserve and reuse
    modelQueueCullingEntries[model] = std::move(cullingEntry);
    modelQueue.push_back(model);
    setTransformToModel(model, newObjectPosition);
    renderSelectedObject(model, graphicsProgram);
    return model;
}

void PreviewRenderer::setTransformToModel(Model *model, const glm::vec3 &newObjectPosition) {
    //used to reverse-orient the model to face the live camera and rescale it to fit. Not needed anymore,
    //renderSelectedObject fits its own dedicated camera to whatever the model's real proportions are
    model->getTransformation()->setTransformations(newObjectPosition, glm::vec3(1.0f, 1.0f, 1.0f),
                                                    glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

void PreviewRenderer::beginOffscreenModelPreview(GraphicsPipelineStage* targetStage, std::shared_ptr<GraphicsProgram> graphicsProgram) {
    //set unconditionally, cheap, so each draw is self-sufficient no matter what earlier ones did to this
    //uniform. The flag below is only about end-of-frame cleanup
    graphicsProgram->setUniform("renderModelIMGUI", 1);
    offscreenPreviewRenderedThisFrame = true;
    targetStage->activate(true);
}

void PreviewRenderer::beginPreviewCamera(uint32_t playerBlockBuffer, const glm::vec3 &cameraPosition, const glm::mat4 &cameraMatrix, const glm::mat4 &projectionMatrix) {
    world->fillPlayerBlock(previewPlayerBlock, cameraPosition, cameraMatrix, projectionMatrix);
    world->graphicsWrapper->writeUniformBuffer(playerBlockBuffer, previewPlayerBlock);
    world->graphicsWrapper->bindUniformBuffer(playerBlockBuffer, GraphicsInterface::PLAYER_BLOCK_BINDING);
}

void PreviewRenderer::endPreviewCamera() {
    world->graphicsWrapper->bindUniformBuffer(world->frameResourceHandles->playerBlockBuffer, GraphicsInterface::PLAYER_BLOCK_BINDING);
}

void PreviewRenderer::finalizeOffscreenModelPreviews(std::shared_ptr<GraphicsProgram> graphicsProgram) {
    if (offscreenPreviewRenderedThisFrame) {
        graphicsProgram->setUniform("renderModelIMGUI", 0);
        world->renderPipeline->reActivateLastStage();
        offscreenPreviewRenderedThisFrame = false;
    }
}

//world-space bounds of a model's mesh, from the asset's bind-pose bounding box transformed by renderTransform.
//Takes the transform explicitly rather than reading getWorldTransform() itself, in case a caller ever needs a
//different one (both current callers just pass getWorldTransform() unchanged)
static void computeModelWorldBounds(Model *model, const glm::mat4 &renderTransform, glm::vec3 &outMin, glm::vec3 &outMax) {
    const glm::vec3 localBoundsMin = model->getModelAsset()->getBoundingBoxMin();
    const glm::vec3 localBoundsMax = model->getModelAsset()->getBoundingBoxMax();
    outMin = glm::vec3(std::numeric_limits<float>::max());
    outMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
        glm::vec3 localCorner((cornerIndex & 1) ? localBoundsMax.x : localBoundsMin.x,
                               (cornerIndex & 2) ? localBoundsMax.y : localBoundsMin.y,
                               (cornerIndex & 4) ? localBoundsMax.z : localBoundsMin.z);
        glm::vec3 worldCorner = glm::vec3(renderTransform * glm::vec4(localCorner, 1.0f));
        outMin = glm::min(outMin, worldCorner);
        outMax = glm::max(outMax, worldCorner);
    }
}

//camera looking at the box center from viewDirection, at the distance that fits all 8 corners in the given FOV
//plus a little headroom. Depth has to count here, since both previews can be orbited to a side view now. Fits
//every corner rather than just width/height or a diagonal radius, both of which over- or under-shoot depending
//on the box's shape. Distance per corner falls out of tan(halfFov) = lateralComponent / depth, solved directly
//since depth is linear in distance; the fit uses whichever corner/axis needs the most
static glm::mat4 computeFittedPreviewCameraMatrix(const glm::vec3 &boundsMin, const glm::vec3 &boundsMax,
                                                   float fovYRadians, float aspect, const glm::vec3 &viewDirection,
                                                   float zoomFactor, glm::vec3 &outCameraPosition) {
    glm::vec3 center = (boundsMin + boundsMax) * 0.5f;
    glm::vec3 extents = (boundsMax - boundsMin) * 0.5f;
    if (glm::length(extents) < 0.005f) {
        extents = glm::vec3(0.5f);//degenerate/zero-size bounds guard
    }

    const glm::vec3 direction = glm::normalize(viewDirection);
    const glm::vec3 forward = -direction;//the camera's actual view direction is the opposite way
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    const glm::vec3 up = glm::cross(right, forward);

    const float halfFovY = fovYRadians * 0.5f;
    const float halfFovX = std::atan(std::tan(halfFovY) * aspect);

    float requiredDistance = 0.0f;
    for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
        glm::vec3 cornerOffset((cornerIndex & 1) ? extents.x : -extents.x,
                                (cornerIndex & 2) ? extents.y : -extents.y,
                                (cornerIndex & 4) ? extents.z : -extents.z);
        float alongDirection = glm::dot(cornerOffset, direction);
        float rightComponent = std::abs(glm::dot(cornerOffset, right));
        float upComponent = std::abs(glm::dot(cornerOffset, up));
        requiredDistance = std::max(requiredDistance, alongDirection + rightComponent / std::tan(halfFovX));
        requiredDistance = std::max(requiredDistance, alongDirection + upComponent / std::tan(halfFovY));
    }
    //10% headroom so the model doesn't sit exactly edge-to-edge against the preview borders
    requiredDistance *= 1.1f;
    requiredDistance *= zoomFactor;

    outCameraPosition = center + direction * requiredDistance;
    return glm::lookAt(outCameraPosition, center, worldUp);
}

//rotates baseDirection by yaw around world up, then pitch around the resulting right axis. Takes plain floats
//instead of OrbitState since it's a free function and OrbitState is private. Doesn't clamp, callers do that
static glm::vec3 applyOrbit(const glm::vec3 &baseDirection, float yaw, float pitch) {
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 yawedDirection = glm::angleAxis(yaw, worldUp) * baseDirection;
    const glm::vec3 pitchAxis = glm::normalize(glm::cross(yawedDirection, worldUp));
    return glm::angleAxis(pitch, pitchAxis) * yawedDirection;
}

const glm::vec3 PreviewRenderer::MODEL_PREVIEW_BASE_DIRECTION(0.0f, 0.5f, 2.5f);
const glm::vec3 PreviewRenderer::ASSET_PREVIEW_BASE_DIRECTION(-0.6f, 0.7f, 1.8f);

void PreviewRenderer::applyOrbitDragToState(OrbitState &orbit, const glm::vec3 &baseDirection, float dragDeltaX, float dragDeltaY) {
    constexpr float sensitivity = 0.01f;//radians per pixel, tuned by feel
    orbit.yaw -= dragDeltaX * sensitivity;
    float candidatePitch = orbit.pitch - dragDeltaY * sensitivity;

    //can't just clamp candidatePitch to a fixed range, that's relative to baseDirection which isn't horizontal
    //itself (bone preview's is already tilted up ~11 degrees), so base tilt plus offset can still cross the
    //pole. Clamp the resulting direction's own elevation instead, same idea as FreeMovingPlayer::rotate
    glm::vec3 candidateDirection = glm::normalize(applyOrbit(baseDirection, orbit.yaw, candidatePitch));
    constexpr float maxVerticalComponent = 0.99f;
    if (std::abs(candidateDirection.y) <= maxVerticalComponent) {
        orbit.pitch = candidatePitch;
    }
    //else just drop this frame's pitch delta, yaw still applies
}

void PreviewRenderer::applyModelPreviewOrbitDrag(float dragDeltaX, float dragDeltaY) {
    applyOrbitDragToState(modelPreview.orbit, MODEL_PREVIEW_BASE_DIRECTION, dragDeltaX, dragDeltaY);
}

void PreviewRenderer::applyAssetPreviewOrbitDrag(float dragDeltaX, float dragDeltaY) {
    applyOrbitDragToState(assetPreviewOrbit, ASSET_PREVIEW_BASE_DIRECTION, dragDeltaX, dragDeltaY);
}

void PreviewRenderer::applyZoomToState(OrbitState &orbit, float wheelDelta, float maxZoomFactor) {
    constexpr float zoomStepPerNotch = 0.9f;//tuned by feel, scroll forward = closer, same idea as FlameGraph's zoom
    //below this, requiredDistance*zoomFactor hits zero and glm::lookAt's normalize(center-eye) goes to NaN
    constexpr float minZoomFactor = 0.02f;
    orbit.zoomFactor *= std::pow(zoomStepPerNotch, wheelDelta);
    orbit.zoomFactor = std::clamp(orbit.zoomFactor, minZoomFactor, maxZoomFactor);
}

void PreviewRenderer::applyModelPreviewZoom(float wheelDelta) {
    //judging a LOD means pulling back to see the whole thing and pushing in on one edge, so the ceiling is
    //well past the fit distance. The skeleton overlay's own 1.15 is applied where it is baked, not here
    applyZoomToState(modelPreview.orbit, wheelDelta, 4.0f);
}

void PreviewRenderer::applyAssetPreviewZoom(float wheelDelta) {
    applyZoomToState(assetPreviewOrbit, wheelDelta, 1.15f);
}

void PreviewRenderer::renderSelectedObject(Model* model, std::shared_ptr<GraphicsProgram> graphicsProgram) {
    glm::vec3 boundsMin, boundsMax;
    computeModelWorldBounds(model, model->getTransformation()->getWorldTransform(), boundsMin, boundsMax);
    glm::vec3 previewCameraPosition;
    const float aspect = static_cast<float>(colorTexture->getWidth()) / static_cast<float>(colorTexture->getHeight());
    //mild top-left overview, like a product photo, not the bone preview's near-frontal shot. Assets vary
    //wildly in shape so a bit of top+side reads better than a flat front view
    const glm::vec3 assetPreviewViewDirection = applyOrbit(ASSET_PREVIEW_BASE_DIRECTION, assetPreviewOrbit.yaw, assetPreviewOrbit.pitch);
    glm::mat4 previewCameraMatrix = computeFittedPreviewCameraMatrix(boundsMin, boundsMax, glm::radians(60.0f), aspect,
                                                                     assetPreviewViewDirection, assetPreviewOrbit.zoomFactor, previewCameraPosition);
    glm::mat4 previewProjectionMatrix = glm::perspective(glm::radians(60.0f), aspect, 0.1f,
                                                          glm::length(boundsMax - boundsMin) * 20.0f + 10.0f);

    beginPreviewCamera(assetPreviewPlayerBlockBuffer, previewCameraPosition, previewCameraMatrix, previewProjectionMatrix);
    beginOffscreenModelPreview(backgroundRenderStage.get(), graphicsProgram);
    //preview models are not in the world, so we write their slot ourselves. It goes up with the next frame's
    //uploadFrameData, so the first frame of a new preview has no transform and draws nothing
    world->frameData.setModelTransform(model->getWorldObjectID(), model->getTransformation()->getWorldTransform());
    model->convertToRenderList(0, 0).render(world->graphicsWrapper, graphicsProgram, true);

    endPreviewCamera();
}

bool PreviewRenderer::updateAssetPreview(const std::string &assetFullPath, const glm::vec3 &previewPosition, std::shared_ptr<GraphicsProgram> graphicsProgram) {
    if (assetFullPath != lastPreviewedAssetPath) {
        //browsed selection changed: drop any orbit applied to the previous asset rather than carrying it over
        assetPreviewOrbit = OrbitState{};
        lastPreviewedAssetPath = assetFullPath;
    }
    if (modelIdSet.empty() && modelQueue.empty()) {
        for(size_t i = 0; i < MAX_PRELOAD_MODEL_COUNT_EDITOR; ++i) {
            modelIdSet.insert(world->getNextObjectID());
        }
    }
    if ((modelAssetsWaitingCPULoad.find(assetFullPath) == modelAssetsWaitingCPULoad.end()
            && world->assetManager->isLoaded({assetFullPath})) || modelAssetsPreloaded.find(assetFullPath) != modelAssetsPreloaded.end()) {
        // Preloaded case
        Model* model = getModelAndMoveToEnd(assetFullPath);
        if(model == nullptr) {
            createRenderAndAddModelToLRU(assetFullPath, previewPosition, graphicsProgram);
            if(modelAssetsPreloaded.count(assetFullPath)) {
                // Only release the editor-side preload reference; if the asset was
                // already in the world we have no extra counter to balance.
                modelAssetsPreloaded.erase(assetFullPath);
                world->assetManager->freeAsset({assetFullPath});
            }
        } else {
            setTransformToModel(model, previewPosition);
            renderSelectedObject(model, graphicsProgram);
        }
        return false;
    } else if(modelAssetsWaitingCPULoad.find(assetFullPath) != modelAssetsWaitingCPULoad.end()) {
        if(modelAssetsWaitingCPULoad[assetFullPath]->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
            world->assetManager->partialLoadGPUSide(modelAssetsWaitingCPULoad[assetFullPath]);
            modelAssetsPreloaded[assetFullPath] = modelAssetsWaitingCPULoad[assetFullPath];
            modelAssetsWaitingCPULoad.erase(assetFullPath);
            return false;
        } else {
            return true;
        }
    } else {
        //Requesting Load case
        modelAssetsWaitingCPULoad[assetFullPath] = world->assetManager->partialLoadAssetAsync<ModelAsset>({assetFullPath});
        return true;
    }
}

//world position to local pixel space, 0,0 top-left, y down, same convention as PositionIMGUI
//Since we use a dedicated context, we don't need to hack together some way to actually position them
static bool projectWorldPositionToLocalPixel(const glm::vec3 &worldPosition, const glm::mat4 &cameraMatrix,
                                              const glm::mat4 &projectionMatrix, float width, float height,
                                              ImVec2 &outPixel) {
    glm::vec4 clipPosition = projectionMatrix * cameraMatrix * glm::vec4(worldPosition, 1.0f);
    if (clipPosition.w <= 0.0f) {
        return false;
    }
    glm::vec3 ndcPosition = glm::vec3(clipPosition) / clipPosition.w;
    float u = (ndcPosition.x * 0.5f) + 0.5f;
    float v = 1.0f - ((ndcPosition.y * 0.5f) + 0.5f);
    outPixel.x = u * width;
    outPixel.y = v * height;
    return true;
}

void PreviewRenderer::bakeSkeletonOverlay(Model* model, const std::vector<glm::mat4> &jointTransforms,
                                  const glm::mat4 &previewCameraMatrix, const glm::mat4 &previewProjectionMatrix,
                                  std::shared_ptr<GraphicsProgram> graphicsProgram) {
    const float width = static_cast<float>(modelPreview.width);
    const float height = static_cast<float>(modelPreview.height);

    // get the previous context, and then switch
    ImGuiContext* previousContext = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(modelPreview.imGuiContext);
    ImGuiIO &previewIO = ImGui::GetIO();
    previewIO.DisplaySize = ImVec2(width, height);
    previewIO.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    previewIO.DeltaTime = 1.0f / TICK_PER_SECOND;//dummy; this context is draw-only and never advances real time
    ImGui::NewFrame();

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const glm::mat4 worldTransform = model->getTransformation()->getWorldTransform();
    const std::vector<std::pair<uint32_t, uint32_t>> boneEdges = model->getModelAsset()->getBoneHierarchyEdges();

    //jointTransforms is sized to NR_BONE, not this skeleton's real bone count, unused slots default to identity
    //and would draw a phantom joint at the origin if we iterated them directly
    std::set<uint32_t> realBoneIDs;
    for (const auto &edge : boneEdges) {
        realBoneIDs.insert(edge.first);
        realBoneIDs.insert(edge.second);
    }

    int32_t selectedBoneID = model->getSelectedBoneID();

    modelPreview.boneScreenPositions.clear();

    for (const auto &edge : boneEdges) {
        uint32_t childBoneID = edge.first;
        uint32_t parentBoneID = edge.second;
        if (childBoneID >= jointTransforms.size() || parentBoneID >= jointTransforms.size()) {
            continue;
        }
        glm::vec3 childWorldPos = glm::vec3((worldTransform * jointTransforms[childBoneID])[3]);
        glm::vec3 parentWorldPos = glm::vec3((worldTransform * jointTransforms[parentBoneID])[3]);
        ImVec2 childPixel, parentPixel;
        if (projectWorldPositionToLocalPixel(childWorldPos, previewCameraMatrix, previewProjectionMatrix, width, height, childPixel) &&
            projectWorldPositionToLocalPixel(parentWorldPos, previewCameraMatrix, previewProjectionMatrix, width, height, parentPixel)) {
            drawList->AddLine(parentPixel, childPixel, IM_COL32(255, 220, 0, 200), 2.0f);
        }
    }
    for (uint32_t boneID : realBoneIDs) {
        if (boneID >= jointTransforms.size()) {
            continue;
        }
        glm::vec3 boneWorldPos = glm::vec3((worldTransform * jointTransforms[boneID])[3]);
        ImVec2 bonePixel;
        if (!projectWorldPositionToLocalPixel(boneWorldPos, previewCameraMatrix, previewProjectionMatrix, width, height, bonePixel)) {
            continue;
        }
        bool isSelected = (static_cast<int32_t>(boneID) == selectedBoneID);
        drawList->AddCircleFilled(bonePixel, isSelected ? 6.0f : 3.5f, isSelected ? IM_COL32(255, 60, 60, 255) : IM_COL32(80, 200, 255, 220));
        modelPreview.boneScreenPositions.emplace_back(boneID, bonePixel);
    }

    ImGui::Render();

    // we wanna render imgui render list, not model, flag it
    graphicsProgram->setUniform("renderModelIMGUI", 0);
    imgGuiHelper->RenderDrawLists(graphicsProgram);

    ImGui::SetCurrentContext(previousContext);
}

void PreviewRenderer::ensureModelPreviewTarget(uint32_t width, uint32_t height) {
    width = std::clamp(width, MIN_PREVIEW_SIZE, MAX_PREVIEW_SIZE);
    height = std::clamp(height, MIN_PREVIEW_SIZE, MAX_PREVIEW_SIZE);
    if (width == modelPreview.width && height == modelPreview.height) {
        return;
    }
    //neither the textures nor the stage can be resized, so the whole target is rebuilt. The wrapper is handed
    //the new texture at the end of every render, so nothing keeps pointing at the old one
    modelPreview.width = width;
    modelPreview.height = height;
    modelPreview.renderStage = std::make_unique<GraphicsPipelineStage>(world->graphicsWrapper, width, height, "", "", true, true, true, false, false);
    modelPreview.colorTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, width, height);
    modelPreview.colorTexture->setName("EditorModelPreviewColorTexture");
    modelPreview.colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    modelPreview.depthTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH, GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::FLOAT, width, height);
    modelPreview.depthTexture->setName("EditorModelPreviewDepthTexture");
    modelPreview.renderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::COLOR0, modelPreview.colorTexture, true);
    modelPreview.renderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, modelPreview.depthTexture, true);
}

void PreviewRenderer::renderModelIntoTarget(Model* model, uint32_t lodLevel, GraphicsPipelineStage* targetStage,
                                            uint32_t width, uint32_t height, const OrbitState &orbit, int32_t rigIdOverride, uint32_t playerBlockBuffer,
                                            std::shared_ptr<GraphicsProgram> graphicsProgram,
                                            glm::mat4 &outCameraMatrix, glm::mat4 &outProjectionMatrix) {
    // We want to render the model from the front. But there are 2 sets of information used for this:
    // 1) Object transform -> Used for the real object, in a texture, read from multiple threads
    // 2) Camera transform(s) -> Used for everything, but in UBO, single threaded
    // To avoid race condition risk, we will update the camera transforms.
    //
    // model->getAabbMin()/getAabbMax() is out: it returns the model's live gameplay animation state, we want wall time
    // So we get the asset AABB and use it as a proxy
    glm::vec3 aabbMin, aabbMax;
    //plain transform on purpose, a centerOffset compensation was tried here and pivoted the preview toward the
    //head instead of fixing it
    computeModelWorldBounds(model, model->getTransformation()->getWorldTransform(), aabbMin, aabbMax);

    // We can just estimate the far size, it doesn't really matter
    float farPlaneRadius = glm::length(aabbMax - aabbMin) * 0.5f;
    if (farPlaneRadius < 0.01f) {
        farPlaneRadius = 1.0f;//degenerate/zero-size AABB guard
    }

    const float fovYRadians = glm::radians(60.0f);
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    glm::vec3 previewCameraPosition;
    //near-frontal, a bit above, not the asset preview's top-left angle
    const glm::vec3 modelPreviewViewDirection = applyOrbit(MODEL_PREVIEW_BASE_DIRECTION, orbit.yaw, orbit.pitch);
    outCameraMatrix = computeFittedPreviewCameraMatrix(aabbMin, aabbMax, fovYRadians, aspect,
                                                       modelPreviewViewDirection, orbit.zoomFactor, previewCameraPosition);
    outProjectionMatrix = glm::perspective(fovYRadians, aspect, 0.1f, farPlaneRadius * 20.0f + 10.0f);

    beginPreviewCamera(playerBlockBuffer, previewCameraPosition, outCameraMatrix, outProjectionMatrix);
    beginOffscreenModelPreview(targetStage, graphicsProgram);
    // We want animation, so we should not force Not animated, unlike the add object preview
    model->convertToRenderList(lodLevel, 0, rigIdOverride).render(world->graphicsWrapper, graphicsProgram, false);
    //the overlay projects with the matrices handed back, so the world's block can go back now
    endPreviewCamera();
}

uint32_t PreviewRenderer::clampLodLevel(const Model* model, int32_t forcedLodLevel) {
    //a level the model does not have would draw someone else's index range
    if (forcedLodLevel <= 0 || model->getModelAsset()->getLodLadder().getMeshLodCount() <= 1) {
        return 0;
    }
    return std::min((uint32_t) forcedLodLevel, model->getModelAsset()->getLodLadder().getMeshLodCount() - 1);
}

float PreviewRenderer::poseModelPreview(Model* model, std::vector<glm::mat4> &outJointTransforms) {
    if (model->getWorldObjectID() != modelPreview.modelObjectID || model->getAnimationName() != modelPreview.animationName) {
        //new model or animation: restart playback and reset orbit, a fresh target shouldn't inherit the last
        //one's rotation
        modelPreview.modelObjectID = model->getWorldObjectID();
        modelPreview.animationName = model->getAnimationName();
        modelPreview.startWallTime = world->wallTime;
        modelPreview.pausedTime = 0.0f;
        modelPreview.orbit = OrbitState{};
    }
    float previewAnimationTime = modelPreview.paused ? modelPreview.pausedTime
                                                     : static_cast<float>(world->wallTime - modelPreview.startWallTime);
    if (model->getModelAsset()->isAnimated()) {
        // The model getTransform does not check the vector size, we need to resize here.
        std::vector<glm::mat4> skinningMatrices(NR_BONE);
        // get transform and get joint transform can be combined, but we don't want to, because that would change the hot path
        // logic for editor. This split is intentional
        model->getModelAsset()->getTransform(previewAnimationTime, true, model->getAnimationName(), skinningMatrices);
        model->getModelAsset()->getJointTransforms(previewAnimationTime, true, model->getAnimationName(), outJointTransforms);
        world->frameData.setBoneTransforms(modelPreview.rigId, skinningMatrices);
        //goes up with the next frame's uploadFrameData, so the preview shows last frame's pose
    }
    return previewAnimationTime;
}

ImGuiImageWrapper* PreviewRenderer::renderModelPreview(Model* model, int32_t forcedLodLevel, uint32_t requestedWidth, uint32_t requestedHeight,
                                                       ModelPreviewPlayback &playback, std::shared_ptr<GraphicsProgram> graphicsProgram) {
    ensureModelPreviewTarget(requestedWidth, requestedHeight);
    const bool animated = model->getModelAsset()->isAnimated();
    float duration = animated ? model->getModelAsset()->getAnimationDurationMilliseconds(model->getAnimationName()) : 0.0f;
    if (playback.paused) {
        modelPreview.paused = true;
        modelPreview.pausedTime = playback.timeMilliseconds;
    } else if (modelPreview.paused) {
        //resumes from the frame it was paused or scrubbed to
        modelPreview.paused = false;
        modelPreview.startWallTime = world->wallTime - static_cast<uint64_t>(std::max(modelPreview.pausedTime, 0.0f));
    }
    std::vector<glm::mat4> jointTransforms(NR_BONE);
    float shownTime = poseModelPreview(model, jointTransforms);
    playback.durationMilliseconds = duration;
    if (!playback.paused) {
        playback.timeMilliseconds = duration > 0.0f ? std::fmod(shownTime, duration) : 0.0f;
    } else if (modelPreview.pausedTime != playback.timeMilliseconds) {
        playback.timeMilliseconds = modelPreview.pausedTime;//a new model or animation started over
    }

    glm::mat4 previewCameraMatrix, previewProjectionMatrix;
    renderModelIntoTarget(model, clampLodLevel(model, forcedLodLevel), modelPreview.renderStage.get(),
                          modelPreview.width, modelPreview.height, modelPreview.orbit,
                          static_cast<int32_t>(modelPreview.rigId), modelPreview.playerBlockBuffer, graphicsProgram,
                          previewCameraMatrix, previewProjectionMatrix);

    if (animated && playback.showSkeleton) {
        bakeSkeletonOverlay(model, jointTransforms, previewCameraMatrix, previewProjectionMatrix, graphicsProgram);
    } else {
        modelPreview.boneScreenPositions.clear();//nothing to hit test, and stale positions would select a bone
    }

    modelPreview.wrapper->texture = modelPreview.colorTexture;
    modelPreview.wrapper->layer = 0;
    return modelPreview.wrapper;
}

int32_t PreviewRenderer::findClosestBoneAtPixel(float pixelX, float pixelY, float hitRadius) const {
    float closestDistanceSquared = hitRadius * hitRadius;
    int32_t closestBoneID = -1;
    for (const auto &boneScreenPosition : modelPreview.boneScreenPositions) {
        float deltaX = boneScreenPosition.second.x - pixelX;
        float deltaY = boneScreenPosition.second.y - pixelY;
        float distanceSquared = deltaX * deltaX + deltaY * deltaY;
        if (distanceSquared <= closestDistanceSquared) {
            closestDistanceSquared = distanceSquared;
            closestBoneID = static_cast<int32_t>(boneScreenPosition.first);
        }
    }
    return closestBoneID;
}

void PreviewRenderer::ensureComparisonTarget(ComparisonTarget &target, uint32_t size, const std::string &textureName) {
    if (target.size == size && target.colorTexture != nullptr) {
        return;
    }
    target.size = size;
    target.renderStage = std::make_unique<GraphicsPipelineStage>(world->graphicsWrapper, size, size, "", "", true, true, true, false, false);
    target.colorTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::RGBA, GraphicsInterface::FormatTypes::RGBA, GraphicsInterface::DataTypes::UNSIGNED_BYTE, size, size);
    target.colorTexture->setName(textureName);
    //nearest, because the point is to see the level's own pixels magnified, not a smoothed version of them
    target.colorTexture->setFilterMode(GraphicsInterface::FilterModes::NEAREST);
    target.depthTexture = std::make_shared<Texture>(world->graphicsWrapper, GraphicsInterface::TextureTypes::T2D, GraphicsInterface::InternalFormatTypes::DEPTH, GraphicsInterface::FormatTypes::DEPTH, GraphicsInterface::DataTypes::FLOAT, size, size);
    target.depthTexture->setName(textureName + "Depth");
    target.renderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::COLOR0, target.colorTexture, true);
    target.renderStage->setOutput(GraphicsInterface::FrameBufferAttachPoints::DEPTH, target.depthTexture, true);
    if (target.wrapper == nullptr) {
        target.wrapper = new ImGuiImageWrapper();
    }
    if (target.playerBlockBuffer == 0) {
        target.playerBlockBuffer = world->graphicsWrapper->createUniformBuffer();
    }
    target.wrapper->texture = target.colorTexture;
    target.wrapper->layer = 0;
}

LodComparisonImages PreviewRenderer::renderLodComparison(Model* model, int32_t forcedLodLevel, uint32_t designSizePixels,
                                                         std::shared_ptr<GraphicsProgram> graphicsProgram) {
    LodComparisonImages images;
    uint32_t size = std::clamp(designSizePixels, MIN_COMPARISON_SIZE, MAX_COMPARISON_SIZE);
    ensureComparisonTarget(comparisonLevelTarget, size, "EditorLodComparisonLevel");
    ensureComparisonTarget(comparisonOriginalTarget, size, "EditorLodComparisonOriginal");

    std::vector<glm::mat4> jointTransforms(NR_BONE);
    poseModelPreview(model, jointTransforms);
    //the same orbit for both, a comparison of two camera angles would say nothing
    glm::mat4 cameraMatrix, projectionMatrix;
    renderModelIntoTarget(model, clampLodLevel(model, forcedLodLevel), comparisonLevelTarget.renderStage.get(),
                          size, size, modelPreview.orbit, static_cast<int32_t>(modelPreview.rigId), comparisonLevelTarget.playerBlockBuffer, graphicsProgram,
                          cameraMatrix, projectionMatrix);
    renderModelIntoTarget(model, 0, comparisonOriginalTarget.renderStage.get(),
                          size, size, modelPreview.orbit, static_cast<int32_t>(modelPreview.rigId), comparisonOriginalTarget.playerBlockBuffer, graphicsProgram,
                          cameraMatrix, projectionMatrix);
    images.levelImage = comparisonLevelTarget.wrapper;
    images.originalImage = comparisonOriginalTarget.wrapper;
    return images;
}
