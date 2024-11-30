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
#include "../ALHelper.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


class GraphicsInterface;
class ALHelper;

class AssetManager {
public:
    enum AssetTypes { Asset_type_DIRECTORY, Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP, Asset_type_SOUND, Asset_type_GRAPHICSPROGRAM, Asset_type_UNKNOWN };

    std::mutex cpuLoadConditionMutex;
    std::mutex partialLoadCpuMutex;
    std::condition_variable cpuLoadDoneCondition;

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
    class AssetLoadQueue {
    private:
        std::list<std::pair<std::shared_ptr<Asset>, bool>> assets;
        std::mutex queueMutex;
        std::condition_variable queueEmptyCond;
    public:
        void pushBack(const std::shared_ptr<Asset>& asset, bool pushToNextQueue) {
            std::lock_guard<std::mutex> lock(queueMutex);
            assets.emplace_back(asset, pushToNextQueue);
            queueEmptyCond.notify_one();
        }

        void pushBack(const std::pair<std::shared_ptr<Asset>, bool>& assetAndPushToNextQueue) {
            std::lock_guard<std::mutex> lock(queueMutex);
            assets.push_back(assetAndPushToNextQueue);
            queueEmptyCond.notify_one();
        }

        std::pair<std::shared_ptr<Asset>, bool> popFrontOrBlock() {
            std::unique_lock<std::mutex> lock(queueMutex);
            while (assets.empty()) {
                queueEmptyCond.wait(lock);
            }
            std::pair<std::shared_ptr<Asset>, bool> assetPtr =assets.front();
            assets.pop_front();
            return assetPtr;
        }

        std::pair<std::shared_ptr<Asset>, bool> popFrontOrBlockFor(const std::chrono::milliseconds milliseconds) {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueEmptyCond.wait_for(lock, milliseconds);
            if (assets.empty()) {
                return {nullptr, false};
            }
            std::pair<std::shared_ptr<Asset>, bool> assetPtr =assets.front();
            assets.pop_front();
            return assetPtr;
        }

        std::pair<std::shared_ptr<Asset>, bool> popFrontOrReturn() {
            std::unique_lock<std::mutex> lock(queueMutex);
            if(assets.empty()) {
                return {nullptr, false};
            }
            std::pair<std::shared_ptr<Asset>, bool> assetPtr =assets.front();
            assets.pop_front();
            return assetPtr;
        }

        bool isEmpty() {
            std::unique_lock<std::mutex> lock(queueMutex);
            return assets.empty();
        }
    };

    AssetLoadQueue assetLoadCpuQueue;
    AssetLoadQueue assetLoadGPUQueue;

    struct AssetLoadThreadState {
        std::shared_ptr<std::thread> thread;
        bool running = true;
    };
    std::vector<AssetLoadThreadState*> assetLoadThreads;

    void cpuLoadAsset(bool& running) {
        while(running) {
            std::pair<std::shared_ptr<Asset>, bool> assetAndLoadNext = assetLoadCpuQueue.popFrontOrBlockFor(std::chrono::milliseconds(5));
            if (assetAndLoadNext.first != nullptr) {
                if(assetAndLoadNext.first->getLoadState() != Asset::LoadState::INITIATED) {
                    std::cerr << " asset " << assetAndLoadNext.first->getName() << " tried to start another load??" << std::endl;
                    continue;
                }
                assetAndLoadNext.first->setLoadState(Asset::LoadState::CPU_LOAD_STARTED);
                assetAndLoadNext.first->loadCPUPart();
                assetAndLoadNext.first->setLoadState(Asset::LoadState::CPU_LOAD_DONE);
                if(assetAndLoadNext.second){
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

    std::map<size_t, std::pair<std::shared_ptr<Material>, uint32_t>> materials;//this is used to make objects share materials.

    //std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    AvailableAssetsNode* availableAssetsRootNode = nullptr;
    std::map<std::pair<AssetTypes, std::string>, AvailableAssetsNode*> filteredResults;
    GraphicsInterface* graphicsWrapper;
    ALHelper *alHelper;

    uint32_t getNextAssetIndex() {
        return ++nextAssetIndex;
    }

    void addAssetsRecursively(const std::string &directoryPath, const std::string &fileName,
                                  const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions,
                                  AvailableAssetsNode* nodeToProcess);

    std::vector<std::pair<std::string, AssetTypes>> loadAssetExtensionList();

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

    explicit AssetManager(GraphicsInterface* graphicsWrapper, ALHelper *alHelper) : graphicsWrapper(graphicsWrapper), alHelper(alHelper) {
        loadAssetList();
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
        std::unordered_set<int> startedAssetIds;
        for(const auto &files : filesList) {
            if (assets.count(files) == 0) {
                uint32_t nextAssetIndexLocal = getNextAssetIndex();
                bool loaded = false;
                //check if asset is cereal deserialize file.
                if(files.size() == 1) {
                    std::string extension = files[0].substr(files[0].find_last_of(".") + 1);
                    if (extension == "limonmodel") {
#ifdef CEREAL_SUPPORT
                        std::ifstream is(files[0], std::ios::binary);
                        cereal::BinaryInputArchive archive(is);
                        assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files, archive), 0);
#else
                        std::cerr << "Limon compiled without limonmodel support. Please acquire a release version. Exiting..." << std::endl;
                    std::cerr << "Compile should define \"CEREAL_SUPPORT\"." << std::endl;
                    exit(-1);
#endif
                        loaded = true;
                    }
                }
                if(!loaded) {
                    assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files), 0);
                    startedAssetIds.insert(nextAssetIndexLocal);
                    assetLoadCpuQueue.pushBack(assets[files].first, true);
                }
            }
            assets[files].second++;
            loadedAssets.emplace_back(std::dynamic_pointer_cast<T>(assets[files].first));
        }
        //now load the assets to GPU on main thread
        while(!startedAssetIds.empty()) {
            std::pair<std::shared_ptr<Asset>, bool> assetAndPushToNext = assetLoadGPUQueue.popFrontOrReturn();
            if(assetAndPushToNext.first != nullptr) {
                if(startedAssetIds.find(assetAndPushToNext.first->getAssetID()) == startedAssetIds.end()) {
                    continue;//we didn't start this, we should not finish it
                }
                if(assetAndPushToNext.first->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
                    assetAndPushToNext.first->loadGPUPart();
                    assetAndPushToNext.first->setLoadState(Asset::LoadState::DONE);
                }
                if(assetAndPushToNext.first->getLoadState() == Asset::LoadState::DONE) {
                    startedAssetIds.erase(assetAndPushToNext.first->getAssetID());
                }
            } else {
                std::unique_lock<std::mutex> lock(cpuLoadConditionMutex);
                cpuLoadDoneCondition.wait_for(lock, std::chrono::milliseconds{5});
            }
        }
        return loadedAssets;
    }

