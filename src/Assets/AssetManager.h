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
public:
    enum AssetTypes { Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP, Asset_type_SOUND };

    struct EmbeddedTexture {
        char format[9] = "\0";
        uint32_t height = 0;
        uint32_t width = 0;
        uint8_t * texelData = nullptr;

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
                this->texelData = new uint8_t[this->height * this->width];
                memcpy(this->texelData, texture2.texelData, this->height* this->width);
            } else {
                //compressed data
                this->texelData = new uint8_t[this->width];
                memcpy(this->texelData, texture2.texelData, this->width);
            }

        }

        ~EmbeddedTexture() {
            delete texelData;
        }
    };
private:
    const std::string ASSET_EXTENSIONS_FILE = "./engine/assetExtensions.xml";

    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>> assets;
    std::unordered_map<std::string, std::vector<EmbeddedTexture>> embeddedTextures;
    uint32_t nextAssetIndex = 1;

    std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    GLHelper *glHelper;
    ALHelper *alHelper;

    void addAssetsRecursively(const std::string &directoryPath, const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions);

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
            if(embeddedTextures.find(files[0]) != embeddedTextures.end()) {
                embeddedTextures.erase(files[0]);
            }
        }
    }

    const std::map<std::string, AssetTypes>& getAvailableAssetsList() {
        return availableAssetsList;
    };

    void addEmbeddedTextures(const std::string& ownerAsset, std::vector<EmbeddedTexture> textures) {
        this->embeddedTextures[ownerAsset] = textures;
    }

    const EmbeddedTexture* getEmbeddedTextures(const std::string& ownerAsset, uint32_t textureID) {
        if(embeddedTextures.find(ownerAsset) == embeddedTextures.end()) {
            return nullptr;
        }
        if(embeddedTextures[ownerAsset].size() <= textureID) {
            return nullptr;
        }

        return &embeddedTextures[ownerAsset][textureID];

    }


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
