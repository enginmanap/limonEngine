//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSETMANAGER_H
#define LIMONENGINE_ASSETMANAGER_H


#include <string>
#include <map>
#include <set>
#include <utility>
#include <tinyxml2.h>
#include <fstream>
#ifdef CEREAL_SUPPORT
#include <cereal/archives/xml.hpp>
#include <Cereal/include/cereal/archives/binary.hpp>
#endif

#include "Asset.h"
#include "LoadRequester.h"
#include "MaterialRegistry.h"
#include "../LimonFileSystem.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

class GraphicsInterface;
class ALHelper;
class AssetTreeManager;
class Material;
class World;//only ever used as an opaque owner token for material override rules

class AssetManager {
public:
    struct EmbeddedTexture {
        char format[9] = "\0";
        uint32_t height = 0;
        uint32_t width = 0;
        std::vector<uint8_t> texelData;

        bool checkFormat(const char* s) const {
            if (nullptr == s) {
                return false;
            }
            return (0 == ::strncmp(format, s, sizeof(format)));
        }

        EmbeddedTexture() = default;

        EmbeddedTexture(const EmbeddedTexture& texture2) {
            memcpy(this->format, texture2.format, 9);
            this->height = texture2.height;
            this->width = texture2.width;
            if(this->height != 0) {
                this->texelData.resize(this->height * this->width);
                memcpy(this->texelData.data(), texture2.texelData.data(), this->height* this->width);
            } else {
                //compressed data
                this->texelData.resize(this->width);
                memcpy(this->texelData.data(), texture2.texelData.data(), this->width);
            }

        }

        template<class Archive>
        void serialize( Archive & ar ) {
            ar(format, height, width, texelData );
        }

    };

    //an owner's own threads loading for its requester, like the converter's piece builders. Their loads then go to that requester
    class LoadingForScope {
        LoadRequester *previous;
    public:
        explicit LoadingForScope(LoadRequester &requester) : previous(loadingFor) {
            loadingFor = &requester;
        }
        ~LoadingForScope() {
            loadingFor = previous;
        }
        LoadingForScope(const LoadingForScope &) = delete;
        LoadingForScope &operator=(const LoadingForScope &) = delete;
    };

private:
    struct QueuedAsset {
        std::shared_ptr<Asset> asset;
        bool reportFailures;//the requester's mode, the loader thread takes it over
        LoadRequester *requester;//the loads a load starts carry its requester too
    };

    class AssetLoadQueue {
    private:
        std::vector<QueuedAsset> assets;
        std::mutex queueMutex;
        std::condition_variable queueEmptyCond;
    public:
        void pushBack(const QueuedAsset &queuedAsset) {
            std::lock_guard<std::mutex> lock(queueMutex);
            assets.push_back(queuedAsset);
            queueEmptyCond.notify_one();
        }