    template<class T>
    std::shared_ptr<T> partialLoadAssetAsync(const std::vector<std::string> files) {
    //this method can be called by another thread, because some assets create other assets
    // ex: Model assets load Texture assets
    // If model asset was loading on a non main loading thread, this will be called by that thread.
    // Since there are multiple non main loading threads, this means race condition possible, and we need a lock.
    std::unique_lock<std::mutex> lock(partialLoadCpuMutex);
        if (assets.count(files) == 0) {
            uint32_t nextAssetIndexLocal = getNextAssetIndex();
            bool loaded = false;
            //check if asset is cereal deserialize file.
            if(files.size() == 1) {
                std::string extension = files[0].substr(files[0].find_last_of(".") + 1);
                if (extension == "limonmodel") {
#ifdef CEREAL_SUPPORT
                    std::ifstream is(files[0], std::ios::binary);
                    cereal::BinaryInputArchive archive(is);
                    assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files, archive), 0);
#else
                    std::cerr << "Limon compiled without limonmodel support. Please acquire a release version. Exiting..." << std::endl;
                std::cerr << "Compile should define \"CEREAL_SUPPORT\"." << std::endl;
                exit(-1);
#endif
                    loaded = true;
                }
            }
            if(!loaded) {
                assets[files] = std::make_pair(std::make_shared<T>(this, nextAssetIndexLocal, files), 0);
                assetLoadCpuQueue.pushBack(assets[files].first, false);
            }
        }
        assets[files].second++;
        return std::dynamic_pointer_cast<T>(assets[files].first);
    }

    void partialLoadGPUSide(std::shared_ptr<Asset> asset) {
        if(asset->getLoadState() == Asset::LoadState::DONE) {
            //Some code paths will try to load the asset again.
            return;
        }
        while(asset->getLoadState() != Asset::LoadState::CPU_LOAD_DONE && asset->getLoadState() != Asset::LoadState::DONE) {
            std::unique_lock<std::mutex> lock(cpuLoadConditionMutex);
            cpuLoadDoneCondition.wait_for(lock, std::chrono::milliseconds{5});
        }
        if(asset->getLoadState() == Asset::LoadState::CPU_LOAD_DONE) {
            asset->loadGPUPart();
            asset->setLoadState(Asset::LoadState::DONE);
        }
        return;
    }

    template<class T>
    std::shared_ptr<T>loadAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {
            bool loaded = false;
            //check if asset is cereal deserialize file.
            if(files.size() == 1) {
                std::string extension = files[0].substr(files[0].find_last_of(".") + 1);
                if (extension == "limonmodel") {
#ifdef CEREAL_SUPPORT
                    std::ifstream is(files[0], std::ios::binary);
                    cereal::BinaryInputArchive archive(is);
                    assets[files] = std::make_pair(std::make_shared<T>(this, getNextAssetIndex(), files, archive), 0);
#else
                    std::cerr << "Limon compiled without limonmodel support. Please acquire a release version. Exiting..." << std::endl;
                    std::cerr << "Compile should define \"CEREAL_SUPPORT\"." << std::endl;
                    exit(-1);
#endif
                    loaded = true;
                }
            }
            if(!loaded) {
                assets[files] = std::make_pair(std::make_shared<T>(this, getNextAssetIndex(), files), 0);
                assets[files].first->load();
            }
        }
        if(assets[files].first->getLoadState() != Asset::LoadState::DONE) {
            //some other thread is working on this, we should block.
            while (assets[files].first->getLoadState() != Asset::LoadState::DONE) {
                std::cerr << "Partial load and full load clashing, please fix. Will busy wait" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
        }
        assets[files].second++;
        return std::dynamic_pointer_cast<T>(assets[files].first);
    }

    void freeAsset(const std::vector<std::string> files) {
        if(files.size() == 0) {
            std::cerr << "Free asset call with empty file list, this is invalid!" << std::endl;
            return;
        }
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

    bool isLoaded(std::vector<std::string> filename) {
        return this->assets.find(filename) != this->assets.end();
    }

    const AvailableAssetsNode* getAvailableAssetsTree() {
        return availableAssetsRootNode;
    };

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

    std::shared_ptr<Material> registerMaterial(std::shared_ptr<Material> material);
    void unregisterMaterial(std::shared_ptr<Material> material);

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

        delete availableAssetsRootNode;

        for (auto tree_iterator = filteredResults.begin(); tree_iterator != filteredResults.end(); ++tree_iterator) {
            delete tree_iterator->second;
        }

    }
};


#endif //LIMONENGINE_ASSETMANAGER_H
