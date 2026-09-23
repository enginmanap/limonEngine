#include "VisibilityManager.h"
#include "World.h"
#include "Camera/Camera.h"
#include "Graphics/GraphicsPipeline.h"
#include "GameObjects/Model.h"
#include "GameObjects/Players/Player.h"
#include "Utils/HardCodedTags.h"
#include "../Profiler/ProfilerMacros.h"

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
                VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->objects, world->getStartingPlayer(), cameraVisibility.second, world->currentPlayer->getPosition(), world->options, &cullingBarrier, cameraVisibility.first->getName());
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
        VisibilityRequest* request = new VisibilityRequest(camera, &world->objects, world->getStartingPlayer(), tagMap,
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

void VisibilityManager::removeCameras(const std::vector<Camera*>& cameras) {
    for (Camera* camera : cameras) {
        removeCamera(camera);
    }
}

void VisibilityManager::fillVisibleObjectsUsingTags() {
    //first clear up dirty cameras, and the player camera, because the player camera writes the occlusion
    //culling depth map and re-evaluates all objects every frame (keyed on the player role, not projection
    //type, so an orthographic player camera is handled the same as a perspective one).
    static const uint64_t playerCameraTag = HashUtil::hashString(HardCodedTags::CAMERA_PLAYER);
    for (auto &it: cullingResults) {
        if (it.first->isDirty() || it.first->hasTag(playerCameraTag)) {
            for(auto& renderEntries:*it.second) {
                renderEntries.second.clear();
            }
        }
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

    for (auto objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt) {
        //all cameras calculated, clear dirty for object
        objectIt->second->setCleanForFrustum();
    }
    for (auto &it: cullingResults) {
        for (auto& it2:*it.second) {
            it2.second.cleanUpEmptyRenderLists();
        }
    }
}

std::map<VisibilityRequest*, SDL2MultiThreading::InternalThread*> VisibilityManager::occlusionThreadManager() {
    std::map<VisibilityRequest*, SDL2MultiThreading::InternalThread*> visibilityProcessing;
    for (auto &cameraVisibility: cullingResults) {
        VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->objects, world->getStartingPlayer(), cameraVisibility.second, world->currentPlayer->getPosition(), world->options, &cullingBarrier, cameraVisibility.first->getName());
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
    //every LOD is baked, so this picks which bake occludes. Meshes the baker refused fall back to the same level raw
    const uint32_t occluderLodLevel = static_cast<uint32_t>(visibilityRequest->occlusionBakeLodLevelOption.getOrDefault(0L));
    float skipRenderDistance = 0, skipRenderSize = 0, maxSkipRenderSize = 0;
    float objectAverageDepth;
    float objectScreenSize;
    float objectDistance = 1.0f;
    const Camera::CameraTypes lodCameraType = visibilityRequest->camera->getType();
    const bool perspectiveLodProjection = lodCameraType != Camera::CameraTypes::ORTHOGRAPHIC;
    const float lodPixelTolerance = static_cast<float>(visibilityRequest->lodPixelToleranceOption.getOrDefault(1.0));
    //error in model units times this is its size in pixels at distance one, shadow cameras render to their own map size
    float lodPixelScale;
    if (lodCameraType == Camera::CameraTypes::PERSPECTIVE) {
        lodPixelScale = visibilityRequest->camera->getProjectionMatrix()[1][1] * static_cast<float>(visibilityRequest->displayHeightOption.getOrDefault(1080)) * 0.5f;
    } else if (lodCameraType == Camera::CameraTypes::ORTHOGRAPHIC) {
        lodPixelScale = visibilityRequest->camera->getProjectionMatrix()[1][1] * static_cast<float>(visibilityRequest->shadowMapDirectionalSizeOption.getOrDefault(1024)) * 0.5f;
    } else {
        //cube faces are 90 degree perspective, so their projection scale is 1. Never ask a cube camera for a matrix, it exits
        lodPixelScale = static_cast<float>(visibilityRequest->shadowMapPointHeightOption.getOrDefault(512)) * 0.5f;
    }
    long splitModelToMeshCount;
    bool softwareOcclusionRenderDump = false;
    long softwareOcclusionRenderDumpFrequency = 500;
    float softwareOcclusionOccluderSize = 0.25f;
    glm::mat4 cameraProjectionMatrix;
    glm::vec3 viewDirection;
    glm::vec3 cameraPos;
    splitModelToMeshCount = visibilityRequest->splitModelToMeshCountOption.get();
    if(visibilityRequest->camera->getType() == Camera::CameraTypes::PERSPECTIVE ||
       visibilityRequest->camera->getType() == Camera::CameraTypes::ORTHOGRAPHIC) {
        skipRenderDistance = visibilityRequest->skipRenderDistanceOption.get();
        skipRenderSize = visibilityRequest->skipRenderSizeOption.get();
        maxSkipRenderSize = visibilityRequest->maxSkipRenderSizeOption.get();
        cameraProjectionMatrix = visibilityRequest->camera->getProjectionMatrix() * visibilityRequest->camera->getCameraMatrixConst();
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
    uint32_t nonOccludedCount = 0; // occludees that survived the depth test
    float maxScreenSize = 0.0;
    for (auto objectIt = visibilityRequest->objects->begin(); objectIt != visibilityRequest->objects->end(); ++objectIt) {
        bool isHiddenPlayerAttachment = false;
        if (visibilityRequest->playerDead) {
            const Attachable* hierarchyRoot = objectIt->second;
            while (hierarchyRoot->getParentObject() != nullptr) {
                hierarchyRoot = hierarchyRoot->getParentObject();
            }
            isHiddenPlayerAttachment = hierarchyRoot == visibilityRequest->playerObject;
        }
        //a dead player's attachments stop moving, so the dirty skip would leave them in the light cameras
        if(!visibilityRequest->cameraIsDirty && !objectIt->second->isDirtyForFrustum() && skipOcclusionCulling && !isHiddenPlayerAttachment) {
            continue; //if neither object nor camera dirty, no need to recalculate
        }
        Model *currentModel = objectIt->second;
        bool isVisible = !isHiddenPlayerAttachment && visibilityRequest->camera->isVisible(*currentModel);//find if visible
        for (auto& visibilityEntry: *visibilityRequest->visibility) {
            if (VisibilityRequest::isAnyTagMatch(visibilityEntry.first, currentModel->getTags())) {
                if(isVisible) {
                    const std::vector<Model::MeshMeta *> &meshMetas =currentModel->getMeshMetaData();
                    if (meshMetas.size() < static_cast<size_t>(splitModelToMeshCount)) {
                        totalCounter += meshMetas.size();
                        bool lodSkipped = isSkippedByLodDistance(skipRenderDistance, skipRenderSize, maxSkipRenderSize, cameraProjectionMatrix, visibilityRequest->playerPosition, objectIt->second->getAabbMin(), objectIt->second->getAabbMax(), objectAverageDepth, objectScreenSize, objectDistance);
                        if (!lodSkipped) {
                            float objectScale = getLodObjectScale(currentModel);
                            float lodDistance = perspectiveLodProjection ? objectDistance : 1.0f;
                            if (objectScreenSize > softwareOcclusionOccluderSize || !runOcclusion) {
                                if (objectScreenSize > maxScreenSize) {
                                    maxScreenSize = objectScreenSize;
                                }
                                occluderCounter += meshMetas.size();
                                //an animated occluder would need its node transform and pose, which we don't have here, so it only ever occludes itself
                                if (runOcclusion && !currentModel->isAnimated()) {
                                    visibilityRequest->occlusionCuller.renderOccluder(currentModel, occluderLodLevel);
                                    //std::cout << currentModel->getName() << ":" << " is occluder " << std::endl;
                                }
                                for (auto& meshMeta:meshMetas) {
                                    uint32_t lod = selectLodLevel(meshMeta->mesh.get(), lodDistance, objectScale, lodPixelScale, lodPixelTolerance);
                                    visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel, lod, objectAverageDepth);
                                }
                            } else {
                                for (auto& meshMeta:meshMetas) {//per mesh, because each mesh carries its own LOD errors
                                    uint32_t lod = selectLodLevel(meshMeta->mesh.get(), lodDistance, objectScale, lodPixelScale, lodPixelTolerance);
                                    visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, lod, objectAverageDepth, &visibilityEntry.second);
                                }
                            }
                        } else {
                            lodSkipCounter++;
                            for (auto& meshMeta : meshMetas) {
                                visibilityEntry.second.removeMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel->getWorldObjectID());
                            }
                        }
                    } else {
                        //for models with more than 10 meshes, we don't wanna add all of them to renderlist, need to re check visibility
                        for (auto& meshMeta:meshMetas) {
                            totalCounter++;
                            if (visibilityRequest->camera->isVisible(currentModel->getTransformation()->getWorldTransform() * meshMeta->mesh->getAabbMin(),
                                                                     currentModel->getTransformation()->getWorldTransform() * meshMeta->mesh->getAabbMax())) {
                                glm::vec3 meshWorldMin, meshWorldMax;
                                AABBConverter::getWorldSpaceAABB(currentModel->getTransformation()->getWorldTransform(), meshMeta->mesh->getAabbMin(), meshMeta->mesh->getAabbMax(), meshWorldMin, meshWorldMax);
                                bool lodSkipped = isSkippedByLodDistance(skipRenderDistance, skipRenderSize, maxSkipRenderSize, cameraProjectionMatrix, visibilityRequest->playerPosition, meshWorldMin, meshWorldMax, objectAverageDepth, objectScreenSize, objectDistance);
                                uint32_t lod = selectLodLevel(meshMeta->mesh.get(), perspectiveLodProjection ? objectDistance : 1.0f, getLodObjectScale(currentModel), lodPixelScale, lodPixelTolerance);
                                if (!lodSkipped) {
                                    if (objectScreenSize > softwareOcclusionOccluderSize || !runOcclusion) {
                                        if (objectScreenSize > maxScreenSize) {
                                            maxScreenSize = objectScreenSize;
                                        }
                                        occluderCounter++;
                                        if (runOcclusion && !currentModel->isAnimated()) {
                                            visibilityRequest->occlusionCuller.renderOccluder(meshMeta, currentModel->getTransformation()->getWorldTransform(), occluderLodLevel);
                                        }
                                        visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel, lod, objectAverageDepth);
                                    } else {
                                        visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, lod, objectAverageDepth, &visibilityEntry.second);
                                    }
                                } else {
                                    lodSkipCounter++;
                                    visibilityEntry.second.removeMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel->getWorldObjectID());
                                }
                            } else {
                                frustumCulledCount++;
                            }
                        }
                    }
                    if (currentModel->isAnimated()) {
                        visibilityRequest->changedBoneTransforms[currentModel->getRigId()] = currentModel->getBoneTransforms();
                    }
                } else { //if not visible
                    const std::vector<Model::MeshMeta *> &meshMetas =currentModel->getMeshMetaData();
                    frustumCulledCount += static_cast<uint32_t>(meshMetas.size());
                    for (auto& meshMeta:meshMetas) {
                        visibilityEntry.second.removeMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel->getWorldObjectID());
                    }
                }
            } else {
                //what if we are not matching a tag, but we at some point did?
                const std::vector<Model::MeshMeta *> &meshMetas =currentModel->getMeshMetaData();
                for (auto& meshMeta:meshMetas) {
                    visibilityEntry.second.removeMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel->getWorldObjectID());
                }
            }
        }
    }
    //now we can actually check the occlusion:
    if (runOcclusion) {
        std::vector<OcculudeeMetaData*> nonOccludedMeshes = visibilityRequest->occlusionCuller.getNonOccludedMeshMeta();
        for (auto metaData:nonOccludedMeshes) {
            metaData->renderList->addMeshMaterial(metaData->meshMeta->material, metaData->meshMeta->mesh, metaData->model, metaData->lod, metaData->averageDepth);
        }
        nonOccludedCount = static_cast<uint32_t>(nonOccludedMeshes.size());
        occludedCounter = totalCounter - occluderCounter - nonOccludedCount;
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
#ifdef TRACY_ENABLE
    // If we did not process, because camera is not changed, values are partial, don't update.
    const bool fullProcessingPass = !skipOcclusionCulling || visibilityRequest->cameraIsDirty;
    if (ProfilerState::isTracingVisibility() && fullProcessingPass) {
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
        tracy::Profiler::PlotData(stableName("Total Visible"),  (int64_t)(occluderCounter + nonOccludedCount));
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

//the stored errors are in model units, so the biggest scale component is what they turn into in the world
float VisibilityManager::getLodObjectScale(const Model* model) {
    glm::vec3 scale = model->getTransformation()->getScale();
    return std::max(std::abs(scale.x), std::max(std::abs(scale.y), std::abs(scale.z)));
}

// the coarsest LOD whose stored error stays under the tolerance once projected to pixels at this distance
uint32_t VisibilityManager::selectLodLevel(const MeshAsset* mesh, float objectDistance, float objectScale, float pixelScale, float pixelTolerance) {
    const float *lodErrors = mesh->getLodErrors();
    const uint32_t *triangleCounts = mesh->getTriangleCount();
    uint32_t selected = 0;
    for (uint32_t level = 1; level < MeshAsset::LOD_LEVEL_COUNT; ++level) {
        if (triangleCounts[level] == 0) {
            continue;//simplification can bottom out at zero triangles
        }
        //ortho cameras pass distance 1, their projection doesn't shrink with distance
        float projectedPixels = (lodErrors[level] * objectScale * pixelScale) / objectDistance;
        if (projectedPixels <= pixelTolerance) {
            selected = level;
        }
    }
    return selected;
}

bool VisibilityManager::isSkippedByLodDistance(float skipRenderDistance, float skipRenderSize, float maxSkipRenderSize, const glm::mat4 &cameraProjectionMatrix, const glm::vec3& playerPosition, glm::vec3 minAABB, glm::vec3 maxAABB, float &objectAverageDepth, float &objectScreenSize, float &objectDistance) {
    //now we get to calculate the size in screen
    glm::vec3 ndcMin, ndcMax;
    AABBConverter::getNCDAABB(minAABB, maxAABB, cameraProjectionMatrix, ndcMin, ndcMax);
    const float screenSizeX = (ndcMax.x - ndcMin.x) / 2.0f; // since OpenGL NDC is -1, 1 each side can be max 2, but we want 0,1
    const float screenSizeY = (ndcMax.y - ndcMin.y) / 2.0f;
    objectScreenSize = (screenSizeX * screenSizeY);

    objectAverageDepth = (ndcMax.z + ndcMin.z) / -2.0f;

    const float dx = std::max(minAABB.x - playerPosition.x, std::max(0.0f, playerPosition.x - maxAABB.x));
    const float dy = std::max(minAABB.y - playerPosition.y, std::max(0.0f, playerPosition.y - maxAABB.y));
    const float dz = std::max(minAABB.z - playerPosition.z, std::max(0.0f, playerPosition.z - maxAABB.z));
    objectDistance = std::max(std::sqrt(dx*dx + dy*dy + dz*dz), 0.001f);//zero when the camera is inside the AABB, and we divide by it
    if(skipRenderDistance !=0 && objectDistance > skipRenderDistance) {           //Is it distant enough to skip?
        if ((maxAABB.x - minAABB.x) < maxSkipRenderSize &&                    //Is it actually small enough to skip? We don't wanna skip mountains becuse they are far away.
            (maxAABB.y - minAABB.y) < maxSkipRenderSize )
            if(screenSizeX < skipRenderSize && screenSizeY < skipRenderSize) {  //Is it small enough in the screen to skip?
                return true;
            }
    }
    return false;
}