        bool popFrontOrBlockFor(const std::chrono::milliseconds milliseconds, QueuedAsset &queuedAsset) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueEmptyCond.wait_for(lock, milliseconds);
            if (assets.empty()) {
                return false;
            }
            queuedAsset = assets.front();
            assets.erase(assets.begin());
            return true;
        }
    };

    struct GPULoadRequest {
        std::shared_ptr<Asset> asset;
        LoadRequester *requester;//who gets it once it is uploaded
    };

    struct AssetLoadThreadState {
        std::shared_ptr<std::thread> thread;
        bool running = true;
    };

    //the requester this thread loads for: a loader thread inside a CPU part, or an owner's own worker in a LoadingForScope
    inline static thread_local LoadRequester *loadingFor = nullptr;//Thread local means it will have copies for caller thread, not the class member, which is super weird c++ thingy.

    std::mutex cpuLoadConditionMutex;
    std::mutex assetsMutex; //guards the `assets` map only; must be held for every insert/find/erase/refcount change on it, but released before any actual loading work
    std::mutex requestersMutex;//guards the requester lists and the upload queue, taken before assetsMutex when both are held
    std::mutex embeddedTexturesMutex;//models add theirs from loadCPUPart, on the loader threads
    std::condition_variable cpuLoadDoneCondition;

    //loading
    AssetLoadQueue assetLoadCpuQueue;
    std::vector<AssetLoadThreadState*> assetLoadThreads;
    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>> assets;
    std::atomic<std::int32_t> nextAssetIndex;
    //model key to the .limonmodel a package carries instead of the raw model, both normalized. Filled once before any load
    std::unordered_map<std::string, std::string> packedModelFiles;

    //one thread's requesters run together, the others wait for all of them to close. Only running ones reach the CPU queue
    std::vector<LoadRequester *> waitingRequesters;//in the order they were opened
    std::vector<LoadRequester *> runningRequesters;
    std::map<const Asset *, std::vector<LoadRequester *>> cpuLoadWaiters;//loads in the CPU queue or on a loader thread, and who gets them when done
    std::vector<GPULoadRequest> assetLoadGPUQueue;//CPU parts done for UPLOADED requesters, the world runs one GPU part per frame
    std::thread::id mainThread;//owns the graphics context, GPU parts run only there

    std::unordered_map<std::string, std::vector<std::shared_ptr<const EmbeddedTexture>>> embeddedTextures;

    //its own object because it shares no state with asset caching, neither touches the other's members
    MaterialRegistry materialRegistry;
    GraphicsInterface* graphicsWrapper;
    ALHelper *alHelper;
    std::unique_ptr<AssetTreeManager> assetTree;

    //set by a loader thread for the request it is loading, so the loads that load starts take the same mode.
    //function locals, a thread_local std::string member gets an init routine in every translation unit and mingw refuses the duplicates
    static bool &reportLoadFailures() {
        thread_local bool report = false;
        return report;
    }

    static std::string &loadFailureReason() {
        thread_local std::string reason;
        return reason;
    }

    //only a request made in report mode can leave an asset FAILED, a game caller that needed it stops the engine
    static void exitUnlessReporting(const std::shared_ptr<Asset> &failedAsset) {
        if (!reportLoadFailures()) {
            std::cerr << failedAsset->getAssetName() << " could not be loaded, the reason is above. Exiting..." << std::endl;
            exit(-1);
        }
    }

    uint32_t getNextAssetIndex() {
        return ++nextAssetIndex;
    }

    void cpuLoadAsset(bool& running) {
        while(running) {
            //inside the loop: a copy kept to the next load could be an asset's last owner, and it would be destroyed here, without the graphics context
            QueuedAsset assetAndLoadNext;
            if (!assetLoadCpuQueue.popFrontOrBlockFor(std::chrono::milliseconds(5), assetAndLoadNext)) {
                continue;
            }
            if(assetAndLoadNext.asset->getLoadState() != Asset::LoadState::INITIATED) {
                std::cerr << " asset " << assetAndLoadNext.asset->getAssetName() << " tried to start another load??" << std::endl;
                continue;
            }
            assetAndLoadNext.asset->setLoadState(Asset::LoadState::CPU_LOAD_STARTED);
            reportLoadFailures() = assetAndLoadNext.reportFailures;
            loadFailureReason().clear();
            loadingFor = assetAndLoadNext.requester;
            assetAndLoadNext.asset->loadCPUPart();
            loadingFor = nullptr;
            finishCPULoad(assetAndLoadNext, loadFailureReason());
            reportLoadFailures() = false;
            cpuLoadDoneCondition.notify_all();
        }
    }

    void loadPackageManifest();

    //a package converts every raw model, so a raw key is redirected to its .limonmodel. A loose raw file still wins, it is the higher layer
    std::vector<std::string> resolvePackedModel(const std::vector<std::string> &files) const {
        if (files.size() != 1 || packedModelFiles.empty()) {
            return files;
        }
        size_t flipPosition = files[0].rfind("?flip");
        std::string modelFile = files[0].substr(0, flipPosition);
        std::string normalizedKey = LimonFileSystem::normalizePath(modelFile) + (flipPosition == std::string::npos ? "" : files[0].substr(flipPosition));
        std::unordered_map<std::string, std::string>::const_iterator packedModel = packedModelFiles.find(normalizedKey);
        if (packedModel == packedModelFiles.end() || LimonFileSystem::isLooseDataFile(modelFile)) {
            return files;
        }
        return {"./" + packedModel->second};
    }

    template<class T>
    static std::shared_ptr<Asset> createAsset(AssetManager *assetManager, uint32_t assetID, const std::vector<std::string> &files) {
        return std::make_shared<T>(assetManager, assetID, files);
    }

    friend class LoadRequester;
    void closeRequester(LoadRequester &requester, bool fromOwner);
    //sets the state and hands the asset to everyone waiting on it, an empty reason is success
    void finishCPULoad(const QueuedAsset &loaded, const std::string &failureReason);
    bool isRunning(LoadRequester &requester);
    void exitUnlessGPUContextThread(const char *operation) const;
    //callers hold requestersMutex
    void startPendingRequestsInternal(LoadRequester &requester);
    void handOverInternal(LoadRequester &requester, const std::shared_ptr<Asset> &asset, const std::string &failureReason);
    void startNextRequestersInternal();
    void waitForStartedLoadsInternal(LoadRequester &requester, std::unique_lock<std::mutex> &requestersLock);
    void releaseReferencesInternal(const std::vector<LoadRequester::TrackedRequest> &requests);

