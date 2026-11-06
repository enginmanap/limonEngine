#include "VisibilityManager.h"
#include "World.h"
#include "Camera/Camera.h"
#include "Graphics/GraphicsPipeline.h"
#include "GameObjects/Model.h"
#include "GameObjects/ModelGroup.h"
#include "GameObjects/Players/Player.h"
#include "Utils/HardCodedTags.h"
#include "../Profiler/ProfilerMacros.h"

static_assert(MeshAsset::LOD_MAX_LEVEL_COUNT <= 256, "VisibilityRequest keeps the previous LOD level of each object in a uint8_t");

VisibilityManager::VisibilityManager(World* world) : world(world) {
    OptionsUtil::Options::Option<bool> multiThreadCullingOption = world->options->getOption<bool>(HASH("performance_multiThreadedCulling"));
    multiThreadedCulling = multiThreadCullingOption.getOrDefault(true);
}

VisibilityManager::~VisibilityManager() {
    stop();
}

void VisibilityManager::stop() {
    for (auto &item: visibilityThreadPool) {
        item.first->running = false;
    }
    for (auto &item: visibilityThreadPool) {
        item.first->visibilityLatch.signal();//releases only this thread, which then observes running==false and returns
        delete item.second;
        delete item.first;
    }
    visibilityThreadPool.clear();
}

void VisibilityManager::start() {
    if(multiThreadedCulling) {
        if (visibilityThreadPool.empty()) {
            visibilityThreadPool = occlusionThreadManager();
        }
    } else {
        if(visibilityThreadPool.empty()) {
            for (auto &cameraVisibility: cullingResults) {
                VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->cullingEntries, world->getStartingPlayer(), cameraVisibility.second, world->currentPlayer->getPosition(), world->options, &cullingBarrier, cameraVisibility.first->getName());
                visibilityThreadPool[request] = nullptr;
            }
        }
    }
}

void VisibilityManager::update() {
    PROFILE_VISIBILITY("VisibilityManager::update");
    fillVisibleObjectsUsingTags();
}

void VisibilityManager::onPipelineChange() {
    resetTagsAndRefillCulling();
}

std::unordered_map<Camera*, std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>*>& VisibilityManager::getCullingResults() {
    return cullingResults;
}

void VisibilityManager::addCamera(Camera* camera) {
    auto* tagMap = new std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>();
    cullingResults.insert(std::make_pair(camera, tagMap));

    // Populate render-tag sets from the current pipeline whenever one is loaded. This must NOT be gated on
    // the thread pool being non-empty: a camera swapped in after onPipelineChange() but before start()
    // (e.g. the player camera switching to an orthographic type at load) would otherwise keep an empty
    // tag map and produce no render lists (black scene). During the earliest construction the pipeline is
    // not loaded yet; onPipelineChange()+start() populate tags then.
    if (world->renderPipeline != nullptr) {
        for (const auto& pipelineEntry : world->renderPipeline->getCameraTagToRenderTagSetMap()) {
            if (camera->hasTag(HashUtil::hashString(pipelineEntry.first))) {
                for (const std::set<std::string>& tagSet : pipelineEntry.second) {
                    std::vector<uint64_t> hashList;
                    for (const std::string& tag : tagSet) {
                        hashList.emplace_back(HashUtil::hashString(tag));
                    }
                    tagMap->insert(std::make_pair(hashList, RenderList()));
                }
            }
        }
    }

    // If threads are already running (world loaded and playing), also create a thread entry so
    // fillVisibleObjectsUsingTags processes this camera. During initial load the pool is empty and start()
    // creates the threads.
    if (!visibilityThreadPool.empty()) {
        VisibilityRequest* request = new VisibilityRequest(camera, &world->cullingEntries, world->getStartingPlayer(), tagMap,
                                                           world->currentPlayer->getPosition(),
                                                           world->options, &cullingBarrier, camera->getName());
        if (multiThreadedCulling) {
            SDL2MultiThreading::InternalThread* thread = new SDL2MultiThreading::InternalThread(
                camera->getName(),
                [request]() { VisibilityManager::staticOcclusionThread(request); }
            );
            thread->run();
            visibilityThreadPool[request] = thread;
        } else {
            visibilityThreadPool[request] = nullptr;
        }
    }
}

