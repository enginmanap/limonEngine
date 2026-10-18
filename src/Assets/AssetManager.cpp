//
// Created by engin on 27.07.2016.
//

#include "AssetManager.h"
#include "AssetTreeManager.h"
#include "ModelAsset.h"

#include <algorithm>
#include <string>
#include "../LimonFileSystem.h"
#include "../Package/EngineDataFiles.h"

AssetManager::AssetManager(GraphicsInterface* graphicsWrapper, ALHelper *alHelper) : materialRegistry(graphicsWrapper), graphicsWrapper(graphicsWrapper), alHelper(alHelper),
    assetTree(new AssetTreeManager()) {
    nextAssetIndex.store(0);
    mainThread = std::this_thread::get_id();//constructed after the graphics context, on its thread
    loadPackageManifest();
    size_t num_threads = std::thread::hardware_concurrency();
    for (size_t i = 0; i < num_threads; ++i) {
        AssetLoadThreadState* state = new AssetLoadThreadState();
        state->running = true;
        state->thread = std::make_shared<std::thread>(&AssetManager::cpuLoadAsset, this, std::ref(state->running));
        assetLoadThreads.push_back(state);
    }
}

AssetManager::~AssetManager() {
    //Stop the cpu load threads
    for (auto thread_iterator = assetLoadThreads.begin(); thread_iterator != assetLoadThreads.end(); ++thread_iterator) {
        (*thread_iterator)->running = false;
    }
    for (auto thread_iterator = assetLoadThreads.begin(); thread_iterator != assetLoadThreads.end(); ++thread_iterator) {
        (*thread_iterator)->thread->join();
    }
    //free all the assets
    for (auto assetLoadIterator = assetLoadThreads.begin(); assetLoadIterator != assetLoadThreads.end(); ++assetLoadIterator) {
        delete *assetLoadIterator;
    }

}

void AssetManager::loadPackageManifest() {
    if (!LimonFileSystem::isPackageMounted()) {
        return;
    }
    tinyxml2::XMLDocument manifestDocument;
    if (LimonFileSystem::loadXML(EngineDataFiles::PACKAGE_MANIFEST, manifestDocument) != tinyxml2::XML_SUCCESS ||
        manifestDocument.FirstChildElement("PackManifest") == nullptr) {
        //every pack --pack builds has one, without it no packed model can load
        std::cerr << "Data.pak has no valid " << EngineDataFiles::PACKAGE_MANIFEST << ", it was not built by --pack. Exiting." << std::endl;
        exit(-1);
    }
    for (const tinyxml2::XMLElement *modelElement = manifestDocument.FirstChildElement("PackManifest")->FirstChildElement("Model");
         modelElement != nullptr; modelElement = modelElement->NextSiblingElement("Model")) {
        const char *key = modelElement->Attribute("key");
        const char *file = modelElement->Attribute("file");
        if (key == nullptr || file == nullptr) {
            std::cerr << "Model entry without key or file in " << EngineDataFiles::PACKAGE_MANIFEST << ", the package is broken. Exiting." << std::endl;
            exit(-1);
        }
        packedModelFiles[key] = file;
    }
    std::cout << "Data.pak converts " << packedModelFiles.size() << " model paths to .limonmodel files" << std::endl;
}

void LoadRequester::close() {
    assetManager->closeRequester(*this, true);
}

LoadRequester::~LoadRequester() {
    assetManager->closeRequester(*this, false);
}

std::unique_ptr<LoadRequester> AssetManager::openRequester(bool reportFailures, LoadRequester::GPUMode gpuMode, uint32_t maxInFlight) {
    std::unique_ptr<LoadRequester> requester(new LoadRequester(this, reportFailures, gpuMode, maxInFlight));
    std::lock_guard<std::mutex> lock(requestersMutex);
    //a thread never waits for its own requesters, only it could close them. Why is that? because otherwise, 2 request types from same thread
    // would be blocking each other on drain side, and deadlock. This is a comprimise, if your thread asks for contraticting things (exit or report) thats on you.
    if (runningRequesters.empty() || runningRequesters.front()->ownerThread == requester->ownerThread) {
        requester->running = true;
        runningRequesters.push_back(requester.get());
    } else {
        waitingRequesters.push_back(requester.get());
    }
    return requester;
}

bool AssetManager::isRunning(LoadRequester &requester) {
    std::lock_guard<std::mutex> lock(requestersMutex);
    return requester.running;
}

void AssetManager::exitUnlessGPUContextThread(const char *operation) const {
    if (std::this_thread::get_id() != mainThread) {
        std::cerr << operation << " called outside the gpu context thread, it has no graphics context there. Exiting..." << std::endl;
        exit(-1);
    }
}

