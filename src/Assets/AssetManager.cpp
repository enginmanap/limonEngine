//
// Created by engin on 27.07.2016.
//

#include "AssetManager.h"
#include "ModelAsset.h"

#include <algorithm>
#include <string>
#include "../LimonFileSystem.h"
#include "../Package/EngineDataFiles.h"

void AssetManager::addAssetsRecursively(const std::string &directoryPath, const std::string &fileName,
                                        const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions,
                                        AvailableAssetsNode* nodeToProcess) {
    nodeToProcess->name = fileName;
    nodeToProcess->nameLower = nodeToProcess->name;
    std::transform(nodeToProcess->nameLower.begin(), nodeToProcess->nameLower.end(), nodeToProcess->nameLower.begin(), ::tolower);

    nodeToProcess->fullPath = directoryPath;

    std::vector<LimonFileSystem::DirectoryEntry> entries;
    if (!LimonFileSystem::listDirectory(directoryPath, entries)) {
        return;
    }

    nodeToProcess->assetType = Asset_type_DIRECTORY;

    for (const LimonFileSystem::DirectoryEntry &entry : entries) {
        std::string filePath = directoryPath + "/" + entry.name;//this is the asset name the editor writes to maps, keep the "./" form
        if (entry.isDirectory) {
            //we found a child, and it is a directory. Create a node and recurse
            AvailableAssetsNode* newNode = new AvailableAssetsNode();
            newNode->parent = nodeToProcess;
            nodeToProcess->children.push_back(newNode);
            addAssetsRecursively(filePath, entry.name, fileExtensions, newNode);

        } else {
            AssetTypes assetType;
            if(isExtensionInList(entry.name, fileExtensions, assetType)) {
                //we found a element that is an asset create and add to children
                AvailableAssetsNode* newNode = new AvailableAssetsNode;
                newNode->fullPath = filePath;
                newNode->name = entry.name;
                newNode->nameLower = newNode->name;
                std::transform(newNode->nameLower.begin(), newNode->nameLower.end(), newNode->nameLower.begin(), ::tolower);
                newNode->assetType = assetType;
                newNode->parent = nodeToProcess;
                nodeToProcess->children.push_back(newNode);
                //std::cout << entry.name << " added as asset. with type " << assetType << std::endl;
            } else {
                //file found but not added because extension is not in list
            }

        }
    }
}


std::vector<std::pair<std::string, AssetManager::AssetTypes>> AssetManager::loadAssetExtensionList() {
    std::vector<std::pair<std::string, AssetTypes>> extensionList;
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = LimonFileSystem::loadXML(ASSET_EXTENSIONS_FILE, xmlDoc);
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading Asset file list XML: " <<  tinyxml2::XMLDocument::ErrorIDToName(eResult) << std::endl;
        return extensionList;
    }
    tinyxml2::XMLNode * assetsNode = xmlDoc.FirstChild();
    if (assetsNode == nullptr) {
        std::cerr << "Asset list xml is not a valid XML." << std::endl;
        return extensionList;
    }
    const char* typeNameRaw;
    tinyxml2::XMLElement* currentAssetNode =  assetsNode->FirstChildElement("Asset");
    while(currentAssetNode != nullptr) {
        typeNameRaw = currentAssetNode->Attribute("type");

        if (typeNameRaw != nullptr) {
            std::string typeName = typeNameRaw;
            AssetManager::AssetTypes currentAssetType;
            if (typeName == "Model") {
                currentAssetType = Asset_type_MODEL;
            } else if (typeName == "Texture") {
                currentAssetType = Asset_type_TEXTURE;
            } else if(typeName == "Sound") {
                currentAssetType = Asset_type_SOUND;
            } else if(typeName == "Graphics Program") {
                currentAssetType = Asset_type_GRAPHICSPROGRAM;
            } else {
                std::cerr << "Requested asset type is not implemented yet. exiting.." << std::endl;
                exit(1);
            }
            extensionList.push_back(std::pair<std::string, AssetTypes>(currentAssetNode->GetText(), currentAssetType));
            //std::cout << "adding available asset extension " << currentAssetNode->GetText() << std::endl;
        } else {
            std::cerr << "Asset extension without a type is invalid, skipping" << std::endl;
        }
        currentAssetNode = currentAssetNode->NextSiblingElement("Asset");
    }
    return extensionList;
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

