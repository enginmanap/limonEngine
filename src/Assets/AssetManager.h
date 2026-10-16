//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSETMANAGER_H
#define LIMONENGINE_ASSETMANAGER_H


#include <string>
#include <map>
#include <utility>
#include <tinyxml2.h>
#include <fstream>
#ifdef CEREAL_SUPPORT
#include <cereal/archives/xml.hpp>
#include <Cereal/include/cereal/archives/binary.hpp>
#endif

#include "Asset.h"
#include "MaterialRegistry.h"
#include "../LimonFileSystem.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

class GraphicsInterface;
class ALHelper;
class Material;
class World;//only ever used as an opaque owner token for material override rules

class AssetManager {
    std::mutex cpuLoadConditionMutex;
    std::mutex assetsMutex; //guards the `assets` map only; must be held for every insert/find/erase/refcount change on it, but released before any actual loading work
    std::mutex availableAssetsMutex; //guards `availableAssetsRootNode` and `filteredResults`: getAvailableAssetsTreeFiltered() is called
                                      // from background texture-loading threads (TextureAsset's not-found fallback search), while
                                      // applyPendingAssetListReload() swaps/deletes these from the main thread.
    std::condition_variable cpuLoadDoneCondition;

public:
    enum AssetTypes { Asset_type_DIRECTORY, Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP, Asset_type_SOUND, Asset_type_GRAPHICSPROGRAM, Asset_type_UNKNOWN };

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

    struct AvailableAssetsNode {
        std::string name;
        std::string nameLower;
        std::string fullPath;
        AssetTypes assetType = Asset_type_UNKNOWN;
        AvailableAssetsNode* parent = nullptr;
        std::vector<AvailableAssetsNode*> children;

        ~AvailableAssetsNode() {
            for (size_t i = 0; i < children.size(); ++i) {
                delete children[i];
            }
        }

        const AvailableAssetsNode* findNode(const std::string& requestedPath) const {
            if(this->fullPath == requestedPath) {
                return this;
            }
            for (auto child = children.begin(); child != children.end(); ++child) {
                const AvailableAssetsNode* result = (*child)->findNode(requestedPath);
                if(result != nullptr) {
                    return result;
                }
            }
            return nullptr;
        }
    };
private:
    //set by loadAssetForConversion for its own thread, and by a loader thread for the request it is loading.
    //function locals, a thread_local std::string member gets an init routine in every translation unit and mingw refuses the duplicates
    static bool &reportLoadFailures() {
        thread_local bool report = false;
        return report;
    }

    static std::string &loadFailureReason() {
        thread_local std::string reason;
        return reason;
    }

    struct QueuedAsset {
        std::shared_ptr<Asset> asset;
        bool pushToNextQueue;
        bool reportFailures;//the requester's mode, the loader thread takes it over
    };

    class AssetLoadQueue {
    private:
        std::list<QueuedAsset> assets;
        std::mutex queueMutex;
        std::condition_variable queueEmptyCond;
    public:
        void pushBack(const std::shared_ptr<Asset>& asset, bool pushToNextQueue) {
            std::lock_guard<std::mutex> lock(queueMutex);
            assets.push_back(QueuedAsset{asset, pushToNextQueue, reportLoadFailures()});
            queueEmptyCond.notify_one();
        }

        void pushBack(const QueuedAsset &queuedAsset) {
            std::lock_guard<std::mutex> lock(queueMutex);
            assets.push_back(queuedAsset);
            queueEmptyCond.notify_one();
        }

        QueuedAsset popFrontOrBlock() {
            std::unique_lock<std::mutex> lock(queueMutex);
            while (assets.empty()) {
                queueEmptyCond.wait(lock);
            }
            QueuedAsset assetPtr = assets.front();
            assets.pop_front();
            return assetPtr;
        }

        QueuedAsset popFrontOrBlockFor(const std::chrono::milliseconds milliseconds) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueEmptyCond.wait_for(lock, milliseconds);
            if (assets.empty()) {
                return QueuedAsset{nullptr, false, false};
            }
            QueuedAsset assetPtr = assets.front();
            assets.pop_front();
            return assetPtr;
        }