void AssetManager::startPendingRequestsInternal(LoadRequester &requester) {
    while (!requester.pending.empty() && (requester.maxInFlight == 0 || requester.inFlight.size() + requester.finished.size() < requester.maxInFlight)) {
        LoadRequester::PendingRequest request = requester.pending.front();
        requester.pending.erase(requester.pending.begin());
        std::shared_ptr<Asset> asset;
        Asset::LoadState state;
        {
            std::lock_guard<std::mutex> lock(assetsMutex);
            std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>>::iterator existing = assets.find(request.files);
            if (existing == assets.end()) {
                asset = request.createAsset(this, getNextAssetIndex(), request.files);
                assets[request.files] = std::make_pair(asset, 1);
                assetLoadCpuQueue.pushBack(QueuedAsset{asset, requester.reportFailures, &requester});
            } else {
                existing->second.second++;
                asset = existing->second.first;
            }
            state = asset->getLoadState();
        }
        requester.inFlight.push_back(LoadRequester::TrackedRequest{request.files, asset, ""});
        if (state == Asset::LoadState::INITIATED || state == Asset::LoadState::CPU_LOAD_STARTED) {
            //the loader thread that finishes it hands it to every waiter. It sets the state under requestersMutex, so it can't finish unseen
            cpuLoadWaiters[asset.get()].push_back(&requester);
        } else {
            handOverInternal(requester, asset, "");
        }
    }
}

void AssetManager::handOverInternal(LoadRequester &requester, const std::shared_ptr<Asset> &asset, const std::string &failureReason) {
    std::vector<LoadRequester::TrackedRequest>::iterator trackedIt = requester.inFlight.begin();
    while (trackedIt != requester.inFlight.end() && trackedIt->asset != asset) {
        ++trackedIt;
    }
    if (trackedIt == requester.inFlight.end()) {
        return;
    }
    if (requester.gpuMode == LoadRequester::GPUMode::UPLOADED && asset->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
        assetLoadGPUQueue.push_back(GPULoadRequest{asset, &requester});//stays in flight until the world uploads it
        return;
    }
    trackedIt->failureReason = failureReason;
    requester.finished.push_back(*trackedIt);
    requester.inFlight.erase(trackedIt);
}

void AssetManager::finishCPULoad(const QueuedAsset &loaded, const std::string &failureReason) {
    std::lock_guard<std::mutex> lock(requestersMutex);
    if (!failureReason.empty()) {
        std::lock_guard<std::mutex> assetsLock(assetsMutex);
        std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>>::iterator failedEntry = assets.find(loaded.asset->fileList);
        if (failedEntry != assets.end() && failedEntry->second.first == loaded.asset) {
            assets.erase(failedEntry);//left in, the next request would get the failed asset back instead of trying again
        }
    }
    loaded.asset->setLoadState(failureReason.empty() ? Asset::LoadState::CPU_LOAD_DONE : Asset::LoadState::FAILED);
    std::map<const Asset *, std::vector<LoadRequester *>>::iterator waiters = cpuLoadWaiters.find(loaded.asset.get());
    if (waiters == cpuLoadWaiters.end()) {
        return;//a load another load started, its owner finds out through the asset
    }
    for (LoadRequester *waiter : waiters->second) {
        handOverInternal(*waiter, loaded.asset, failureReason);
    }
    cpuLoadWaiters.erase(waiters);
}

bool AssetManager::popFinishedRequest(LoadRequester &requester, LoadRequester::LoadResult &result, std::chrono::milliseconds wait) {
    if (std::this_thread::get_id() != requester.ownerThread) {
        std::cerr << "A requester's results are taken on a thread that didn't open it, a waiting load there could wait on itself." << std::endl;
    }
    std::unique_lock<std::mutex> lock(requestersMutex);
    if (requester.finished.empty()) {
        lock.unlock();
        {
            std::unique_lock<std::mutex> conditionLock(cpuLoadConditionMutex);
            cpuLoadDoneCondition.wait_for(conditionLock, wait);
        }
        lock.lock();
        if (requester.finished.empty()) {
            return false;
        }
    }
    result.files = requester.finished.front().files;
    result.asset = requester.finished.front().asset;
    result.failureReason = requester.finished.front().failureReason;
    requester.finished.erase(requester.finished.begin());
    startPendingRequestsInternal(requester);
    return true;
}

