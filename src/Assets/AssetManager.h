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

class GLHelper;


class AssetManager {
    enum AssetTypes { Asset_type_MODEL, Asset_type_TEXTURE, Asset_type_SKYMAP };
    //second of the pair is how many times load requested. prework for unload
    std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>> assets;
    uint32_t nextAssetIndex = 1;

    std::map<std::string, AssetTypes> availableAssetsList;//this map should be ordered, or editor list order would be unpredictable
    GLHelper *glHelper;
public:

    explicit AssetManager(GLHelper *glHelper) : glHelper(glHelper) {}

    /**
     * This should be done not from file but file system. Best way is switch to c++17, but not sure
     *
     * @param assetListFile
     */
    bool loadAssetList(const std::string& assetListFile) {
        tinyxml2::XMLDocument xmlDoc;
        tinyxml2::XMLError eResult = xmlDoc.LoadFile(assetListFile.c_str());
        if (eResult != tinyxml2::XML_SUCCESS) {
            std::cerr << "Error loading Asset file list XML: " <<  xmlDoc.ErrorName() << std::endl;
            return false;
        }
        tinyxml2::XMLNode * assetsNode = xmlDoc.FirstChild();
        if (assetsNode == nullptr) {
            std::cerr << "Asset list xml is not a valid XML." << std::endl;
            return false;
        }
        const char* typeName;
        tinyxml2::XMLElement* currentAssetNode =  assetsNode->FirstChildElement("Asset");
        while(currentAssetNode != nullptr) {
            typeName = currentAssetNode->Attribute("type");
            if(!strcmp(typeName, "Model")) {// if type Model
                availableAssetsList[currentAssetNode->GetText()] = AssetTypes::Asset_type_MODEL;
                std::cout << "adding available asset " << currentAssetNode->GetText() << std::endl;
            } else {
                std::cerr << "Not implemented yet" << std::endl;
                exit(-1);
            }
            currentAssetNode = currentAssetNode->NextSiblingElement("Asset");
        }
        return true;
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

    ~AssetManager() {
        //free all the assets
        for (std::map<const std::vector<std::string>, std::pair<Asset *, uint32_t>>::iterator it = assets.begin();
             it != assets.end(); it++) {
            delete it->second.first;
        }
    }

};


#endif //LIMONENGINE_ASSETMANAGER_H