void VisibilityManager::removeCamera(Camera* camera) {
    for (auto it = visibilityThreadPool.begin(); it != visibilityThreadPool.end(); ++it) {
        if (it->first->camera == camera) {
            it->first->running = false;
            it->first->visibilityLatch.signal();
            delete it->second;
            VisibilityRequest* request = it->first;
            visibilityThreadPool.erase(it);
            delete request;
            break;
        }
    }
    auto cullingIt = cullingResults.find(camera);
    if (cullingIt != cullingResults.end()) {
        delete cullingIt->second;
        cullingResults.erase(cullingIt);
    }
}

const RenderList* VisibilityManager::getDebugOverlayRenderList() const {
    static const uint64_t playerCameraTag = HashUtil::hashString(HardCodedTags::CAMERA_PLAYER);
    for (const auto &item: visibilityThreadPool) {
        if (item.first->camera->hasTag(playerCameraTag)) {
            return &item.first->debugOverlayRenderList;
        }
    }
    return nullptr;
}

void VisibilityManager::clearAllRenderLists() {
    //main thread only, the culling threads are parked between ticks
    for (const auto &item: visibilityThreadPool) {
        VisibilityRequest* request = item.first;
        for (auto& renderEntries: *request->visibility) {
            renderEntries.second.clear();
        }
        request->debugOverlayRenderList.clear();
    }
}

void VisibilityManager::removeCameras(const std::vector<Camera*>& cameras) {
    for (Camera* camera : cameras) {
        removeCamera(camera);
    }
}

void VisibilityManager::fillVisibleObjectsUsingTags() {
    //every camera rebuilds its lists each tick, a list is filled and finalized, never patched
    for (const auto &item: visibilityThreadPool) {
        VisibilityRequest* request = item.first;
        for(auto& renderEntries:*request->visibility) {
            renderEntries.second.clear();
        }
        request->debugOverlayRenderList.clear();
    }
    if(multiThreadedCulling) {
        // The start() method now handles the initial thread creation.
        // We only need to signal and wait for processing here.
        //std::cout << "          new frame, trigger occlusion threads" << std::endl;
        // Main thread is checking dirty state, because python player/camera access from other threads
        // require GIL
        size_t wokenThreadCount = 0;
        for (const auto &item: visibilityThreadPool) {
            item.first->cameraIsDirty = item.first->camera->isDirty();
            item.first->playerDead = world->currentPlayer->isDead();
            item.first->visibilityLatch.signal();
            wokenThreadCount++;
        }
        // Wait for all the culling threads to free. If the number is not equal, logs error meaning there are
        // some rouge threads
        cullingBarrier.waitForAll(wokenThreadCount);

        for (const auto &item: visibilityThreadPool) {
            item.first->playerPosition = world->currentPlayer->getPosition();
            for (auto& changedRigs:item.first->changedBoneTransforms) {
                world->changedBoneTransforms.emplace(changedRigs.first, changedRigs.second);
            }
            item.first->changedBoneTransforms.clear();
        }
    } else {
        // The start() method now handles the initial request creation.
        for (const auto &item: visibilityThreadPool) {
            item.first->playerPosition = world->currentPlayer->getPosition();
            item.first->playerDead = world->currentPlayer->isDead();
            fillVisibleObjectPerCamera(item.first);
            item.first->playerPosition = world->currentPlayer->getPosition();
            for (auto& changedRigs:item.first->changedBoneTransforms) {
                world->changedBoneTransforms.emplace(changedRigs.first, changedRigs.second);
            }
            item.first->changedBoneTransforms.clear();
        }
    }

    //all cameras calculated, clear dirty for object
    for (CullingEntry& cullingEntry : world->cullingEntries) {
        cullingEntry.dirtyForFrustum = false;
    }
}

std::map<VisibilityRequest*, SDL2MultiThreading::InternalThread*> VisibilityManager::occlusionThreadManager() {
    std::map<VisibilityRequest*, SDL2MultiThreading::InternalThread*> visibilityProcessing;
    for (auto &cameraVisibility: cullingResults) {
        VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->cullingEntries, world->getStartingPlayer(), cameraVisibility.second, world->currentPlayer->getPosition(), world->options, &cullingBarrier, cameraVisibility.first->getName());
        SDL2MultiThreading::InternalThread* thread = new SDL2MultiThreading::InternalThread(
            request->camera->getName(),
            [request]() { VisibilityManager::staticOcclusionThread(request); }
        );
        thread->run();
        visibilityProcessing[request] = thread;
    }
    return visibilityProcessing;
}

void VisibilityManager::resetVisibilityBufferForRenderPipelineChange() {
    for (auto &item: cullingResults) {
        item.second->clear();
    }
}

