//
// Created by engin on 27.07.2016.
//

#include "AssetManager.h"

#include <algorithm>
#include <string>
#include <dirent.h>
#include <sys/stat.h>

void AssetManager::addAssetsRecursively(const std::string &directoryPath, const std::vector<std::pair<std::string, AssetTypes>> &fileExtensions)
{
    DIR *directory;
    struct dirent *entry;

    if (!(directory = opendir(directoryPath.c_str()))) {
        return;
    }
    while ((entry = readdir(directory)) != NULL) {
        struct stat fileStat;
        std::string filePath = directoryPath + "/" + std::string(entry->d_name);
        if(stat(filePath.c_str(),&fileStat) < 0) {
            std::cout << "stat failed for entry " << entry->d_name << std::endl;
        } else {
            if(S_ISDIR(fileStat.st_mode)) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;
                addAssetsRecursively(filePath, fileExtensions);
            } else {
                AssetTypes assetType;
                if(isExtensionInList(entry->d_name, fileExtensions, assetType)) {
                    availableAssetsList[filePath] = assetType;
                    std::cout << entry->d_name << " added as asset. with type " << assetType << std::endl;
                } else {
                    //file found but not added because extension is not in list
                }

            }
        }
    }
    closedir(directory);
}


std::vector<std::pair<std::string, AssetManager::AssetTypes>> AssetManager::loadAssetExtensionList() {
    std::vector<std::pair<std::string, AssetTypes>> extensionList;
    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLError eResult = xmlDoc.LoadFile(ASSET_EXTENSIONS_FILE.c_str());
    if (eResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Error loading Asset file list XML: " <<  xmlDoc.ErrorName() << std::endl;
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
            } else {
                std::cerr << "Requested asset type is not implemented yet. exiting.." << std::endl;
                exit(1);
            }
            extensionList.push_back(std::pair<std::string, AssetTypes>(currentAssetNode->GetText(), currentAssetType));
            std::cout << "adding available asset extension " << currentAssetNode->GetText() << std::endl;
        } else {
            std::cerr << "Asset extension without a type is invalid, skipping" << std::endl;
        }
        currentAssetNode = currentAssetNode->NextSiblingElement("Asset");
    }
    return extensionList;
}

bool AssetManager::loadAssetList() {
    std::vector<std::pair<std::string, AssetTypes>> fileExtensions = loadAssetExtensionList();
    addAssetsRecursively("./Data", fileExtensions);
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

