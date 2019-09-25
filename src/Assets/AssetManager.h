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

#include "Asset.h"
#include "../ALHelper.h"


class OpenGLGraphics;
class ALHelper;

class AssetManager {
public:
    enum AssetTypes { Asset_type_DIRECTORY, Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP, Asset_type_SOUND, Asset_type_UNKNOWN };

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
    const std::string ASSET_EXTENSIONS_FILE = "./Engine/assetExtensions.xml";

    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>> assets;
    std::unordered_map<std::string, std::vector<std::shared_ptr<const EmbeddedTexture>>> embeddedTextures;
    uint32_t nextAssetIndex = 1;

    //std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    AvailableAssetsNode* availableAssetsRootNode = nullptr;
    std::map<std::pair<AssetTypes, std::string>, AvailableAssetsNode*> filteredResults;
    OpenGLGraphics *glHelper;
    ALHelper *alHelper;

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

    explicit AssetManager(OpenGLGraphics *glHelper, ALHelper *alHelper) : glHelper(glHelper), alHelper(alHelper) {
        loadAssetList();
    }

    void loadUsingCereal(const std::vector<std::string> files);

    template<class T>
    T *loadAsset(const std::vector<std::string> files) {
        if (assets.count(files) == 0) {
            bool loaded = false;
            //check if asset is cereal deserialize file.
            if(files.size() == 1) {
                std::string extension = files[0].substr(files[0].find_last_of(".") + 1);
                if (extension == "limonmodel") {
                    loadUsingCereal(files);
                    loaded = true;
                }
            }
            if(!loaded) {
                assets[files] = std::make_pair(new T(this, nextAssetIndex, files), 0);
                nextAssetIndex++;
            }
        }

        assets[files].second++;
        return (T *) assets[files].first;
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
            //last element that requested the load freed, delete the object
            Asset *assetToRemove = assets[files].first;
            delete assetToRemove;
            assets.erase(files);
            if(embeddedTextures.find(files[0]) != embeddedTextures.end()) {
                embeddedTextures.erase(files[0]);
            }
        }
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


    OpenGLGraphics *getGlHelper() const {
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

        delete availableAssetsRootNode;

        for (auto tree_iterator = filteredResults.begin(); tree_iterator != filteredResults.end(); ++tree_iterator) {
            delete tree_iterator->second;
        }

    }
};


#endif //LIMONENGINE_ASSETMANAGER_H