void VisibilityManager::resetCameraTagsFromPipeline(const std::map<std::string, std::vector<std::set<std::string>>> &cameraRenderTagListMap) {
    for (auto& cameraEntryForCulling:cullingResults) { //key is the camera
        for (const auto& renderTagListMapFromPipelineForCamera : cameraRenderTagListMap) {
            if(cameraEntryForCulling.first->hasTag(HashUtil::hashString(renderTagListMapFromPipelineForCamera.first))) {
                //we have a camera and a renderStage match, update the tag information.
                // in renderTagListMapFromPipelineForCamera we have a list, in the list each element is a set of tags. we want to convert them and create new entries based on that
                cameraEntryForCulling.second->clear();
                for(std::set<std::string> tagSet:renderTagListMapFromPipelineForCamera.second) {
                    std::vector<uint64_t> tempHashList;
                    for(std::string tagString: tagSet) {
                        uint64_t tempHash = HashUtil::hashString(tagString);
                        tempHashList.emplace_back(tempHash);
                    }
                    //One set is done, put it in the culling data structure
                    cameraEntryForCulling.second->insert(std::make_pair(tempHashList, RenderList()));
                }
            }
        }
    }
}

void VisibilityManager::resetTagsAndRefillCulling() {
    resetVisibilityBufferForRenderPipelineChange();
    resetCameraTagsFromPipeline(world->renderPipeline->getCameraTagToRenderTagSetMap());
    fillVisibleObjectsUsingTags();
}