public:
    explicit AssetManager(GraphicsInterface* graphicsWrapper, ALHelper *alHelper);

    ~AssetManager();

    //what a load step calls instead of exit, then returns. The engine stops on an asset it can't load, only a conversion gets the reason back
    static void failLoad(const std::string &message) {
        if (!reportLoadFailures()) {
            std::cerr << message << " Exiting..." << std::endl;
            exit(-1);
        }
        std::cerr << message << std::endl;
        if (loadFailureReason().empty()) {
            loadFailureReason() = message;
        }
    }

    static bool hasLoadFailed() {
        return !loadFailureReason().empty();
    }

    //reportFailures hands a failed load back with its reason, otherwise the engine stops on it. Close it, or let it go out of scope, once its results are processed
    std::unique_ptr<LoadRequester> openRequester(bool reportFailures, LoadRequester::GPUMode gpuMode, uint32_t maxInFlight);

    static bool isInsideLoad() {
        return loadingFor != nullptr;
    }

    template<class T>
    void requestAsset(LoadRequester &requester, const std::vector<std::string> &requestedFiles) {
        std::lock_guard<std::mutex> lock(requestersMutex);
        if (requester.closed) {
            std::cerr << "Asset " << StringUtils::join(requestedFiles, ",") << " requested from a closed requester, ignored." << std::endl;
            return;
        }
        requester.pending.push_back(LoadRequester::PendingRequest{resolvePackedModel(requestedFiles), &createAsset<T>});
        if (requester.running) {
            startPendingRequestsInternal(requester);
        }
    }

    //a finished load, with the reference the request took: after its CPU part, or DONE for UPLOADED. Waits up to wait for one, false if none came
    bool popFinishedRequest(LoadRequester &requester, LoadRequester::LoadResult &result, std::chrono::milliseconds wait);

    bool hasOutstandingRequests(LoadRequester &requester) {
        std::lock_guard<std::mutex> lock(requestersMutex);
        return !requester.pending.empty() || !requester.inFlight.empty() || !requester.finished.empty();
    }

    //main thread, once per frame: the GPU part of one asset an UPLOADED requester waits for
    void uploadOneQueuedAsset();

    template<class T>
    std::shared_ptr<T>loadAsset(const std::vector<std::string> requestedFiles) {
        if (isInsideLoad()) {
            //this thread loads for a requester, waiting on a requester of its own here would wait on that one
            std::cerr << "Synchronous load of " << StringUtils::join(requestedFiles, ",") << " inside another load, this is a bug. Exiting..." << std::endl;
            exit(-1);
        }
        std::unique_ptr<LoadRequester> requester = openRequester(false, LoadRequester::GPUMode::SELF, 1);
        requestAsset<T>(*requester, requestedFiles);
        if (!isRunning(*requester)) {
            std::cerr << "Loading " << StringUtils::join(requestedFiles, ",") << " while another thread's loads run, it waits for them. Nothing should load now." << std::endl;
        }
        LoadRequester::LoadResult result;
        while (!popFinishedRequest(*requester, result, std::chrono::milliseconds{5})) {
        }
        partialLoadGPUSide(result.asset);
        requester->close();
        return std::dynamic_pointer_cast<T>(result.asset);
    }

    template<class T>
    std::vector<std::shared_ptr<T>>parallelLoadAssetList(const std::vector<std::vector<std::string>> filesList) {
        std::unique_ptr<LoadRequester> requester = openRequester(false, LoadRequester::GPUMode::SELF, 0);
        std::map<std::vector<std::string>, std::shared_ptr<T>> loadedAssets;
        for (const std::vector<std::string> &requestedFiles : filesList) {
            std::vector<std::string> files = resolvePackedModel(requestedFiles);
            if (loadedAssets.count(files) == 0) {
                loadedAssets[files] = nullptr;
                requestAsset<T>(*requester, files);
            }
        }
        //now load the assets to GPU on main thread
        LoadRequester::LoadResult result;
        while (hasOutstandingRequests(*requester)) {
            if (popFinishedRequest(*requester, result, std::chrono::milliseconds{5})) {
                partialLoadGPUSide(result.asset);
                loadedAssets[result.files] = std::dynamic_pointer_cast<T>(result.asset);
            }
        }
        requester->close();
        std::vector<std::shared_ptr<T>> loadedAssetList;
        std::set<std::vector<std::string>> handedOver;
        for (const std::vector<std::string> &requestedFiles : filesList) {
            std::vector<std::string> files = resolvePackedModel(requestedFiles);
            if (!handedOver.insert(files).second) {
                std::lock_guard<std::mutex> lock(assetsMutex);
                assets[files].second++;//every entry of the list holds a reference, the request took only the first
            }
            loadedAssetList.push_back(loadedAssets[files]);
        }
        return loadedAssetList;
    }

    //only inside a load, it joins that load's requester. Anywhere else a load is a synchronous loadAsset
    template<class T>
    std::shared_ptr<T> partialLoadAssetAsync(const std::vector<std::string> requestedFiles) {
    if (!isInsideLoad()) {
        std::cerr << "Asynchronous load of " << StringUtils::join(requestedFiles, ",") << " outside an asset's load has no requester, this is a bug. Exiting..." << std::endl;
        exit(-1);
    }
    const std::vector<std::string> files = resolvePackedModel(requestedFiles);
    //this method can be called by another thread, because some assets create other assets
    // ex: Model assets load Texture assets
    // If model asset was loading on a non main loading thread, this will be called by that thread.
    // Since there are multiple non main loading threads, this means race condition possible, and we need a lock.
    std::unique_lock<std::mutex> lock(assetsMutex);
        if (assets.count(files) == 0) {
            uint32_t nextAssetIndexLocal = getNextAssetIndex();
            assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files), 0);
            assetLoadCpuQueue.pushBack(QueuedAsset{assets[files].first, loadingFor->reportFailures, loadingFor});
        }
        assets[files].second++;
        return std::dynamic_pointer_cast<T>(assets[files].first);
    }

    //no requester, so it never waits behind another thread's loads. null if it isn't fully loaded
    template<class T>
    std::shared_ptr<T> acquireLoadedAsset(const std::vector<std::string> &requestedFiles) {
        const std::vector<std::string> files = resolvePackedModel(requestedFiles);
        std::lock_guard<std::mutex> lock(assetsMutex);
        std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>>::iterator assetIt = assets.find(files);
        if (assetIt == assets.end() || assetIt->second.first->getLoadState() != Asset::LoadState::DONE) {
            return nullptr;
        }
        assetIt->second.second++;
        return std::dynamic_pointer_cast<T>(assetIt->second.first);
    }

    void waitForCPULoad(const std::shared_ptr<Asset> &asset) {
        while(asset->getLoadState() != Asset::LoadState::CPU_LOAD_DONE && asset->getLoadState() != Asset::LoadState::DONE &&
              asset->getLoadState() != Asset::LoadState::FAILED) {
            std::unique_lock<std::mutex> lock(cpuLoadConditionMutex);
            cpuLoadDoneCondition.wait_for(lock, std::chrono::milliseconds{5});
        }
    }

    void partialLoadGPUSide(std::shared_ptr<Asset> asset) {
        exitUnlessGPUContextThread("GPU part");
        if(asset->getLoadState() == Asset::LoadState::DONE) {
            //Some code paths will try to load the asset again.
            return;
        }
        waitForCPULoad(asset);
        if(asset->getLoadState() == Asset::LoadState::FAILED) {
            exitUnlessReporting(asset);
            return;
        }
        if(asset->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
            asset->loadGPUPart();
            asset->setLoadState(Asset::LoadState::DONE);
        }
        return;
    }

    void freeAsset(const std::vector<std::string> requestedFiles) {
        const std::vector<std::string> files = resolvePackedModel(requestedFiles);
        if(files.size() == 0) {
            std::cerr << "Free asset call with empty file list, this is invalid!" << std::endl;
            return;
        }
        std::lock_guard<std::mutex> lock(assetsMutex);
        if (assets.count(files) == 0) {
            std::cerr << "Unloading an asset [";
            for (uint32_t i = 0; i < files.size() -1; ++i) {
                std::cerr << files[i] << ", ";
            }

            std::cerr << files[files.size()-1] << "] that was not loaded. skipping." << std::endl;
            return;
        }
        assets[files].second--;
        if (assets[files].second == 0) {
            //last element that requested the load freed, remove our own reference to it, so it gets freed.
            if (assets[files].first.use_count() > 2) {
                //possible issue
                std::cerr << "Reference counter for asset " << files[0] << " is more than 2, there is a leak" << std::endl;
            }
            //CPU_LOAD_DONE is a finished load for a requester that skips GPU parts
            if(assets[files].first->getLoadState() == Asset::LoadState::INITIATED || assets[files].first->getLoadState() == Asset::LoadState::CPU_LOAD_STARTED) {
                std::cerr << "trying to delete a partially loaded object" + files[0] + ", probably a bug" << std::endl;
            }
            //std::cerr << "deleting asset as all references are removed" << std::endl;
            assets.erase(files);
            if (files.size() == 1) {
                //flip variants share the images of their file, freeing either one drops them
                std::lock_guard<std::mutex> embeddedLock(embeddedTexturesMutex);
                embeddedTextures.erase(files[0].substr(0, files[0].rfind("?flip")));
            }
        }
    }

    //the loaded models a save would write, packing refuses while there are any
    std::vector<std::string> getModelsWithUnsavedChanges();

    bool isLoaded(std::vector<std::string> filename) {
        filename = resolvePackedModel(filename);
        std::lock_guard<std::mutex> lock(assetsMutex);
        return this->assets.find(filename) != this->assets.end();
    }

    void loadUsingCereal(const std::vector<std::string> files [[gnu::unused]]);

    //a raw model a package carries only as its .limonmodel still loads, so it exists for whoever asks
    bool isModelAvailable(const std::string &modelKey) const {
        std::vector<std::string> resolvedFiles = resolvePackedModel({modelKey});
        return LimonFileSystem::fileExists(resolvedFiles[0].substr(0, resolvedFiles[0].rfind("?flip")));
    }

    //the raw model keys a mounted package converted into this .limonmodel, empty if it is not one of them
    std::vector<std::string> getPackedModelKeys(const std::string &packedModelFile) const {
        std::string normalizedFile = LimonFileSystem::normalizePath(packedModelFile);
        std::vector<std::string> modelKeys;
        for (const std::pair<const std::string, std::string> &packedModel : packedModelFiles) {
            if (packedModel.second == normalizedFile) {
                modelKeys.push_back(packedModel.first);
            }
        }
        return modelKeys;
    }

    //keyed by the file the images came from, a later add for the same file keeps what is there. freeAsset drops the entry
    void addEmbeddedTextures(const std::string& ownerAsset, std::vector<std::shared_ptr<const EmbeddedTexture>> textures) {
        std::lock_guard<std::mutex> lock(embeddedTexturesMutex);
        embeddedTextures.emplace(ownerAsset, textures);
    }

    std::shared_ptr<const EmbeddedTexture> getEmbeddedTextures(const std::string& ownerAsset, uint32_t textureID) {
        std::lock_guard<std::mutex> lock(embeddedTexturesMutex);
        std::unordered_map<std::string, std::vector<std::shared_ptr<const EmbeddedTexture>>>::const_iterator textureSet = embeddedTextures.find(ownerAsset);
        if (textureSet == embeddedTextures.end() || textureSet->second.size() <= textureID) {
            return nullptr;
        }
        return textureSet->second[textureID];
    }

    //the asset files on disk, for pickers and lookups by name
    AssetTreeManager *getTree() const {
        return assetTree.get();
    }

    MaterialRegistry& getMaterialRegistry() {
        return materialRegistry;
    }

    const MaterialRegistry& getMaterialRegistry() const {
        return materialRegistry;
    }

    GraphicsInterface* getGraphicsWrapper() const {
        return graphicsWrapper;
    }

    ALHelper *getAlHelper() const {
        return alHelper;
    }
};


#endif //LIMONENGINE_ASSETMANAGER_H