void AssetManager::uploadOneQueuedAsset() {
    GPULoadRequest upload;
    {
        std::lock_guard<std::mutex> lock(requestersMutex);
        if (assetLoadGPUQueue.empty()) {
            return;
        }
        upload = assetLoadGPUQueue.front();
        assetLoadGPUQueue.erase(assetLoadGPUQueue.begin());
    }
    partialLoadGPUSide(upload.asset);
    {
        std::lock_guard<std::mutex> lock(requestersMutex);
        //it may have closed while the lock was released for the upload
        if (std::find(runningRequesters.begin(), runningRequesters.end(), upload.requester) != runningRequesters.end()) {
            handOverInternal(*upload.requester, upload.asset, "");
        }
    }
    cpuLoadDoneCondition.notify_all();
}

void AssetManager::startNextRequestersInternal() {
    if (waitingRequesters.empty()) {
        return;
    }
    //report only first, then the ones skipping GPU parts, then the order they were opened in
    LoadRequester *next = nullptr;
    for (LoadRequester *candidate : waitingRequesters) {
        if (next == nullptr || (candidate->reportFailures && !next->reportFailures) ||
            (candidate->reportFailures == next->reportFailures && candidate->gpuMode == LoadRequester::GPUMode::SKIP && next->gpuMode != LoadRequester::GPUMode::SKIP)) {
            next = candidate;
        }
    }
    //all of that thread's waiting requesters together, one of them waiting for another would never end
    std::thread::id nextOwner = next->ownerThread;
    for (std::vector<LoadRequester *>::iterator waitingIt = waitingRequesters.begin(); waitingIt != waitingRequesters.end(); ) {
        if ((*waitingIt)->ownerThread != nextOwner) {
            ++waitingIt;
            continue;
        }
        LoadRequester *starting = *waitingIt;
        waitingIt = waitingRequesters.erase(waitingIt);
        starting->running = true;
        runningRequesters.push_back(starting);
        startPendingRequestsInternal(*starting);
    }
}

// It is possible that the thread that requested loads give up and closed the request. In that case, we still need to clean up:
// the loads it started are in the CPU queue or on a loader thread, and they come back to it when they finish
void AssetManager::waitForStartedLoadsInternal(LoadRequester &requester, std::unique_lock<std::mutex> &requestersLock) {
    bool loading = true;
    while (loading) {
        loading = false;
        for (const LoadRequester::TrackedRequest &trackedRequest : requester.inFlight) {
            Asset::LoadState state = trackedRequest.asset->getLoadState();
            if (state == Asset::LoadState::INITIATED || state == Asset::LoadState::CPU_LOAD_STARTED) {
                loading = true;
                break;
            }
        }
        if (loading) {
            requestersLock.unlock();
            {
                std::unique_lock<std::mutex> conditionLock(cpuLoadConditionMutex);
                cpuLoadDoneCondition.wait_for(conditionLock, std::chrono::milliseconds{5});
            }
            requestersLock.lock();
        }
    }
}

void AssetManager::releaseReferencesInternal(const std::vector<LoadRequester::TrackedRequest> &requests) {
    for (const LoadRequester::TrackedRequest &trackedRequest : requests) {
        if (trackedRequest.asset->getLoadState() != Asset::LoadState::FAILED) {//a failed one is out of the map, with its reference
            freeAsset(trackedRequest.files);
        }
    }
}

void AssetManager::closeRequester(LoadRequester &requester, bool fromOwner) {
    if (fromOwner && std::this_thread::get_id() != requester.ownerThread) {
        std::cerr << "A requester is closed on a thread that didn't open it." << std::endl;
    }
    std::unique_lock<std::mutex> lock(requestersMutex);
    if (requester.closed) {
        return;
    }
    requester.closed = true;
    requester.pending.clear();
    waitForStartedLoadsInternal(requester, lock);
    for (std::vector<GPULoadRequest>::iterator uploadIt = assetLoadGPUQueue.begin(); uploadIt != assetLoadGPUQueue.end(); ) {
        if (uploadIt->requester == &requester) {
            uploadIt = assetLoadGPUQueue.erase(uploadIt);
        } else {
            ++uploadIt;
        }
    }
    releaseReferencesInternal(requester.inFlight);
    releaseReferencesInternal(requester.finished);
    requester.inFlight.clear();
    requester.finished.clear();
    if (requester.running) {
        requester.running = false;
        runningRequesters.erase(std::find(runningRequesters.begin(), runningRequesters.end(), &requester));
        if (runningRequesters.empty()) {
            startNextRequestersInternal();
        }
    } else {
        waitingRequesters.erase(std::find(waitingRequesters.begin(), waitingRequesters.end(), &requester));
    }
}