void VisibilityManager::fillVisibleObjectPerCamera(const VisibilityRequest* visibilityRequest) {
    PROFILE_VISIBILITY("fillVisibleObjectPerCamera");
    ZoneNameV(___tracy_scoped_zone, visibilityRequest->camera->getName().c_str(), visibilityRequest->camera->getName().size());
    //-1 picks each occluder's bake by the size of an occlusion buffer pixel, 0 and up forces that level. Meshes the
    //baker refused fall back to the same level raw
    const int32_t forcedOccluderLodLevel = static_cast<int32_t>(visibilityRequest->occlusionBakeLodLevelOption.getOrDefault(-1L));
    float skipRenderDistance = 0, skipRenderSize = 0, maxSkipRenderSize = 0;
    float objectAverageDepth;
    float objectScreenSize;
    float objectDistance = 1.0f;
    //a point light sees an object from its own position, so a model next to the light gets its fine level there
    //even when the player is far. A directional light has no position, so its cascades follow the player
    const bool lodFromLightToo = visibilityRequest->camera->getType() == Camera::CameraTypes::CUBE;
    LodSelectionContext lodContext;
    lodContext.hysteresis = static_cast<float>(visibilityRequest->lodSwitchHysteresisOption.getOrDefault(0.15));
    lodContext.forceLevel = visibilityRequest->lodForceLevelOption.getOrDefault(-1L);
    //a depth only pass is the one thing that may use welded levels, and that is the camera role, not its
    //projection. An orthographic player camera still samples textures
    static const uint64_t lodPlayerCameraTag = HashUtil::hashString(HardCodedTags::CAMERA_PLAYER);
    lodContext.isShadowCamera = !visibilityRequest->camera->hasTag(lodPlayerCameraTag);
    long splitModelToMeshCount;
    bool softwareOcclusionRenderDump = false;
    long softwareOcclusionRenderDumpFrequency = 500;
    float softwareOcclusionOccluderSize = 0.25f;
    glm::mat4 cameraProjectionMatrix;
    glm::vec3 viewDirection;
    glm::vec3 cameraPos;
    //world size of one occlusion buffer pixel along its coarser axis, per unit of distance for a perspective camera
    float occluderPixelSize = 0.0f;
    bool occluderPixelGrowsWithDistance = false;
    splitModelToMeshCount = visibilityRequest->splitModelToMeshCountOption.get();
    if(visibilityRequest->camera->getType() == Camera::CameraTypes::PERSPECTIVE ||
       visibilityRequest->camera->getType() == Camera::CameraTypes::ORTHOGRAPHIC) {
        skipRenderDistance = visibilityRequest->skipRenderDistanceOption.get();
        skipRenderSize = visibilityRequest->skipRenderSizeOption.get();
        maxSkipRenderSize = visibilityRequest->maxSkipRenderSizeOption.get();
        const glm::mat4 projectionMatrix = visibilityRequest->camera->getProjectionMatrix();
        cameraProjectionMatrix = projectionMatrix * visibilityRequest->camera->getCameraMatrixConst();
        occluderPixelSize = std::max(2.0f / (projectionMatrix[0][0] * static_cast<float>(visibilityRequest->occlusionCuller.getBufferWidth())),
                                     2.0f / (projectionMatrix[1][1] * static_cast<float>(visibilityRequest->occlusionCuller.getBufferHeight())));
        occluderPixelGrowsWithDistance = visibilityRequest->camera->getType() == Camera::CameraTypes::PERSPECTIVE;
    }
    int& frameCount = visibilityRequest->occlusionCuller.dumpFrameCount;
    // Read live (realtime toggle, editor exposed). When disabled, the player camera still rebuilds its render
    // lists every frame (skipOcclusionCulling stays false for it), but the software occluder is bypassed and
    // every frustum/LOD-passing object is added directly. runOcclusion gates only the occluder work.
    bool occlusionCullingEnabled = visibilityRequest->occlusionEnabledOption.getOrDefault(true);

    // Software occlusion culling runs for the player camera and directional light cameras.
    // SDOC handles orthographic via auto-detection in the submodule (see snapdragon-oc
    // commit 42a9b69c): it detects ortho from the VP bottom row and carries clip-Z in the invW slot.
    static const uint64_t playerCameraTag      = HashUtil::hashString(HardCodedTags::CAMERA_PLAYER);
    static const uint64_t directionalLightTag  = HashUtil::hashString(HardCodedTags::CAMERA_LIGHT_DIRECTIONAL);
    static const uint64_t nonOccluderTag       = HashUtil::hashString(HardCodedTags::OBJECT_MODEL_NON_OCCLUDER);
    bool skipOcclusionCulling = false;
    const bool isDirectionalLightCamera = visibilityRequest->camera->hasTag(directionalLightTag);
    if (!visibilityRequest->camera->hasTag(playerCameraTag) &&
        !isDirectionalLightCamera) {
        skipOcclusionCulling = true;
    } else {
        glm::mat4 invertedView = glm::inverse(visibilityRequest->camera->getCameraMatrixConst());
        viewDirection = -glm::vec3(invertedView[2]);
        viewDirection = glm::normalize(viewDirection);
        cameraPos = glm::vec3(invertedView[3]); // 4th column
        softwareOcclusionRenderDump = visibilityRequest->occlusionRenderDumpOption.getOrDefault(false);
        softwareOcclusionRenderDumpFrequency = visibilityRequest->occlusionRenderDumpFrequencyOption.getOrDefault(500);
        if (isDirectionalLightCamera) {
            softwareOcclusionOccluderSize = visibilityRequest->occlusionOccluderSizeOrthographicOption.getOrDefault(0.01f);
        } else {
            softwareOcclusionOccluderSize = visibilityRequest->occlusionOccluderSizePerspectiveOption.getOrDefault(0.1f);
        }

        if (occlusionCullingEnabled) {
            visibilityRequest->occlusionCuller.newFrame(cameraPos, viewDirection, visibilityRequest->camera->getCameraMatrixConst(), visibilityRequest->camera->getProjectionMatrix());
        }
    }
    // Occluder rasterization + occludee resolution only runs for the player camera when culling is enabled.
    const bool runOcclusion = !skipOcclusionCulling && occlusionCullingEnabled;
    // For orthographic (directional light) cameras the area metric objectScreenSize produces
    // values much lower than for a perspective camera at the same relative object size, because
    // perspective exaggerates near objects while orthographic does not. Use the max linear
    // screen dimension (max(screenSizeX, screenSizeY)) for the occluder split instead,
    // so the same threshold "25% of frustum width or height" works across all cascade sizes.

    uint32_t frustumCulledCount = 0;
    uint32_t totalCounter = 0;
    uint32_t lodSkipCounter = 0;
    uint32_t occluderCounter = 0;
    uint32_t occludedCounter = 0;
    uint32_t nonOccludedCount = 0; // occludees and occluders that survived the depth test
    float maxScreenSize = 0.0;
    {
    PROFILE_VISIBILITY("Culling::ObjectLoop");
    const std::vector<CullingEntry>& cullingEntries = *visibilityRequest->cullingEntries;
    const bool occluderSizeByGroup = runOcclusion && visibilityRequest->occlusionOccluderSizeByGroupOption.getOrDefault(true);
    const float groupMemberMinSize = softwareOcclusionOccluderSize * static_cast<float>(visibilityRequest->occlusionOccluderGroupMemberMinFractionOption.getOrDefault(0.1));
    for (size_t entryIndex = 0; entryIndex < cullingEntries.size(); ++entryIndex) {
        const CullingEntry& cullingEntry = cullingEntries[entryIndex];
#ifndef NDEBUG
        if (cullingEntry.nearestModelGroup != ModelGroup::findNearestFor(cullingEntry.model)) {
            std::cerr << "Model " << cullingEntry.model->getWorldObjectID() << " has a stale nearest model group, a parent change skipped WorldAPIAccessor::refreshNearestModelGroup" << std::endl;
            std::exit(-1);
        }
#endif
        const ModelGroup* occluderGroup = occluderSizeByGroup ? cullingEntry.nearestModelGroup : nullptr;
        bool isHiddenPlayerAttachment = false;
        if (visibilityRequest->playerDead) {
            const Attachable* hierarchyRoot = cullingEntry.model;
            while (hierarchyRoot->getParentObject() != nullptr) {
                hierarchyRoot = hierarchyRoot->getParentObject();
            }
            isHiddenPlayerAttachment = hierarchyRoot == visibilityRequest->playerObject;
        }
        bool isVisible = !isHiddenPlayerAttachment && visibilityRequest->camera->isVisible(cullingEntry.aabbMin, cullingEntry.aabbMax);
        if (!isVisible) {
            frustumCulledCount++;//counts models, not meshes. The split mesh path is going away, it doesn't need its own count
            continue;
        }
        Model *currentModel = cullingEntry.model;
        //the whole model path below depends on the object and the camera, never on the tag set, so it runs once
        //here instead of once per matching tag set
        const std::vector<Model::MeshMeta *> &meshMetas = currentModel->getMeshMetaData();
        const bool wholeModelPath = meshMetas.size() < static_cast<size_t>(splitModelToMeshCount);
        bool modelLodSkipped = false;
        uint32_t modelLod = 0;
        if (isVisible && wholeModelPath) {
            modelLodSkipped = isSkippedByLodDistance(skipRenderDistance, skipRenderSize, maxSkipRenderSize, cameraProjectionMatrix, visibilityRequest->playerPosition, currentModel->getAabbMin(), currentModel->getAabbMax(), objectAverageDepth, objectScreenSize, objectDistance);
            if (!modelLodSkipped) {
                lodContext.objectDistance = lodFromLightToo ? std::min(objectDistance, distanceToBox(visibilityRequest->camera->getPosition(), currentModel->getAabbMin(), currentModel->getAabbMax())) : objectDistance;
                lodContext.objectScale = getLodObjectScale(currentModel);
                modelLod = currentModel->getModelAsset()->getLodLadder().selectLevel(lodContext,
                                   visibilityRequest->getPreviousLodLevel(currentModel->getWorldObjectID()));
                visibilityRequest->setPreviousLodLevel(currentModel->getWorldObjectID(), modelLod);
            }
        }
        //which meshes occlude doesn't depend on the tag set, so the first matching set submits them. Every other set would rasterize them again
        bool occludersSubmitted = false;
        for (auto& visibilityEntry: *visibilityRequest->visibility) {
            if (VisibilityRequest::isAnyTagMatch(visibilityEntry.first, currentModel->getTags())) {
                if(isVisible) {
                    if (wholeModelPath) {
                        totalCounter += meshMetas.size();
                        if (!modelLodSkipped) {
                            if (!runOcclusion || passesOccluderSize(occluderGroup, objectScreenSize, cameraProjectionMatrix, softwareOcclusionOccluderSize, groupMemberMinSize)) {
                                if (objectScreenSize > maxScreenSize) {
                                    maxScreenSize = objectScreenSize;
                                }
                                occluderCounter += meshMetas.size();
                                //an animated occluder would need its node transform and pose, which we don't have here, so it only ever occludes itself
                                if (runOcclusion && !currentModel->isAnimated() && !occludersSubmitted && !currentModel->hasTag(nonOccluderTag)) {
                                    visibilityRequest->occlusionCuller.renderOccluder(currentModel, selectOccluderLod(currentModel->getModelAsset()->getLodLadder(), forcedOccluderLodLevel,
                                                                                                         occluderPixelSize * (occluderPixelGrowsWithDistance ? objectDistance : 1.0f), getLodObjectScale(currentModel)));
                                    //std::cout << currentModel->getName() << ":" << " is occluder " << std::endl;
                                }
                                for (auto& meshMeta:meshMetas) {
                                    //an occluder can be hidden by another one too. Its own aabb is in front of its surface, so it can't hide itself
                                    if (runOcclusion) {
                                        visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, modelLod, objectAverageDepth, &visibilityEntry.second);
                                    } else {
                                        visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel, modelLod, objectAverageDepth);
                                    }
                                }
                            } else {
                                for (auto& meshMeta:meshMetas) {
                                    visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, modelLod, objectAverageDepth, &visibilityEntry.second);
                                }
                            }
                        } else {
                            lodSkipCounter++;
                        }
                    } else {
                        //for models with more than 10 meshes, we don't wanna add all of them to renderlist, need to re check visibility
                        for (auto& meshMeta:meshMetas) {
                            totalCounter++;
                            glm::vec3 meshWorldMin, meshWorldMax;
                            //transforming only min and max corners shrinks the box of a rotated mesh, wrongly culling it
                            AABBConverter::getWorldSpaceAABB(currentModel->getTransformation()->getWorldTransform(), meshMeta->mesh->getAabbMin(), meshMeta->mesh->getAabbMax(), meshWorldMin, meshWorldMax);
                            if (visibilityRequest->camera->isVisible(meshWorldMin, meshWorldMax)) {
                                bool lodSkipped = isSkippedByLodDistance(skipRenderDistance, skipRenderSize, maxSkipRenderSize, cameraProjectionMatrix, visibilityRequest->playerPosition, meshWorldMin, meshWorldMax, objectAverageDepth, objectScreenSize, objectDistance);
                                lodContext.objectDistance = lodFromLightToo ? std::min(objectDistance, distanceToBox(visibilityRequest->camera->getPosition(), meshWorldMin, meshWorldMax)) : objectDistance;
                                lodContext.objectScale = getLodObjectScale(currentModel);
                                uint32_t lod = currentModel->getModelAsset()->getLodLadder().selectLevel(lodContext,
                                                              visibilityRequest->getPreviousLodLevel(currentModel->getWorldObjectID()));
                                visibilityRequest->setPreviousLodLevel(currentModel->getWorldObjectID(), lod);
                                if (!lodSkipped) {
                                    if (!runOcclusion || passesOccluderSize(occluderGroup, objectScreenSize, cameraProjectionMatrix, softwareOcclusionOccluderSize, groupMemberMinSize)) {
                                        if (objectScreenSize > maxScreenSize) {
                                            maxScreenSize = objectScreenSize;
                                        }
                                        occluderCounter++;
                                        if (runOcclusion && !currentModel->isAnimated() && !occludersSubmitted && !currentModel->hasTag(nonOccluderTag)) {
                                            visibilityRequest->occlusionCuller.renderOccluder(meshMeta, currentModel->getTransformation()->getWorldTransform(),
                                                                                              selectOccluderLod(currentModel->getModelAsset()->getLodLadder(), forcedOccluderLodLevel,
                                                                                                                occluderPixelSize * (occluderPixelGrowsWithDistance ? objectDistance : 1.0f), getLodObjectScale(currentModel)));
                                        }
                                        if (runOcclusion) {
                                            visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, lod, objectAverageDepth, &visibilityEntry.second);
                                        } else {
                                            visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel, lod, objectAverageDepth);
                                        }
                                    } else {
                                        visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, lod, objectAverageDepth, &visibilityEntry.second);
                                    }
                                } else {
                                    lodSkipCounter++;
                                }
                            }
                        }
                    }
                    occludersSubmitted = true;
                    if (currentModel->isAnimated()) {
                        visibilityRequest->changedBoneTransforms[currentModel->getRigId()] = currentModel->getBoneTransforms();
                    }
                }
            }
        }
    }
    }
    //now we can actually check the occlusion:
    if (runOcclusion) {
        //shadow cascades keep their normal lists, the debug view is about what the player sees
        const bool debugReverse = visibilityRequest->occlusionDebugReverseOption.getOrDefault(false) && visibilityRequest->camera->hasTag(playerCameraTag);
        std::vector<OcculudeeMetaData*> nonOccludedMeshes;
        std::vector<OcculudeeMetaData*> occludedMeshes;
        if (debugReverse) {
            visibilityRequest->occlusionCuller.splitOccludees(nonOccludedMeshes, occludedMeshes);
        } else {
            nonOccludedMeshes = visibilityRequest->occlusionCuller.getNonOccludedMeshMeta();
        }
        if (debugReverse) {
            PROFILE_VISIBILITY("Culling::AddVisibleDebugReverse");
            for (auto metaData:occludedMeshes) {
                metaData->renderList->addMeshMaterial(metaData->meshMeta->material, metaData->meshMeta->mesh, metaData->model, metaData->lod, metaData->averageDepth);
            }
            //every tag list of the player camera holds the same mesh, the overlay list keeps it once
            for (auto metaData:nonOccludedMeshes) {
                visibilityRequest->debugOverlayRenderList.addMeshMaterial(metaData->meshMeta->material, metaData->meshMeta->mesh, metaData->model, metaData->lod, metaData->averageDepth);
            }
        } else {
            PROFILE_VISIBILITY("Culling::AddVisible");
            for (auto metaData:nonOccludedMeshes) {
                metaData->renderList->addMeshMaterial(metaData->meshMeta->material, metaData->meshMeta->mesh, metaData->model, metaData->lod, metaData->averageDepth);
            }
        }
        nonOccludedCount = static_cast<uint32_t>(nonOccludedMeshes.size());
        occludedCounter = totalCounter - nonOccludedCount;//occluders are queried too, so they are part of nonOccludedCount
        if (occluderCounter != 0 && occludedCounter != 0) {
            //std::cout << "Total occluder count is " << occluderCounter << " and it occluded " << occludedCounter << std::endl;
        }
        frameCount++;
        if (frameCount == softwareOcclusionRenderDumpFrequency) {
            if (softwareOcclusionRenderDump) {
                std::string dumpFileName;
                if (isDirectionalLightCamera) {
                    const float frustumHalfWidth = 1.0f / visibilityRequest->camera->getProjectionMatrix()[0][0];
                    char buf[256];
                    snprintf(buf, sizeof(buf), "SDOCdepthMap_%s_%.1f",
                             visibilityRequest->camera->getName().c_str(), frustumHalfWidth);
                    dumpFileName = buf;
                } else {
                    dumpFileName = "SDOCdepthMap_player";
                }
                visibilityRequest->occlusionCuller.dumpDepth(dumpFileName);
            }
            frameCount = 0;
        }
    }
    {
        PROFILE_VISIBILITY("Culling::Finalize");
        for (auto& visibilityEntry: *visibilityRequest->visibility) {
            visibilityEntry.second.finalize();
        }
        visibilityRequest->debugOverlayRenderList.finalize();
    }
