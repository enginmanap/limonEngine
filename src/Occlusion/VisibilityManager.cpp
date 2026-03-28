#include "VisibilityManager.h"
#include "World.h"
#include "Camera/Camera.h"
#include "Graphics/GraphicsPipeline.h"
#include "GameObjects/Model.h"
#include "GameObjects/Players/Player.h"

VisibilityManager::VisibilityManager(World* world) : world(world) {
    OptionsUtil::Options::Option<bool> multiThreadCullingOption = world->options->getOption<bool>(HASH("multiThreadedCulling"));
    multiThreadedCulling = multiThreadCullingOption.getOrDefault(true);
}

VisibilityManager::~VisibilityManager() {
    for (auto &item: visibilityThreadPool) {
        item.first->running = false;
    }
    for (auto &item: visibilityThreadPool) {

        item.first->waitMainThreadCondition.signalWaiting();
        if (item.second) {
            SDL_WaitThread(item.second, NULL);
        }
        delete item.first;
    }
}

void VisibilityManager::update() {
    fillVisibleObjectsUsingTags();
}

void VisibilityManager::onPipelineChange() {
    resetTagsAndRefillCulling();
}

std::unordered_map<Camera*, std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>*>& VisibilityManager::getCullingResults() {
    return cullingResults;
}

void VisibilityManager::addCamera(Camera* camera) {
    cullingResults.insert(std::make_pair(camera, new std::unordered_map<std::vector<uint64_t>, RenderList, VisibilityRequest::uint64_vector_hasher>()));
}

void VisibilityManager::removeCamera(Camera* camera) {
    cullingResults.erase(camera);
}

