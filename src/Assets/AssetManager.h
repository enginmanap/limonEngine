//
// Created by engin on 27.07.2016.
//

#ifndef LIMONENGINE_ASSETMANAGER_H
#define LIMONENGINE_ASSETMANAGER_H


#include <string>
#include <map>
#include <utility>
#include <tinyxml2.h>

#include "Asset.h"
#include "../ALHelper.h"

class GLHelper;
class ALHelper;

class AssetManager {

    const std::string ASSET_EXTENSIONS_FILE = "./engine/assetExtensions.xml";
    enum AssetTypes { Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP, Asset_type_SOUND };
    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>> assets;
    uint32_t nextAssetIndex = 1;

    std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    GLHelper *glHelper;
    ALHelper *alHelper;

    void addAssetsRecursively(const std::string &directoryPath, const std::vector<std::string> &fileExtensions);

    std::vector<std::string> loadAssetExtensionList();

    bool loadAssetList();

    bool isExtensionInList(const std::string &name, const std::vector<std::string> &vector);
    static bool isEnding(std::string const &fullString, std::string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }
public:

    explicit AssetManager(GLHelper *glHelper, ALHelper *alHelper) : glHelper(glHelper), alHelper(alHelper) {
        loadAssetList();
    }




    template<class T>
    T *loadAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {
            assets[files] = std::make_pair(new T(this, nextAssetIndex, files), 0);
            nextAssetIndex++;
        }

        assets[files].second++;
        return (T *) assets[files].first;
    }

    void freeAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {
            std::cerr << "Unloading an asset that was not loaded. skipping" << std::endl;
            return;
        }
        assets[files].second--;
        if (assets[files].second == 0) {
            //last element that requested the load freed, delete the object
            Asset *assetToRemove = assets[files].first;
            delete assetToRemove;
            assets.erase(files);
        }
    }

    const std::map<std::string, AssetTypes>& getAvailableAssetsList() {
        return availableAssetsList;
    };


    GLHelper *getGlHelper() const {
        return glHelper;
    }

    ALHelper *getAlHelper() const {
        return alHelper;
    }

    ~AssetManager() {
        //free all the assets
        for (std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>>::iterator it = assets.begin();
             it != assets.end(); it++) {
            delete it->second.first;
        }
    }
};


#endif //LIMONENGINE_ASSETMANAGER_H