#ifdef TRACY_ENABLE
    if (ProfilerState::isTracingVisibility()) {
        // Tracy requires static names for plot. Multithreading requires thread_local
        thread_local static std::unordered_map<std::string, std::string> plotNameCache;
        // Directional light has single name, multiple cameras, because of CSM
        // We will name it with cascade size
        char cameraSuffixBuf[256];
        if (isDirectionalLightCamera) {
            const float frustumHalfWidth = 1.0f / visibilityRequest->camera->getProjectionMatrix()[0][0];
            snprintf(cameraSuffixBuf, sizeof(cameraSuffixBuf), "::%s[%.1f]",
                     visibilityRequest->camera->getName().c_str(),
                     frustumHalfWidth);
        } else {
            snprintf(cameraSuffixBuf, sizeof(cameraSuffixBuf), "::%s",
                     visibilityRequest->camera->getName().c_str());
        }
        const std::string cameraSuffix(cameraSuffixBuf);

        auto stableName = [&](const char* stat) -> const char* {
            std::string key = std::string(stat) + cameraSuffix;
            return plotNameCache.emplace(key, key).first->second.c_str();
        };

        tracy::Profiler::PlotData(stableName("Frustum Culled"), (int64_t)frustumCulledCount);
        tracy::Profiler::PlotData(stableName("LOD Skipped"),    (int64_t)lodSkipCounter);
        tracy::Profiler::PlotData(stableName("Total Visible"),  (int64_t)(runOcclusion ? nonOccludedCount : occluderCounter));
        if (runOcclusion) {
            tracy::Profiler::PlotData(stableName("Occluders"), (int64_t)occluderCounter);
            tracy::Profiler::PlotData(stableName("Occluded"),  (int64_t)occludedCounter);
        }
    }