void VisibilityManager::fillVisibleObjectsUsingTags() {
    //first clear up dirty cameras, and perspective camera, because Perspective camera needs to write occlusion culling depth map, so it has to iterate over all.
    for (auto &it: cullingResults) {
        if (it.first->isDirty() || it.first->getType() == Camera::CameraTypes::PERSPECTIVE) {
            for(auto renderEntries:*it.second) {
                renderEntries.second.clear();
            }
        }
    }
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
            //std::this_thread::sleep_for(std::chrono::milliseconds(1000));//make sure all threads are started before continuing.
        }

        //std::cout << "          new frame, trigger occlusion threads" << std::endl;
        VisibilityRequest::waitMainThreadCondition.signalWaiting();
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
        if(visibilityThreadPool.empty()) {
            for (auto &cameraVisibility: cullingResults) {
                VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->objects, cameraVisibility.second, world->currentPlayer->getPosition(), world->options);
                visibilityThreadPool[request] = nullptr;
            }
        }
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
        VisibilityRequest* request = new VisibilityRequest(cameraVisibility.first, &world->objects, cameraVisibility.second, world->currentPlayer->getPosition(), world->options);
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
    const VisibilityRequest* visibilityRequest = static_cast<const VisibilityRequest *>(visibilityRequestRaw);
    std::vector<long> lodDistances = visibilityRequest->lodDistancesOption.get();
    float skipRenderDistance = 0, skipRenderSize = 0, maxSkipRenderSize = 0;
    float objectAverageDepth;
    float objectScreenSize;
    long splitModelToMeshCount;
    bool softwareOcclusionRenderDump = false;
    long softwareOcclusionRenderDumpFrequency = 500;
    float softwareOcclusionOccluderSize = 0.25f;
    glm::mat4 viewMatrix;
    glm::vec3 viewDirection;
    glm::vec3 cameraPos;
    splitModelToMeshCount = visibilityRequest->SplitModelToMeshCountOption.get();
    if(visibilityRequest->camera->getType() == Camera::CameraTypes::PERSPECTIVE ||
       visibilityRequest->camera->getType() == Camera::CameraTypes::ORTHOGRAPHIC) {
        skipRenderDistance = visibilityRequest->skipRenderDistanceOption.get();
        skipRenderSize = visibilityRequest->skipRenderSizeOption.get();
        maxSkipRenderSize = visibilityRequest->maxSkipRenderSizeOption.get();
        viewMatrix = visibilityRequest->camera->getProjectionMatrix() * visibilityRequest->camera->getCameraMatrixConst();
    }
    static int frameCount = 0;

    bool skipOcclusionCulling = false;
    if (visibilityRequest->camera->getType() != Camera::CameraTypes::PERSPECTIVE) {
        skipOcclusionCulling = true;
    } else {
        glm::mat4 invertedView = glm::inverse(visibilityRequest->camera->getCameraMatrixConst());
        viewDirection = -glm::vec3(invertedView[2]);
        viewDirection = glm::normalize(viewDirection);
        cameraPos = glm::vec3(invertedView[3]); // 4th column
        softwareOcclusionRenderDump = visibilityRequest->SoftwareOcclusionRenderDumpOption.getOrDefault(false);
        softwareOcclusionRenderDumpFrequency = visibilityRequest->SoftwareOcclusionRenderDumpFrequencyOption.getOrDefault(500);
        softwareOcclusionOccluderSize = visibilityRequest->SoftwareOcclusionOccluderSizeOption.getOrDefault(0.25f);

        visibilityRequest->occlusionCuller.newFrame(cameraPos, viewDirection, visibilityRequest->camera->getCameraMatrixConst(), visibilityRequest->camera->getProjectionMatrix());
    }
    uint32_t frustumCulledCount = 0;
    uint32_t totalCounter = 0;
    uint32_t lodSkipCounter = 0;
    uint32_t occluderCounter = 0;
    uint32_t occludedCounter = 0;
    float maxScreenSize = 0.0;
    std::string maxScreenSizeObjectName;
    for (auto objectIt = visibilityRequest->objects->begin(); objectIt != visibilityRequest->objects->end(); ++objectIt) {
        if(!visibilityRequest->camera->isDirty() && !objectIt->second->isDirtyForFrustum() && skipOcclusionCulling) {
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
                        uint32_t lod = getLodLevel(lodDistances, skipRenderDistance, skipRenderSize, maxSkipRenderSize, viewMatrix, visibilityRequest->playerPosition, objectIt->second->getAabbMin(), objectIt->second->getAabbMax(), objectAverageDepth, objectScreenSize);
                        if (lod != SKIP_LOD_LEVEL) {
                            if (objectScreenSize > softwareOcclusionOccluderSize || skipOcclusionCulling) {
                                if (objectScreenSize > maxScreenSize) {
                                    maxScreenSize = objectScreenSize;
                                    maxScreenSizeObjectName = currentModel->getName();
                                }
                                occluderCounter += meshMetas.size();
                                if (!skipOcclusionCulling) {
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
                        }
                    } else {
                        //for models with more than 10 meshes, we don't wanna add all of them to renderlist, need to re check visibility
                        for (auto& meshMeta:meshMetas) {
                            totalCounter++;
                            if (visibilityRequest->camera->isVisible(currentModel->getTransformation()->getWorldTransform() * meshMeta->mesh->getAabbMin(),
                                                                     currentModel->getTransformation()->getWorldTransform() * meshMeta->mesh->getAabbMax())) {
                                uint32_t lod = getLodLevel(lodDistances, skipRenderDistance, skipRenderSize, maxSkipRenderSize, viewMatrix, visibilityRequest->playerPosition, meshMeta->mesh->getAabbMin(), meshMeta->mesh->getAabbMax(), objectAverageDepth, objectScreenSize);
                                if (lod != SKIP_LOD_LEVEL) {
                                    if (objectScreenSize > 0.25f || skipOcclusionCulling) {
                                        if (objectScreenSize > maxScreenSize) {
                                            maxScreenSize = objectScreenSize;
                                            maxScreenSizeObjectName = currentModel->getName();
                                        }
                                        occluderCounter++;
                                        if (!skipOcclusionCulling) {
                                            visibilityRequest->occlusionCuller.renderOccluder(meshMeta, currentModel->getTransformation()->getWorldTransform());
                                        }
                                        visibilityEntry.second.addMeshMaterial(meshMeta->material, meshMeta->mesh, currentModel, lod, objectAverageDepth);
                                    } else {
                                        visibilityRequest->occlusionCuller.addOccludee(meshMeta, currentModel, lod, objectAverageDepth, &visibilityEntry.second);
                                    }
                                } else {
                                    lodSkipCounter++;
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
    if (!skipOcclusionCulling) {
        std::vector<OcculudeeMetaData*> nonOccludedMeshes = visibilityRequest->occlusionCuller.getNonOccludedMeshMeta();
        for (auto metaData:nonOccludedMeshes) {
            metaData->renderList->addMeshMaterial(metaData->meshMeta->material, metaData->meshMeta->mesh, metaData->model, metaData->lod, metaData->averageDepth);
        }
        occludedCounter = totalCounter - occluderCounter - nonOccludedMeshes.size();
        if (occluderCounter != 0 && occludedCounter != 0) {
            //std::cout << "Total occluder count is " << occluderCounter << " and it occluded " << occludedCounter << std::endl;
        }
        frameCount++;
        if (frameCount == softwareOcclusionRenderDumpFrequency) {
            if (softwareOcclusionRenderDump) {
                visibilityRequest->occlusionCuller.dumpDepth();
            }
            frameCount = 0;
        }
    }
}

int VisibilityManager::staticOcclusionThread(void* visibilityRequestRaw) {
    VisibilityRequest* visibilityRequest = static_cast<VisibilityRequest *>(visibilityRequestRaw);
    //std::cout << "Thread for  " << visibilityRequest->camera->getName() << " launched, waiting for condition" << std::endl;
    //std::cout << "Thread for  " << visibilityRequest->camera->getName() << " started" << std::endl;

    while(visibilityRequest->running) {
        visibilityRequest->inProgressLock.lock();
        visibilityRequest->started = true;
        fillVisibleObjectPerCamera(visibilityRequestRaw);
        visibilityRequest->processingDone = true;
        visibilityRequest->inProgressLock.unlock();
        //std::cout << "Processing done for camera " << visibilityRequest->camera->getName() << " now waiting for condition" << std::endl;
        VisibilityRequest::waitMainThreadCondition.waitCondition(visibilityRequest->blockMutex);
        //std::cout << "signal received by " << visibilityRequest->camera->getName() << " starting processing again" << std::endl;
    }
    visibilityRequest->inProgressLock.unlock();
    return 0;
}

uint32_t VisibilityManager::getLodLevel(const std::vector<long>& lodDistances, float skipRenderDistance, float skipRenderSize, float maxSkipRenderSize, const glm::mat4 &viewMatrix, const glm::vec3& playerPosition, glm::vec3 minAABB, glm::vec3 maxAABB, float &objectAverageDepth, float &objectScreenSize) {
    //now we get to calculate the size in screen
    glm::vec3 ndcMin, ndcMax;
    AABBConverter::getNCDAABB(minAABB, maxAABB, viewMatrix, ndcMin, ndcMax);
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
