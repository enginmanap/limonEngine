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

        wakeThreadsCondition.signalWaiting();
        if (item.second) {
            SDL_WaitThread(item.second, NULL);
        }
        delete item.first;
    }
    visibilityThreadPool.clear();
}

void VisibilityManager::start() {
    if(multiThreadedCulling) {
        if (visibilityThreadPool.empty()) {
            visibilityThreadPool = occlusionThreadManager();
            bool isAllThreadsStarted = false;
            while (!isAllThreadsStarted) {
                bool allThreadStarted = true;
                for (const auto &item: visibilityThreadPool) {
                    if (!item.first->started) {
                        allThreadStarted = false;
                    }
                }
                isAllThreadsStarted = allThreadStarted;
            }
        }
    } else {
        if(visibilityThreadPool.empty()) {
            for (auto &cameraVisibility: cullingResults) {
                VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->objects, cameraVisibility.second, world->currentPlayer->getPosition(), world->options, &wakeThreadsCondition);
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
        VisibilityRequest* request = new VisibilityRequest(camera, &world->objects, tagMap,
                                                           world->currentPlayer->getPosition(),
                                                           world->options, &wakeThreadsCondition);
        if (multiThreadedCulling) {
            SDL_Thread* thread = SDL_CreateThread(staticOcclusionThread, camera->getName().c_str(), request);
            while (!request->started) {} // wait for thread to signal ready
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
            wakeThreadsCondition.signalWaiting();
            SDL_WaitThread(it->second, nullptr);
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
        for (const auto &item: visibilityThreadPool) {
            item.first->cameraIsDirty = item.first->camera->isDirty();
        }
        wakeThreadsCondition.signalWaiting();
        while (true) {
            bool allDone = true;
            for (const auto &item: visibilityThreadPool) {
                item.first->inProgressLock.lock();
                if (!item.first->processingDone) {
                    allDone = false;
                }
                item.first->inProgressLock.unlock();
            }
            if (allDone) {
                break;
            }
        }

        for (const auto &item: visibilityThreadPool) {
            item.first->inProgressLock.lock();
            item.first->playerPosition = world->currentPlayer->getPosition();
            for (auto& changedRigs:item.first->changedBoneTransforms) {
                world->changedBoneTransforms.emplace(changedRigs.first, changedRigs.second);
            }
            item.first->changedBoneTransforms.clear();
            item.first->processingDone = false;
            item.first->inProgressLock.unlock();
        }
    } else {
        // The start() method now handles the initial request creation.
        for (const auto &item: visibilityThreadPool) {
            item.first->playerPosition = world->currentPlayer->getPosition();
            fillVisibleObjectPerCamera(item.first);
            item.first->playerPosition = world->currentPlayer->getPosition();
            for (auto& changedRigs:item.first->changedBoneTransforms) {
                world->changedBoneTransforms.emplace(changedRigs.first, changedRigs.second);
            }
            item.first->changedBoneTransforms.clear();
        }
    }

    world->setPlayerAttachmentsForChangedBoneTransforms(world->startingPlayer.attachedModel);

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

std::map<VisibilityRequest*, SDL_Thread *> VisibilityManager::occlusionThreadManager() {
    std::map<VisibilityRequest*, SDL_Thread*> visibilityProcessing;
    for (auto &cameraVisibility: cullingResults) {
        VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->objects, cameraVisibility.second, world->currentPlayer->getPosition(), world->options, &wakeThreadsCondition);
        SDL_Thread* thread = SDL_CreateThread(staticOcclusionThread, request->camera->getName().c_str(), request);
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

void VisibilityManager::fillVisibleObjectPerCamera(const void* visibilityRequestRaw) {
    PROFILE_VISIBILITY("fillVisibleObjectPerCamera");
    const VisibilityRequest* visibilityRequest = static_cast<const VisibilityRequest *>(visibilityRequestRaw);
    ZoneNameV(___tracy_scoped_zone, visibilityRequest->camera->getName().c_str(), visibilityRequest->camera->getName().size());
    std::vector<long> lodDistances = visibilityRequest->lodDistancesOption.get();
    float skipRenderDistance = 0, skipRenderSize = 0, maxSkipRenderSize = 0;
    float objectAverageDepth;
    float objectScreenSize;
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
        if(!visibilityRequest->cameraIsDirty && !objectIt->second->isDirtyForFrustum() && skipOcclusionCulling) {
            continue; //if neither object nor camera dirty, no need to recalculate
        }
        Model *currentModel = dynamic_cast<Model *>(objectIt->second);
        if (currentModel == nullptr) {
            std::cerr << "model id " << objectIt->second << " is not a model?" << std::endl;
            exit(1);
        }
        bool isVisible = visibilityRequest->camera->isVisible(*currentModel);//find if visible
        for (auto& visibilityEntry: *visibilityRequest->visibility) {
            if (VisibilityRequest::isAnyTagMatch(visibilityEntry.first, currentModel->getTags())) {
                if(isVisible) {
                    const std::vector<Model::MeshMeta *> &meshMetas =currentModel->getMeshMetaData();
                    if (meshMetas.size() < static_cast<size_t>(splitModelToMeshCount)) {
                        totalCounter += meshMetas.size();
                        uint32_t lod = getLodLevel(lodDistances, skipRenderDistance, skipRenderSize, maxSkipRenderSize, cameraProjectionMatrix, visibilityRequest->playerPosition, objectIt->second->getAabbMin(), objectIt->second->getAabbMax(), objectAverageDepth, objectScreenSize);
                        if (lod != SKIP_LOD_LEVEL) {
                            if (objectScreenSize > softwareOcclusionOccluderSize || !runOcclusion) {
                                if (objectScreenSize > maxScreenSize) {
                                    maxScreenSize = objectScreenSize;
                                }
                                occluderCounter += meshMetas.size();
                                if (runOcclusion) {
                                    visibilityRequest->occlusionCuller.renderOccluder(currentModel);
                                    //std::cout << currentModel->getName() << ":" << " is occluder " << std::endl;
                                }
                                for (auto& meshMeta:meshMetas) {
                                    visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel, lod, objectAverageDepth);
                                }
                            } else {
                                visibilityRequest->occlusionCuller.addOccludee(currentModel, lod, objectAverageDepth, &visibilityEntry.second);
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
                                uint32_t lod = getLodLevel(lodDistances, skipRenderDistance, skipRenderSize, maxSkipRenderSize, cameraProjectionMatrix, visibilityRequest->playerPosition, meshWorldMin, meshWorldMax, objectAverageDepth, objectScreenSize);
                                if (lod != SKIP_LOD_LEVEL) {
                                    if (objectScreenSize > softwareOcclusionOccluderSize || !runOcclusion) {
                                        if (objectScreenSize > maxScreenSize) {
                                            maxScreenSize = objectScreenSize;
                                        }
                                        occluderCounter++;
                                        if (runOcclusion) {
                                            visibilityRequest->occlusionCuller.renderOccluder(meshMeta, currentModel->getTransformation()->getWorldTransform());
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
    if (ProfilerState::traceVisibility && fullProcessingPass) {
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

int VisibilityManager::staticOcclusionThread(void* visibilityRequestRaw) {
    VisibilityRequest* visibilityRequest = static_cast<VisibilityRequest *>(visibilityRequestRaw);
    // We are re ordering the logic so these threads are started and can be used,
    // but they are blocked until gameplay logic actually starts to request updates.
    visibilityRequest->started = true;

    while(visibilityRequest->running) {
        visibilityRequest->wakeCondition->waitCondition(visibilityRequest->blockMutex);
        if(!visibilityRequest->running) {
            break;
        }
        visibilityRequest->inProgressLock.lock();
        fillVisibleObjectPerCamera(visibilityRequestRaw);
        visibilityRequest->processingDone = true;
        visibilityRequest->inProgressLock.unlock();
    }
    return 0;
}

uint32_t VisibilityManager::getLodLevel(const std::vector<long>& lodDistances, float skipRenderDistance, float skipRenderSize, float maxSkipRenderSize, const glm::mat4 &cameraProjectionMatrix, const glm::vec3& playerPosition, glm::vec3 minAABB, glm::vec3 maxAABB, float &objectAverageDepth, float &objectScreenSize) {
    //now we get to calculate the size in screen
    glm::vec3 ndcMin, ndcMax;
    AABBConverter::getNCDAABB(minAABB, maxAABB, cameraProjectionMatrix, ndcMin, ndcMax);
    const float screenSizeX = (ndcMax.x - ndcMin.x) / 2.0f; // since OpenGL NDC is -1, 1 each side can be max 2, but we want 0,1
    const float screenSizeY = (ndcMax.y - ndcMin.y) / 2.0f;
    objectScreenSize = (screenSizeX * screenSizeY);

    objectAverageDepth = (ndcMax.z + ndcMin.z) / -2.0f;
    if(lodDistances.empty() && skipRenderDistance == 0.0) {
        return 0;
    }

    const float dx = std::max(minAABB.x - playerPosition.x, std::max(0.0f, playerPosition.x - maxAABB.x));
    const float dy = std::max(minAABB.y - playerPosition.y, std::max(0.0f, playerPosition.y - maxAABB.y));
    const float dz = std::max(minAABB.z - playerPosition.z, std::max(0.0f, playerPosition.z - maxAABB.z));
    const float distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    if(skipRenderDistance !=0 && distance > skipRenderDistance) {           //Is it distant enough to skip?
        if ((maxAABB.x - minAABB.x) < maxSkipRenderSize &&                    //Is it actually small enough to skip? We don't wanna skip mountains becuse they are far away.
            (maxAABB.y - minAABB.y) < maxSkipRenderSize )
            if(screenSizeX < skipRenderSize && screenSizeY < skipRenderSize) {  //Is it small enough in the screen to skip?
                return SKIP_LOD_LEVEL;
            }
    }

    for (size_t i = 0; i < lodDistances.size(); ++i) {
        if(distance < static_cast<float>(lodDistances[i])) {
            return i;
        }
    }
    //what if the distance is bigger than the last entry? we return the last LOD
    return lodDistances.size()-1;
}