#endif
}

void VisibilityManager::staticOcclusionThread(VisibilityRequest* visibilityRequest) {
    // Code that runs on the background thread. It always starts with wait latch, so we know it is starting
    // When it should
    while (true) {
        visibilityRequest->visibilityLatch.wait();
        if (!visibilityRequest->running) {
            return;
        }
        fillVisibleObjectPerCamera(visibilityRequest);
        visibilityRequest->frameBarrier->arrive();
    }
}

//a grouped model is judged by its group's box, so a building's floors don't drop out of the occluders one by one.
//It still needs groupMemberMinSize itself, otherwise every small prop of a big group would be rasterized
bool VisibilityManager::passesOccluderSize(const ModelGroup* group, float ownScreenSize, const glm::mat4& cameraProjectionMatrix, float occluderSize, float groupMemberMinSize) {
    if (group == nullptr) {
        return ownScreenSize > occluderSize;
    }
    if (ownScreenSize < groupMemberMinSize) {
        return false;
    }
    glm::vec3 ndcMin, ndcMax;
    AABBConverter::getNCDAABB(group->getAabbMin(), group->getAabbMax(), cameraProjectionMatrix, ndcMin, ndcMax);
    return ((ndcMax.x - ndcMin.x) / 2.0f) * ((ndcMax.y - ndcMin.y) / 2.0f) > occluderSize;//same measure isSkippedByLodDistance gives a model
}