bool AssetManager::loadAssetList() {
    std::vector<std::pair<std::string, AssetTypes>> fileExtensions = loadAssetExtensionList();
    availableAssetsRootNode = new AvailableAssetsNode();
    availableAssetsRootNode->assetType = Asset_type_DIRECTORY;
    availableAssetsRootNode->fullPath = ".";
    availableAssetsRootNode->nameLower = "";
    availableAssetsRootNode->name = "Assets";
    AvailableAssetsNode* data = new AvailableAssetsNode;
    addAssetsRecursively("./Data", "Data", fileExtensions, data);
    availableAssetsRootNode->children.push_back(data);
    AvailableAssetsNode* engine = new AvailableAssetsNode;
    addAssetsRecursively("./Engine", "Engine", fileExtensions, engine);
    availableAssetsRootNode->children.push_back(engine);
    return true;
}

bool AssetManager::isExtensionInList(const std::string &name, const std::vector<std::pair<std::string, AssetTypes>> &vector,
                                     AssetTypes &elementAssetType) {
    //lowercase the string:
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);

    for (uint32_t i = 0; i < vector.size(); ++i) {
        if(isEnding(lowerCaseName, vector[i].first)) {
            elementAssetType = vector[i].second;
            return true;
        }
    }
    return false;
}

const AssetManager::AvailableAssetsNode *
AssetManager::getAvailableAssetsTreeFiltered(AssetManager::AssetTypes type, const std::string &filterText) {
    std::string filterTextLower = filterText;
    std::transform(filterTextLower.begin(), filterTextLower.end(), filterTextLower.begin(), ::tolower);
    std::pair<AssetTypes , std::string> key = std::make_pair(type,filterTextLower);
    std::lock_guard<std::mutex> lock(availableAssetsMutex);
    if(filteredResults.find(key) != filteredResults.end()) {
        return filteredResults[key];
    }
    //now we should build another tree with given filters
    AssetManager::AvailableAssetsNode *root = getAvailableAssetsTreeFilteredRecursive(availableAssetsRootNode, type, filterTextLower);
    filteredResults[key] = root;
    return root;
}

AssetManager::AvailableAssetsNode *
AssetManager::getAvailableAssetsTreeFilteredRecursive(const AvailableAssetsNode * const assetsNode,
                                                      AssetManager::AssetTypes type,
                                                      const std::string &filterText) {
    AssetManager::AvailableAssetsNode *newAssetsNode = nullptr;
    if(assetsNode->assetType == type) {
        if(assetsNode->nameLower.find(filterText) != std::string::npos) {
            AvailableAssetsNode* newAssetNode = new AvailableAssetsNode();
            newAssetNode->name = assetsNode->name;
            newAssetNode->fullPath = assetsNode->fullPath;
            newAssetNode->nameLower = assetsNode->nameLower;
            newAssetNode->assetType = assetsNode->assetType;
            //not setting parent, because if this was called recursively, caller sets it, if root it should be null
            return newAssetNode;
        } else {
            return nullptr;
        }
    } else if(assetsNode->assetType != Asset_type_DIRECTORY) {
        return nullptr;
    }
    //at this point, we made sure the asset node type is directory, and any child that doesn't match filters will return null.
    for (size_t i = 0; i < assetsNode->children.size(); ++i) {
        AssetManager::AvailableAssetsNode *childAssetNode = getAvailableAssetsTreeFilteredRecursive(assetsNode->children[i], type, filterText);
        if(childAssetNode != nullptr) {
            if(newAssetsNode == nullptr) {
                newAssetsNode = new AvailableAssetsNode();
                newAssetsNode->name = assetsNode->name;
                newAssetsNode->nameLower = assetsNode->nameLower;
                newAssetsNode->fullPath = assetsNode->fullPath;
                newAssetsNode->assetType = assetsNode->assetType;//directory known
                //parent = nullptr;
            }
            childAssetNode->parent = newAssetsNode;
            newAssetsNode->children.push_back(childAssetNode);
        }
    }
    return newAssetsNode;
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
    while (!requester.pending.empty() && (requester.maxInFlight == 0 || requester.tracked.size() < requester.maxInFlight)) {
        LoadRequester::PendingRequest request = requester.pending.front();
        requester.pending.erase(requester.pending.begin());
        std::shared_ptr<Asset> asset;
        std::lock_guard<std::mutex> lock(assetsMutex);
        std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>>::iterator existing = assets.find(request.files);
        if (existing == assets.end()) {
            asset = request.createAsset(this, getNextAssetIndex(), request.files);
            assets[request.files] = std::make_pair(asset, 1);
            assetLoadCpuQueue.pushBack(QueuedAsset{asset, requester.reportFailures, &requester});
        } else {
            //loaded, or loading for another requester of this thread, it is handed over once it is far enough
            existing->second.second++;
            asset = existing->second.first;
        }
        requester.tracked.push_back(LoadRequester::TrackedRequest{request.files, asset});
    }
}