        QueuedAsset popFrontOrReturn() {
            std::unique_lock<std::mutex> lock(queueMutex);
            if(assets.empty()) {
                return QueuedAsset{nullptr, false, false};
            }
            QueuedAsset assetPtr = assets.front();
            assets.pop_front();
            return assetPtr;
        }

        bool isEmpty() {
            std::unique_lock<std::mutex> lock(queueMutex);
            return assets.empty();
        }
    };

    //model key to the .limonmodel a package carries instead of the raw model, both normalized. Filled once before any load
    std::unordered_map<std::string, std::string> packedModelFiles;

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

    AssetLoadQueue assetLoadCpuQueue;
    AssetLoadQueue assetLoadGPUQueue;

    struct AssetLoadThreadState {
        std::shared_ptr<std::thread> thread;
        bool running = true;
    };
    std::vector<AssetLoadThreadState*> assetLoadThreads;

    void cpuLoadAsset(bool& running) {
        while(running) {
            QueuedAsset assetAndLoadNext = assetLoadCpuQueue.popFrontOrBlockFor(std::chrono::milliseconds(5));
            if (assetAndLoadNext.asset != nullptr) {
                if(assetAndLoadNext.asset->getLoadState() != Asset::LoadState::INITIATED) {
                    std::cerr << " asset " << assetAndLoadNext.asset->getAssetName() << " tried to start another load??" << std::endl;
                    continue;
                }
                assetAndLoadNext.asset->setLoadState(Asset::LoadState::CPU_LOAD_STARTED);
                reportLoadFailures() = assetAndLoadNext.reportFailures;
                loadFailureReason().clear();
                assetAndLoadNext.asset->loadCPUPart();
                assetAndLoadNext.asset->setLoadState(hasLoadFailed() ? Asset::LoadState::FAILED : Asset::LoadState::CPU_LOAD_DONE);
                reportLoadFailures() = false;
                if(assetAndLoadNext.pushToNextQueue){//a failed one too, whoever waits on the GPU queue must see it end
                    assetLoadGPUQueue.pushBack(assetAndLoadNext);
                }
                cpuLoadDoneCondition.notify_all();
            }
        }
    }

    const std::string ASSET_EXTENSIONS_FILE = "./Engine/assetExtensions.xml";

    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>> assets;
    std::unordered_map<std::string, std::vector<std::shared_ptr<const EmbeddedTexture>>> embeddedTextures;
    std::atomic<std::int32_t> nextAssetIndex;


    //std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    AvailableAssetsNode* availableAssetsRootNode = nullptr;
    std::map<std::pair<AssetTypes, std::string>, AvailableAssetsNode*> filteredResults;

    std::atomic<bool> assetListReloadInProgress{false};
    AvailableAssetsNode* pendingAssetsRootNode = nullptr;
    std::thread assetReloadThread;
    uint32_t assetListVersion = 0;
    //its own object because it shares no state with asset caching, neither touches the other's members
    MaterialRegistry materialRegistry;

    GraphicsInterface* graphicsWrapper;
    ALHelper *alHelper;

    uint32_t getNextAssetIndex() {
        return ++nextAssetIndex;
    }

    void addAssetsRecursively(const std::string &directoryPath, const std::string &fileName,
                                  const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions,
                                  AvailableAssetsNode* nodeToProcess);

    std::vector<std::pair<std::string, AssetTypes>> loadAssetExtensionList();

    //only a request made in report mode can leave an asset FAILED, a game caller that needed it stops the engine
    static void exitUnlessReporting(const std::shared_ptr<Asset> &failedAsset) {
        if (!reportLoadFailures()) {
            std::cerr << failedAsset->getAssetName() << " could not be loaded, the reason is above. Exiting..." << std::endl;
            exit(-1);
        }
    }


    bool loadAssetList();

    bool isExtensionInList(const std::string &name, const std::vector<std::pair<std::string, AssetTypes>> &vector,
                               AssetTypes &elementAssetType);

    static bool isEnding(std::string const &fullString, std::string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    AvailableAssetsNode * getAvailableAssetsTreeFilteredRecursive(const AvailableAssetsNode * const assetsNode ,
                                                                  AssetTypes type,
                                                                  const std::string &filterText);

public:
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

    explicit AssetManager(GraphicsInterface* graphicsWrapper, ALHelper *alHelper) : materialRegistry(graphicsWrapper), graphicsWrapper(graphicsWrapper), alHelper(alHelper) {
        nextAssetIndex.store(0);
        loadAssetList();
        loadPackageManifest();
        size_t num_threads = std::thread::hardware_concurrency();
        for (size_t i = 0; i < num_threads; ++i) {
            AssetLoadThreadState* state = new AssetLoadThreadState();
            state->running = true;
            state->thread = std::make_shared<std::thread>(&AssetManager::cpuLoadAsset, this, std::ref(state->running));
            assetLoadThreads.push_back(state);
        }
    }

    void loadUsingCereal(const std::vector<std::string> files [[gnu::unused]]);

    template<class T>
    std::vector<std::shared_ptr<T>>parallelLoadAssetList(const std::vector<std::vector<std::string>> filesList) {
        std::vector<std::shared_ptr<T>> loadedAssets;
        std::unordered_set<uint32_t> startedAssetIds;
        for(const std::vector<std::string> &requestedFiles : filesList) {
            const std::vector<std::string> files = resolvePackedModel(requestedFiles);
            std::lock_guard<std::mutex> lock(assetsMutex);
            if (assets.count(files) == 0) {
                uint32_t nextAssetIndexLocal = getNextAssetIndex();
                assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files), 0);
                startedAssetIds.insert(nextAssetIndexLocal);
                assetLoadCpuQueue.pushBack(assets[files].first, true);
            }
            assets[files].second++;
            loadedAssets.emplace_back(std::dynamic_pointer_cast<T>(assets[files].first));
        }
        //now load the assets to GPU on main thread
        while(!startedAssetIds.empty()) {
            QueuedAsset assetAndPushToNext = assetLoadGPUQueue.popFrontOrReturn();
            if(assetAndPushToNext.asset != nullptr) {
                if(startedAssetIds.find(assetAndPushToNext.asset->getAssetID()) == startedAssetIds.end()) {
                    continue;//we didn't start this, we should not finish it
                }
                if(assetAndPushToNext.asset->getLoadState() == Asset::LoadState::FAILED) {
                    exitUnlessReporting(assetAndPushToNext.asset);
                    startedAssetIds.erase(assetAndPushToNext.asset->getAssetID());
                    continue;
                }
                if(assetAndPushToNext.asset->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
                    assetAndPushToNext.asset->loadGPUPart();
                    assetAndPushToNext.asset->setLoadState(Asset::LoadState::DONE);
                }
                if(assetAndPushToNext.asset->getLoadState() == Asset::LoadState::DONE) {
                    startedAssetIds.erase(assetAndPushToNext.asset->getAssetID());
                }
            } else {
                std::unique_lock<std::mutex> lock(cpuLoadConditionMutex);
                cpuLoadDoneCondition.wait_for(lock, std::chrono::milliseconds{5});
            }
        }
        return loadedAssets;
    }

    template<class T>
    std::shared_ptr<T> partialLoadAssetAsync(const std::vector<std::string> requestedFiles) {
    const std::vector<std::string> files = resolvePackedModel(requestedFiles);
    //this method can be called by another thread, because some assets create other assets
    // ex: Model assets load Texture assets
    // If model asset was loading on a non main loading thread, this will be called by that thread.
    // Since there are multiple non main loading threads, this means race condition possible, and we need a lock.
    std::unique_lock<std::mutex> lock(assetsMutex);
        if (assets.count(files) == 0) {
            uint32_t nextAssetIndexLocal = getNextAssetIndex();
            assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files), 0);
            assetLoadCpuQueue.pushBack(assets[files].first, false);
        }
        assets[files].second++;
        return std::dynamic_pointer_cast<T>(assets[files].first);
    }

    void partialLoadGPUSide(std::shared_ptr<Asset> asset) {
        if(asset->getLoadState() == Asset::LoadState::DONE) {
            //Some code paths will try to load the asset again.
            return;
        }
        while(asset->getLoadState() != Asset::LoadState::CPU_LOAD_DONE && asset->getLoadState() != Asset::LoadState::DONE &&
              asset->getLoadState() != Asset::LoadState::FAILED) {
            std::unique_lock<std::mutex> lock(cpuLoadConditionMutex);
            cpuLoadDoneCondition.wait_for(lock, std::chrono::milliseconds{5});
        }
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

    template<class T>
    std::shared_ptr<T>loadAsset(const std::vector<std::string> requestedFiles) {
        const std::vector<std::string> files = resolvePackedModel(requestedFiles);
        std::shared_ptr<Asset> assetPtr;
        bool needsSynchronousLoad = false;
        {
            //reservation: the existence check and the placeholder insert must happen in the same
            // critical section, so no two threads can ever both decide to load the same asset.
            std::lock_guard<std::mutex> lock(assetsMutex);
            if (assets.count(files) == 0) {
                assets[files] = std::make_pair(std::make_shared<T>(this, getNextAssetIndex(), files), 0);
                needsSynchronousLoad = true;
            }
            assetPtr = assets[files].first;
        }
        //everything below runs without assetsMutex held, so unrelated assets stay fully parallel.
        if(needsSynchronousLoad) {
            assetPtr->loadCPUPart();
            if (hasLoadFailed()) {
                assetPtr->setLoadState(Asset::LoadState::FAILED);
            } else {
                assetPtr->loadGPUPart();
                assetPtr->setLoadState(Asset::LoadState::DONE);
            }
        }
        while (assetPtr->getLoadState() != Asset::LoadState::DONE && assetPtr->getLoadState() != Asset::LoadState::FAILED) {
            //some other thread is working on this, we should block.
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        if (assetPtr->getLoadState() == Asset::LoadState::FAILED) {
            exitUnlessReporting(assetPtr);
            return std::dynamic_pointer_cast<T>(assetPtr);//only loadAssetForConversion gets here
        }
        {
            std::lock_guard<std::mutex> lock(assetsMutex);
            assets[files].second++;
        }
        return std::dynamic_pointer_cast<T>(assetPtr);
    }

    //for a converter, like the packer: a file that can't be loaded comes back as nullptr and a reason, instead of stopping the engine
    template<class T>
    std::shared_ptr<T> loadAssetForConversion(const std::vector<std::string> &files, std::string &failureReason) {
        reportLoadFailures() = true;
        loadFailureReason().clear();
        std::shared_ptr<T> loadedAsset = loadAsset<T>(files);
        reportLoadFailures() = false;
        if (loadedAsset->getLoadState() != Asset::LoadState::FAILED) {
            return loadedAsset;
        }
        failureReason = loadFailureReason();
        std::lock_guard<std::mutex> lock(assetsMutex);
        std::map<const std::vector<std::string>, std::pair<std::shared_ptr<Asset>, uint32_t>>::iterator failedEntry = assets.find(resolvePackedModel(files));
        if (failedEntry != assets.end() && failedEntry->second.first == loadedAsset) {
            assets.erase(failedEntry);//left in, the next request would get the failed asset back instead of trying again
        }
        return nullptr;
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
            //before here, do we know it was actually loaded?
            if(assets[files].first->getLoadState() != Asset::LoadState::DONE) {
                std::cerr << "trying to delete a partially loaded object" + files[0] + ", probably a bug" << std::endl;
            }
            //std::cerr << "deleting asset as all references are removed" << std::endl;
            assets.erase(files);
            if(embeddedTextures.find(files[0]) != embeddedTextures.end()) {
                embeddedTextures.erase(files[0]);
            }
        }
    }

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

    std::vector<std::pair<std::string, AssetTypes>> getAssetExtensions() {
        return loadAssetExtensionList();
    }

    bool isLoaded(std::vector<std::string> filename) {
        filename = resolvePackedModel(filename);
        std::lock_guard<std::mutex> lock(assetsMutex);
        return this->assets.find(filename) != this->assets.end();
    }

    const AvailableAssetsNode* getAvailableAssetsTree() {
        std::lock_guard<std::mutex> lock(availableAssetsMutex);
        return availableAssetsRootNode;
    };

    // Since we are locking the asset tree, we should return from getting the nodes as fast as possible
    // To do that, we can't just parse all mixamo assets inline.
    // This helper allows getting the list so we don't need to hold the lock while looking for the files themselves
    std::vector<std::string> getMixamoAnimationFilePaths(const std::string &modelAssetPath) {
        std::vector<std::string> mixamoFilePaths;
        std::lock_guard<std::mutex> lock(availableAssetsMutex);
        const AvailableAssetsNode* thisAssetsNode = availableAssetsRootNode->findNode(modelAssetPath);
        if (thisAssetsNode == nullptr) {
            std::cerr << "The asset " << modelAssetPath << " is not in asset tree, skipping Mixamo search" << std::endl;
            return mixamoFilePaths;
        }
        if (thisAssetsNode->parent == nullptr) {
            return mixamoFilePaths;
        }
        const AvailableAssetsNode* parentNode = thisAssetsNode->parent;
        for (size_t i = 0; i < parentNode->children.size(); ++i) {
            const AvailableAssetsNode* sibling = parentNode->children[i];
            if (sibling->name == "Mixamo") {
                for (size_t j = 0; j < sibling->children.size(); ++j) {
                    const AvailableAssetsNode* mixamoFile = sibling->children[j];
                    if (mixamoFile->assetType == AssetTypes::Asset_type_MODEL) {
                        mixamoFilePaths.push_back(mixamoFile->fullPath);
                    }
                }
            }
        }
        return mixamoFilePaths;
    }

    void beginReloadAssetList() {
        if (assetListReloadInProgress.load()) {
            return;
        }
        assetListReloadInProgress.store(true);
        if (assetReloadThread.joinable()) {
            assetReloadThread.join();
        }
        assetReloadThread = std::thread([this]() {
            std::vector<std::pair<std::string, AssetTypes>> fileExtensions = loadAssetExtensionList();
            AvailableAssetsNode* newRoot = new AvailableAssetsNode();
            newRoot->assetType = Asset_type_DIRECTORY;
            newRoot->fullPath = ".";
            newRoot->nameLower = "";
            newRoot->name = "Assets";
            AvailableAssetsNode* data = new AvailableAssetsNode();
            addAssetsRecursively("./Data", "Data", fileExtensions, data);
            newRoot->children.push_back(data);
            AvailableAssetsNode* engine = new AvailableAssetsNode();
            addAssetsRecursively("./Engine", "Engine", fileExtensions, engine);
            newRoot->children.push_back(engine);
            pendingAssetsRootNode = newRoot;
            assetListReloadInProgress.store(false);
        });
    }

    // Call once per frame from the main thread. Returns true when the new tree is swapped in.
    bool applyPendingAssetListReload() {
        if (assetListReloadInProgress.load()) {
            return false;
        }
        if (pendingAssetsRootNode == nullptr) {
            return false;
        }
        {
            std::lock_guard<std::mutex> lock(availableAssetsMutex);
            for (auto treeIterator = filteredResults.begin(); treeIterator != filteredResults.end(); ++treeIterator) {
                delete treeIterator->second;
            }
            filteredResults.clear();
            delete availableAssetsRootNode;
            availableAssetsRootNode = pendingAssetsRootNode;
        }
        pendingAssetsRootNode = nullptr;
        assetListVersion++;
        return true;
    }

    bool isReloadingAssetList() const {
        return assetListReloadInProgress.load();
    }

    uint32_t getAssetListVersion() const {
        return assetListVersion;
    }

    const AvailableAssetsNode* getAvailableAssetsTreeFiltered(AssetTypes type, const std::string &filterText);


    void addEmbeddedTextures(const std::string& ownerAsset, std::vector<std::shared_ptr<const EmbeddedTexture>> textures) {
        this->embeddedTextures[ownerAsset] = textures;
    }

    std::shared_ptr<const EmbeddedTexture> getEmbeddedTextures(const std::string& ownerAsset, uint32_t textureID) {
        if(embeddedTextures.find(ownerAsset) == embeddedTextures.end()) {
            return nullptr;
        }
        if(embeddedTextures[ownerAsset].size() <= textureID) {
            return nullptr;
        }

        return embeddedTextures[ownerAsset][textureID];

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

    ~AssetManager() {
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

        if (assetReloadThread.joinable()) {
            assetReloadThread.join();
        }
        delete pendingAssetsRootNode;
        delete availableAssetsRootNode;

        for (auto tree_iterator = filteredResults.begin(); tree_iterator != filteredResults.end(); ++tree_iterator) {
            delete tree_iterator->second;
        }

    }
};


#endif //LIMONENGINE_ASSETMANAGER_H