//level distances are set for the model as authored, so a model scaled up is used at proportionally further distances
float VisibilityManager::getLodObjectScale(const Model* model) {
    glm::vec3 scale = model->getTransformation()->getScale();
    return std::max(std::abs(scale.x), std::max(std::abs(scale.y), std::abs(scale.z)));
}

//levels are scored by what they show on the reference screen at a distance, so the occluder picks the level whose
//reference pixel is as big as an occlusion buffer pixel at the object. The occluder is depth only, so welded levels qualify
uint32_t VisibilityManager::selectOccluderLod(const LodLadder& ladder, int32_t forcedFullLadderLevel, float occluderPixelSizeAtObject, float objectScale) {
    if (forcedFullLadderLevel >= 0) {
        return ladder.getMeshLodIndexForFullLadderLevel(static_cast<uint32_t>(forcedFullLadderLevel));
    }
    LodSelectionContext occluderContext;
    occluderContext.isShadowCamera = true;
    occluderContext.objectDistance = occluderPixelSizeAtObject * ladder.getReferencePixelsPerMeter();
    occluderContext.objectScale = objectScale;
    return ladder.selectLevel(occluderContext, 0);//no hysteresis, a switching occluder isn't seen
}

float VisibilityManager::distanceToBox(const glm::vec3 &point, const glm::vec3 &minAABB, const glm::vec3 &maxAABB) {
    const float dx = std::max(minAABB.x - point.x, std::max(0.0f, point.x - maxAABB.x));
    const float dy = std::max(minAABB.y - point.y, std::max(0.0f, point.y - maxAABB.y));
    const float dz = std::max(minAABB.z - point.z, std::max(0.0f, point.z - maxAABB.z));
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

bool VisibilityManager::isSkippedByLodDistance(float skipRenderDistance, float skipRenderSize, float maxSkipRenderSize, const glm::mat4 &cameraProjectionMatrix, const glm::vec3& playerPosition, glm::vec3 minAABB, glm::vec3 maxAABB, float &objectAverageDepth, float &objectScreenSize, float &objectDistance) {
    //now we get to calculate the size in screen
    glm::vec3 ndcMin, ndcMax;
    AABBConverter::getNCDAABB(minAABB, maxAABB, cameraProjectionMatrix, ndcMin, ndcMax);
    const float screenSizeX = (ndcMax.x - ndcMin.x) / 2.0f; // since OpenGL NDC is -1, 1 each side can be max 2, but we want 0,1
    const float screenSizeY = (ndcMax.y - ndcMin.y) / 2.0f;//area below is what the occluder split uses
    objectScreenSize = (screenSizeX * screenSizeY);

    objectAverageDepth = (ndcMax.z + ndcMin.z) / -2.0f;//larger is nearer

    objectDistance = distanceToBox(playerPosition, minAABB, maxAABB);
    if(skipRenderDistance !=0 && objectDistance > skipRenderDistance) {           //Is it distant enough to skip?
        if ((maxAABB.x - minAABB.x) < maxSkipRenderSize &&                    //Is it actually small enough to skip? We don't wanna skip mountains becuse they are far away.
            (maxAABB.y - minAABB.y) < maxSkipRenderSize )
            if(screenSizeX < skipRenderSize && screenSizeY < skipRenderSize) {  //Is it small enough in the screen to skip?
                return true;
            }
    }
    return false;
}