bool AssetManager::takeFinishedRequestInternal(LoadRequester &requester, LoadRequester::LoadResult &result) {
    if (!requester.running) {
        return false;
    }
    for (std::vector<LoadRequester::TrackedRequest>::iterator trackedIt = requester.tracked.begin(); trackedIt != requester.tracked.end(); ++trackedIt) {
        Asset::LoadState state = trackedIt->asset->getLoadState();
        if (state == Asset::LoadState::INITIATED || state == Asset::LoadState::CPU_LOAD_STARTED) {
            continue;
        }
        if (requester.gpuMode == LoadRequester::GPUMode::UPLOADED && state == Asset::LoadState::CPU_LOAD_DONE) {
            if (!trackedIt->uploadQueued) {
                assetLoadGPUQueue.push_back(trackedIt->asset);
                trackedIt->uploadQueued = true;
            }
            continue;
        }
        result.files = trackedIt->files;
        result.asset = trackedIt->asset;
        result.failureReason.clear();
        if (state == Asset::LoadState::FAILED) {
            std::map<const Asset *, std::string>::iterator reason = requester.failureReasons.find(trackedIt->asset.get());
            if (reason != requester.failureReasons.end()) {
                result.failureReason = reason->second;
                requester.failureReasons.erase(reason);
            } else {
                result.failureReason = "it failed while loading for another requester";
            }
        }
        requester.tracked.erase(trackedIt);
        startPendingRequestsInternal(requester);
        return true;
    }
    return false;
}

bool AssetManager::popFinishedRequest(LoadRequester &requester, LoadRequester::LoadResult &result, std::chrono::milliseconds wait) {
    if (std::this_thread::get_id() != requester.ownerThread) {
        std::cerr << "A requester's results are taken on a thread that didn't open it, a waiting load there could wait on itself." << std::endl;
    }
    {
        std::lock_guard<std::mutex> lock(requestersMutex);
        if (takeFinishedRequestInternal(requester, result)) {
            return true;
        }
    }
    {
        std::unique_lock<std::mutex> conditionLock(cpuLoadConditionMutex);
        cpuLoadDoneCondition.wait_for(conditionLock, wait);
    }
    std::lock_guard<std::mutex> lock(requestersMutex);
    return takeFinishedRequestInternal(requester, result);
}

void AssetManager::uploadOneQueuedAsset() {
    std::shared_ptr<Asset> asset;
    {
        std::lock_guard<std::mutex> lock(requestersMutex);
        if (assetLoadGPUQueue.empty()) {
            return;
        }
        asset = assetLoadGPUQueue.front();
        assetLoadGPUQueue.erase(assetLoadGPUQueue.begin());
    }
    partialLoadGPUSide(asset);
}

void AssetManager::recordRequesterFailure(LoadRequester &requester, const std::shared_ptr<Asset> &asset, const std::string &reason) {
    std::lock_guard<std::mutex> lock(requestersMutex);
    for (const LoadRequester::TrackedRequest &trackedRequest : requester.tracked) {
        if (trackedRequest.asset != asset) {
            continue;//a load this one started, its owner finds out through the asset
        }
        requester.failureReasons[asset.get()] = reason;
        std::lock_guard<std::mutex> assetsLock(assetsMutex);
        std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>>::iterator failedEntry = assets.find(trackedRequest.files);
        if (failedEntry != assets.end() && failedEntry->second.first == asset) {
            assets.erase(failedEntry);//left in, the next request would get the failed asset back instead of trying again
        }
        return;
    }
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
    // It is possible that the thread that requested loads give up and closed the request. In that case, we still need to clean up
    bool loading = true;
    while (loading) {
        loading = false;
        for (const LoadRequester::TrackedRequest &trackedRequest : requester.tracked) {
            Asset::LoadState state = trackedRequest.asset->getLoadState();
            if (state == Asset::LoadState::INITIATED || state == Asset::LoadState::CPU_LOAD_STARTED) {
                loading = true;
                break;
            }
        }
        if (loading) {
            lock.unlock();
            {
                std::unique_lock<std::mutex> conditionLock(cpuLoadConditionMutex);
                cpuLoadDoneCondition.wait_for(conditionLock, std::chrono::milliseconds{5});
            }
            lock.lock();
        }
    }
    for (const LoadRequester::TrackedRequest &trackedRequest : requester.tracked) {
        if (trackedRequest.uploadQueued) {
            std::vector<std::shared_ptr<Asset>>::iterator queuedUpload = std::find(assetLoadGPUQueue.begin(), assetLoadGPUQueue.end(), trackedRequest.asset);
            if (queuedUpload != assetLoadGPUQueue.end()) {
                assetLoadGPUQueue.erase(queuedUpload);
            }
        }
        if (trackedRequest.asset->getLoadState() != Asset::LoadState::FAILED) {//a failed one is out of the map, with its reference
            freeAsset(trackedRequest.files);
        }
    }
    requester.tracked.clear();
    requester.failureReasons.clear();
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
