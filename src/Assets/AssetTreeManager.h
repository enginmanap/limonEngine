//
// Created by engin on 18.10.2026.
//

#ifndef LIMONENGINE_ASSETTREEMANAGER_H
#define LIMONENGINE_ASSETTREEMANAGER_H

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

//the asset files under Data/ and Engine/ as a tree, for the editor's pickers and lookups by name. Only AssetManager creates
//one, everyone else reaches it through AssetManager::getTree()
class AssetTreeManager {
public:
    enum AssetTypes { Asset_type_DIRECTORY, Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP, Asset_type_SOUND, Asset_type_GRAPHICSPROGRAM, Asset_type_UNKNOWN };

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
    friend class AssetManager;

    const std::string ASSET_EXTENSIONS_FILE = "./Engine/assetExtensions.xml";

    std::mutex availableAssetsMutex; //guards `availableAssetsRootNode` and `filteredResults`: getAvailableAssetsTreeFiltered() is called
                                      // from background texture-loading threads (TextureAsset's not-found fallback search), while
                                      // applyPendingAssetListReload() swaps/deletes these from the main thread.
    //std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    AvailableAssetsNode* availableAssetsRootNode = nullptr;
    std::map<std::pair<AssetTypes, std::string>, AvailableAssetsNode*> filteredResults;
    std::atomic<bool> assetListReloadInProgress{false};
    AvailableAssetsNode* pendingAssetsRootNode = nullptr;
    std::thread assetReloadThread;
    uint32_t assetListVersion = 0;

    AssetTreeManager();

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

    static std::string findFirstAssetPathByNameRecursive(const AvailableAssetsNode *assetsNode, AssetTypes type, const std::string &nameLower);

public:
    ~AssetTreeManager();

    AssetTreeManager(const AssetTreeManager &) = delete;
    AssetTreeManager &operator=(const AssetTreeManager &) = delete;

    std::vector<std::pair<std::string, AssetTypes>> getAssetExtensions() {
        return loadAssetExtensionList();
    }

    //main thread only, applyPendingAssetListReload frees the nodes there
    const AvailableAssetsNode* getAvailableAssetsTree() {
        std::lock_guard<std::mutex> lock(availableAssetsMutex);
        return availableAssetsRootNode;
    };

    //main thread only, applyPendingAssetListReload frees the nodes there
    const AvailableAssetsNode* getAvailableAssetsTreeFiltered(AssetTypes type, const std::string &filterText);

    //the first asset of type whose name contains name, as a path. Safe on any thread, the walk happens under the lock
    std::string findFirstAssetPathByName(AssetTypes type, const std::string &name);

    // Since we are locking the asset tree, we should return from getting the nodes as fast as possible
    // To do that, we can't just parse all mixamo assets inline.
    // This helper allows getting the list so we don't need to hold the lock while looking for the files themselves
    std::vector<std::string> getMixamoAnimationFilePaths(const std::string &modelAssetPath);

    void beginReloadAssetList();

    // Call once per frame from the main thread. Returns true when the new tree is swapped in.
    bool applyPendingAssetListReload();

    bool isReloadingAssetList() const {
        return assetListReloadInProgress.load();
    }

    uint32_t getAssetListVersion() const {
        return assetListVersion;
    }
};

#endif //LIMONENGINE_ASSETTREEMANAGER_H
